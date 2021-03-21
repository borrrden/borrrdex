#include "uhci.h"
#include "usb.h"
#include "pci/pci.h"
#include "drivers/x86_64/pit.h"
#include "arch/x86_64/interrupt/interrupt.h"
#include "arch/x86_64/io/io.h"
#include "paging/PageFrameAllocator.h"
#include "Panic.h"
#include "string.h"
#include <vector>

#define Q_TO_UINT32(x) (uint32_t)(uint64_t)(x)
#define UINT32_TO_Q(x) (uhci_queue_t *)((uint64_t)(x) & uhci::LINK_PTR_MASK)

extern "C" void __usb_irq_handler();
extern "C" void usb_handle(regs_t* regs) {
    int foo = 0;
}

static std::vector<UHCIController> uhci_controllers;

constexpr uint8_t UHCI_QUEUE_COUNT = 8;
constexpr uint8_t UHCI_UNIQUE_FRAME_COUNT = 128;
constexpr uint16_t UHCI_TOTAL_FRAME_COUNT = 1024;

constexpr uint8_t QUEUE_128MS   = 0;
constexpr uint8_t QUEUE_64MS    = 1;
constexpr uint8_t QUEUE_32MS    = 2;
constexpr uint8_t QUEUE_16MS    = 3;
constexpr uint8_t QUEUE_8MS     = 4;
constexpr uint8_t QUEUE_4MS     = 5;
constexpr uint8_t QUEUE_2MS     = 6;
constexpr uint8_t QUEUE_1MS     = 7;

static bool really_uhci(void* base) {
    uint16_t base_port = (uint16_t)(uint64_t)base;
    for(int i = 0; i < 5; i++) {
        port_write_16(base_port + uhci::REG_CMD_OFFSET, uhci::CMD_GLOBAL_RESET_FLAG);
        pit_sleepms(usb::WAIT_TIME_RST);
        port_write_16(base_port+ uhci::REG_CMD_OFFSET, 0x0000);
    }

    pit_sleepms(usb::WAIT_TIME_RST_RECOVERY);

    if(port_read_16(base_port + uhci::REG_CMD_OFFSET) != 0x0000) {
        return false;
    }

    if(port_read_16(base_port + uhci::REG_STATUS_OFFSET) != 0x0020) {
        return false;
    }

    // Clear on write, so this resets the status
    port_write_16(base_port + uhci::REG_STATUS_OFFSET, 0x00FF);
    if(port_read_8(base_port + uhci::REG_FRAME_MOD_START_OFFSET) != 0x40) {
        return false;
    }

    // Make sure that the following command gets cleared by the controller
    // in a timely fashion
    port_write_16(base_port + uhci::REG_CMD_OFFSET, uhci::CMD_HOST_CTRL_RESET_FLAG);
    pit_sleepms(42);
    if(port_read_16(base_port + uhci::REG_CMD_OFFSET) & uhci::CMD_HOST_CTRL_RESET_FLAG) {
        return false;
    }

    return true;
}

static void setup_controller(void* base) {
    uint16_t base_port = (uint16_t)(uint64_t)base;
    port_write_16(base_port + uhci::REG_INT_ENABLE_OFFSET, 0x0000);
    port_write_16(base_port + uhci::REG_FRAME_NO_OFFSET, 0);

    void* uhci_stack = PageFrameAllocator::SharedAllocator()->RequestPage();
    uint32_t* stack_frame = (uint32_t *)uhci_stack;
    for(int i = 0; i < UHCI_TOTAL_FRAME_COUNT; i++) {
        stack_frame[i] = uhci::FRAME_PTR_TERMINATE_FLAG;
    }

    port_write_32(base_port + uhci::FRAME_BASE_ADDRESS_OFFSET, (uint32_t)(uint64_t)uhci_stack);
    port_write_8(base_port + uhci::REG_FRAME_MOD_START_OFFSET, 0x40);
    port_write_16(base_port + uhci::REG_STATUS_OFFSET, 0x001F);

    port_write_16(base_port + uhci::REG_CMD_OFFSET, uhci::CMD_MAX_PACKET_FLAG | uhci::CMD_CONFIG_DONE_FLAG | uhci::CMD_RUN_STOP_FLAG);
    uhci_controllers.emplace_back(base_port, uhci_stack);
}

static bool port_is_valid(uint16_t port) {
    uint16_t status = port_read_16(port);
    if(!(status & 0x0080)) {
        return false;
    }

    port_write_16(port, status & ~0x0080);
    status = port_read_16(port);
    if(!(status & 0x0080)) {
        return false;
    }

    port_write_16(port, status | 0x0080);
    status = port_read_16(port);
    if(!(status & 0x0080)) {
        return false;
    }

    port_write_16(port, status | 0x000A);
    status = port_read_16(port);
    return (status & 0x000A) == 0;
}

static bool reset_port(uint16_t port) {
    uint16_t status = port_read_16(port) | uhci::PORT_STAT_RESET_FLAG;
    port_write_16(port, status);
    pit_sleepms(50);
    status &= ~uhci::PORT_STAT_RESET_FLAG;
    port_write_16(port, status);
    pit_sleepms(10);

    // Enable it in a loop, since it might not stick the first
    // few times
    for(int i = 0; i < 10; i++) {
        status = port_read_16(port);
        if(!(status & uhci::PORT_STATUS_CONN_FLAG)) {
            // Nothing connected, no need to enable
            return true;
        }

        if(status & (uhci::PORT_STAT_ENABLE_CHANGE_FLAG | uhci::PORT_STATUS_CONN_CHANGE_FLAG)) {
            status &= ~(uhci::PORT_STAT_ENABLE_CHANGE_FLAG | uhci::PORT_STATUS_CONN_CHANGE_FLAG);
            port_write_16(port, status);
            continue;
        }

        if(status & uhci::PORT_STATUS_ENABLE_FLAG) {
            return true;
        }

        status |= uhci::PORT_STATUS_ENABLE_FLAG;
        port_write_16(port, status);
        pit_sleepms(10);
    }

    // Should not really happen unless the hardware is defective..
    return false;
}

static bool enable_port(uint16_t port, bool check) {
    if(check && port_is_valid(port)) {
        // Check if this port exists, only 2 are guaranteed
        return false;
    }

    return reset_port(port);
}

static void enable_ports(void* base) {
    uint16_t start = (uint16_t)(uint64_t)base + uhci::REG_PORT1_STAT_CTRL_OFFSET;
    int index = 0;
    while(enable_port(start, index > 1)) {
        index++;
        start += 2;
    }
}

int uhci_init(pci_device_t* dev) {
    uint64_t io_addr = dev->bar[4] & ~0x3;
    if(!really_uhci((void *)io_addr)) {
        return -1;
    }

    setup_controller((void *)io_addr);
    enable_ports((void *)io_addr);

    return 0;
}

size_t UHCIController::detected_count() {
    return uhci_controllers.size();
}

UHCIController* UHCIController::get(size_t index) {
    KERNEL_ASSERT(index < detected_count());

    return &uhci_controllers[index];
}

UHCIController::UHCIController(uint16_t port, void* stack) 
    :_port(port)
    ,_stack(stack)
{
    _queues = (uhci_queue_t *)PageFrameAllocator::SharedAllocator()->RequestPage();
    for(uint8_t i = 0; i < UHCI_QUEUE_COUNT - 1; i++) {
        _queues[i] = {
            .head_ptr = Q_TO_UINT32(&_queues[i+1]),
            .element_link_ptr = uhci::LINK_PTR_TERMINATE_FLAG,
            .previous_link_ptr = 0,
            .reserved = 0
        };
    }

    _queues[UHCI_QUEUE_COUNT - 1] = {
        .head_ptr = uhci::LINK_PTR_TERMINATE_FLAG,
        .element_link_ptr = uhci::LINK_PTR_TERMINATE_FLAG,
        .previous_link_ptr = 0,
        .reserved = 0
    };

    uint32_t* stack_frame = (uint32_t *)stack;
    for(int i = 1; i <= UHCI_TOTAL_FRAME_COUNT; i++) {
        int index = i % UHCI_UNIQUE_FRAME_COUNT;
        if((index % 128) == 0) {
            stack_frame[i - 1] = Q_TO_UINT32(&_queues[QUEUE_128MS]) | uhci::FRAME_PTR_QUEUE_FLAG;
        } else if((index % 64) == 0) {
            stack_frame[i - 1] = Q_TO_UINT32(&_queues[QUEUE_64MS]) | uhci::FRAME_PTR_QUEUE_FLAG;
        } else if((index % 32) == 0) {
            stack_frame[i - 1] = Q_TO_UINT32(&_queues[QUEUE_32MS]) | uhci::FRAME_PTR_QUEUE_FLAG;
        } else if((index % 16) == 0) {
            stack_frame[i - 1] = Q_TO_UINT32(&_queues[QUEUE_16MS]) | uhci::FRAME_PTR_QUEUE_FLAG;
        } else if((index % 8) == 0) {
            stack_frame[i - 1] = Q_TO_UINT32(&_queues[QUEUE_8MS]) | uhci::FRAME_PTR_QUEUE_FLAG;
        } else if((index % 4) == 0) {
            stack_frame[i - 1] = Q_TO_UINT32(&_queues[QUEUE_4MS]) | uhci::FRAME_PTR_QUEUE_FLAG;
        } else if((index % 2) == 0) {
            stack_frame[i - 1] = Q_TO_UINT32(&_queues[QUEUE_2MS]) | uhci::FRAME_PTR_QUEUE_FLAG;
        } else {
            stack_frame[i - 1] = Q_TO_UINT32(&_queues[QUEUE_1MS]) | uhci::FRAME_PTR_QUEUE_FLAG;
        }
    }
}

UHCIController::~UHCIController() {
    if(_queues) {
        PageFrameAllocator::SharedAllocator()->FreePage((void *)_queues);
    }
}

UHCIController::UHCIController(UHCIController&& other) 
    :_stack(other._stack)
    ,_queues(other._queues)
    ,_port(other._port)
{
    other._port = 0;
    other._queues = nullptr;
    other._stack = nullptr;
}

void UHCIController::insert(uint8_t q, uhci_queue_t* entry) {
    KERNEL_ASSERT(q <= QUEUE_1MS);

    auto* queue = &_queues[q];
    while((queue->element_link_ptr & uhci::LINK_PTR_TERMINATE_FLAG) == 0) {
        queue = UINT32_TO_Q(queue->element_link_ptr & uhci::LINK_PTR_MASK);
    }

    queue->element_link_ptr = Q_TO_UINT32(entry) | uhci::LINK_PTR_QUEUE_FLAG;
    entry->head_ptr = uhci::LINK_PTR_TERMINATE_FLAG;
    entry->previous_link_ptr = (uint32_t)(uint64_t)queue;
}

void UHCIController::remove(uhci_queue_t* entry) {
     auto* prev = UINT32_TO_Q(entry->previous_link_ptr);
     prev->element_link_ptr = entry->element_link_ptr;
     if((entry->element_link_ptr & uhci::LINK_PTR_TERMINATE_FLAG) == 0) {
         auto* next = UINT32_TO_Q(entry->element_link_ptr & uhci::LINK_PTR_MASK);
         next->previous_link_ptr = entry->previous_link_ptr;
     }
}

constexpr uint8_t UHCI_REQUEST_WORD_SIZE = 4;

bool UHCIController::get_device_desc(usb_device_desc_t* dev_desc, bool ls_device, 
                                     int dev_address, int packet_size, int size) {
    static usb_device_req_packet_t setup_packet = {
        .request_type = usb::REQUEST_TYPE_DESCRIPTOR,
        .request = usb::REQUEST_CODE_GET_DESCRIPTOR,
        .value = usb::DESCRIPTOR_TYPE_DEVICE,
        .index = 0,
        .length = 0
    };

    setup_packet.length = size;

    void* buffer = PageFrameAllocator::SharedAllocator()->RequestPage();
    auto* queue = (uhci_queue_t *)buffer;
    queue->head_ptr = uhci::LINK_PTR_TERMINATE_FLAG;
    queue->previous_link_ptr = 0;
    queue->reserved = 0;

    uhci_transfer_desc_t* td = (uhci_transfer_desc_t *)(uint64_t)(queue + 1);
    uhci_transfer_desc_t* start = td;
    queue->element_link_ptr = Q_TO_UINT32(td);

    // TD 0 (SETUP packet)
    td->link_ptr = Q_TO_UINT32(td + 1) | uhci::LINK_PTR_DEPTH_FIRST_FLAG;
    td->reply = (ls_device ? uhci::TD_LOW_SPEED_FLAG : 0) | (0b11 << uhci::TD_ERROR_OFFSET) | (0x80 << uhci::TD_STATUS_OFFSET);
    td->info = (0x7 << uhci::TD_MAX_LEN_OFFSET) | (uhci::TD_PACKET_SETUP + 1);
    td->buf_ptr = (uint32_t)(uint64_t)(start + 3);
    td->reserved[0] = 0; td->reserved[1] = 0; td->reserved[2] = 0; td->reserved[3] = 0;
    td++;
    
    // TD 1 (IN packet)
    td->link_ptr = Q_TO_UINT32(td + 1) | uhci::LINK_PTR_DEPTH_FIRST_FLAG;
    td->reply = (ls_device ? uhci::TD_LOW_SPEED_FLAG : 0) | (0b11 << uhci::TD_ERROR_OFFSET) | (0x80 << uhci::TD_STATUS_OFFSET);
    td->info = (0x7 << uhci::TD_MAX_LEN_OFFSET) | uhci::TD_DATA_TOGGLE_FLAG | uhci::TD_PACKET_IN;
    td->buf_ptr = (uint32_t)(uint64_t)(start + 3 + sizeof(usb_device_req_packet_t));
    td->reserved[0] = 0; td->reserved[1] = 0; td->reserved[2] = 0; td->reserved[3] = 0;
    td++;

    // TD 2 (OUT packet)
    td->link_ptr = uhci::LINK_PTR_TERMINATE_FLAG;
    td->reply = (ls_device ? uhci::TD_LOW_SPEED_FLAG : 0) | (0b11 << uhci::TD_ERROR_OFFSET) | (0x80 << uhci::TD_STATUS_OFFSET);
    td->info = (0x7ff << uhci::TD_MAX_LEN_OFFSET) | uhci::TD_PACKET_OUT | uhci::TD_IOC_FLAG;
    td->reserved[0] = 0; td->reserved[1] = 0; td->reserved[2] = 0; td->reserved[3] = 0;
    td++;
    
    usb_device_req_packet_t* request = (usb_device_req_packet_t *)td;
    memcpy(request, &setup_packet, sizeof(usb_device_req_packet_t));

    insert(QUEUE_1MS, queue);

    uint16_t stat;
    uint16_t frame_no;
    uint8_t td_stat;
    td = start;
    do {
        frame_no = port_read_16(_port + uhci::REG_FRAME_NO_OFFSET) & 0x7ff;
        stat = port_read_16(_port + uhci::REG_STATUS_OFFSET);
        td_stat = (td->reply >> uhci::TD_STATUS_OFFSET) & uhci::TD_STATUS_MASK;
    } while((stat & 0x1) == 0);
}

void UHCIController::discover_devices() {
    uint16_t port = _port + uhci::REG_PORT1_STAT_CTRL_OFFSET;
    int dev_address = 1;
    while(port_is_valid(port)) {
        usb_device_desc_t dev_desc;
        bool ls_device = port_read_16(port) & uhci::PORT_STAT_LOSPD_FLAG;
        if(get_device_desc(&dev_desc, ls_device, 0, 8, 8)) {

        }
        port += 2;
    }
}