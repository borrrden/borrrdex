#include "uhci.h"
#include "usb.h"
#include "pci/pci.h"
#include "drivers/x86_64/pit.h"
#include "arch/x86_64/interrupt/interrupt.h"
#include "arch/x86_64/io/io.h"
#include "arch/x86_64/pic.h"
#include "paging/PageFrameAllocator.h"
#include "SmartPage.h"
#include "Panic.h"
#include "libk/string.h"
#include "io/serial.h"
#include "memory/heap.h"
#include <vector>

#define Q_TO_UINT32(x) (uint32_t)(uint64_t)(x)
#define UINT32_TO_Q(x) (uhci_queue_t *)((uint64_t)(x) & uhci::LINK_PTR_MASK)

extern "C" void __usb_irq_handler();
extern "C" void usb_handle(regs_t* regs) {
    int foo = 0;
    pic_eoi(0xC4);
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

    port_write_32(base_port + uhci::REG_FRAME_LIST_ADDR_OFFSET, (uint32_t)(uint64_t)uhci_stack);
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

int uhci_init(pci_device_t* dev) {
    uint64_t io_addr = dev->bar[4] & ~0x3;
    if(!really_uhci((void *)io_addr)) {
        return -1;
    }

    setup_controller((void *)io_addr);
    interrupt_register(0xc4 - 0x20, __usb_irq_handler);

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

    discover_devices();
    search_for_modules();
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

bool UHCIController::insert(uint8_t q, uhci_queue_t* entry, bool wait) {
    KERNEL_ASSERT(q <= QUEUE_1MS);

    auto* queue = &_queues[q];
    while((queue->element_link_ptr & uhci::LINK_PTR_TERMINATE_FLAG) == 0) {
        queue = UINT32_TO_Q(queue->element_link_ptr & uhci::LINK_PTR_MASK);
    }

    queue->element_link_ptr = Q_TO_UINT32(entry) | uhci::LINK_PTR_QUEUE_FLAG;
    entry->head_ptr = uhci::LINK_PTR_TERMINATE_FLAG;
    entry->previous_link_ptr = (uint32_t)(uint64_t)queue;
    if(!wait) {
        return true;
    }

    int timeout = 10000;
    uint16_t stat;
    while(!((stat = port_read_16(_port + uhci::REG_STATUS_OFFSET)) & uhci::STATUS_USB_INT_FLAG) && timeout > 0) {
        if(stat & uhci::STATUS_USB_ERR_INT_FLAG) {
            uart_print("Received an error interrupt while getting device descriptor...\r\n");
            remove(entry);
            return false;
        }

        timeout--;
        pit_sleepms(1);
    }

    if(timeout == 0) {
        uart_print("UHCI timed out on getting device descriptor...\r\n");
        remove(entry);
        return false;
    }

    // Clear interrupt bit (write clear)
    port_write_16(_port + uhci::REG_STATUS_OFFSET, 0x0001); 
    remove(entry);
    return true;
}

void UHCIController::remove(uhci_queue_t* entry) {
     auto* prev = UINT32_TO_Q(entry->previous_link_ptr);
     prev->element_link_ptr = entry->element_link_ptr;
     if((entry->element_link_ptr & uhci::LINK_PTR_TERMINATE_FLAG) == 0) {
         auto* next = UINT32_TO_Q(entry->element_link_ptr & uhci::LINK_PTR_MASK);
         next->previous_link_ptr = entry->previous_link_ptr;
     }
}

static void make_setup_packet(uhci_transfer_desc_t* td, void* buffer, bool ls_device, int dev_address) {
    td->link_ptr = Q_TO_UINT32(td + 1) | uhci::LINK_PTR_DEPTH_FIRST_FLAG;
    td->ctrl_status = (ls_device ? uhci::TD_LOW_SPEED_FLAG : 0) | (0b11 << uhci::TD_ERROR_OFFSET) | (0x80 << uhci::TD_STATUS_OFFSET);
    td->packet_hdr = (0x7 << uhci::TD_MAX_LEN_OFFSET) | (dev_address & uhci::TD_DEV_ADDRESS_MASK) << uhci::TD_DEV_ADDRESS_OFFSET | uhci::TD_PACKET_SETUP;
    td->buf_ptr = (uint32_t)(uint64_t)buffer;
    td->reserved[0] = 0; td->reserved[1] = 0; td->reserved[2] = 0; td->reserved[3] = 0;
}

static void make_in_packet(uhci_transfer_desc_t* td, void* buffer, bool ls_device, int dev_address, int td_idx, int size) {
    td->link_ptr =  Q_TO_UINT32(td + 1) | uhci::LINK_PTR_DEPTH_FIRST_FLAG;
    td->ctrl_status = (ls_device ? uhci::TD_LOW_SPEED_FLAG : 0) | (0b11 << uhci::TD_ERROR_OFFSET) | (0x80 << uhci::TD_STATUS_OFFSET);
    
    td->packet_hdr = ((size - 1) << uhci::TD_MAX_LEN_OFFSET) | ((td_idx & 1) ? uhci::TD_DATA_TOGGLE_FLAG : 0) | 
                        (dev_address & uhci::TD_DEV_ADDRESS_MASK) << uhci::TD_DEV_ADDRESS_OFFSET | uhci::TD_PACKET_IN;
    td->buf_ptr = (uint32_t)((uint64_t)buffer);
    td->reserved[0] = 0; td->reserved[1] = 0; td->reserved[2] = 0; td->reserved[3] = 0;
}

static void make_out_packet(uhci_transfer_desc_t* td, bool ls_device, int dev_address) {
    td->link_ptr = uhci::LINK_PTR_TERMINATE_FLAG;
    td->ctrl_status = (ls_device ? uhci::TD_LOW_SPEED_FLAG : 0) | (0b11 << uhci::TD_ERROR_OFFSET) | (0x80 << uhci::TD_STATUS_OFFSET);
    td->packet_hdr = (0x7ff << uhci::TD_MAX_LEN_OFFSET) | uhci::TD_DATA_TOGGLE_FLAG | (dev_address & uhci::TD_DEV_ADDRESS_MASK) << uhci::TD_DEV_ADDRESS_OFFSET
                     | uhci::TD_PACKET_OUT;
    td->buf_ptr = 0;
    td->reserved[0] = 0; td->reserved[1] = 0; td->reserved[2] = 0; td->reserved[3] = 0;
}

static bool verify_tds(uhci_transfer_desc_t* td, size_t count) {
    for(int i = 0; i < count; i++) {
        uint8_t td_status = (td->ctrl_status >> uhci::TD_STATUS_OFFSET) & uhci::TD_STATUS_MASK;
        if(td_status != 0) {
            return false;
        }

        td++;
    }

    return true;
}

static int prepare_setup_in_out_queue(usb_device_req_packet_t* request, void* buffer, bool ls_device, 
                                       int max_td, int dev_address, int packet_size, int size) {
    request->length = size;
    auto* queue = (uhci_queue_t *)buffer;
    queue->head_ptr = uhci::LINK_PTR_TERMINATE_FLAG;
    queue->previous_link_ptr = 0;
    queue->reserved = 0;

    uhci_transfer_desc_t* td = (uhci_transfer_desc_t *)(queue + 1);
    uhci_transfer_desc_t* start = td;
    queue->element_link_ptr = Q_TO_UINT32(td);

    // TD 0 (SETUP packet)
    make_setup_packet(td, start + max_td, ls_device, dev_address);
    td++;

    while(size > 0 && (td - start) < max_td - 1) {
        int td_idx = td - start;
        int t = std::min(size, packet_size);
        void* buffer = (void *)((uint64_t)(start + max_td) + 0x10 + (packet_size * (td_idx - 1)));
        make_in_packet(td, buffer, ls_device, dev_address, td_idx, size);
        size -= t;
        td++;
    }

    // TD 2 (OUT packet)
    make_out_packet(td, ls_device, dev_address);
    td->ctrl_status |= uhci::TD_IOC_FLAG;
    td++;

    usb_device_req_packet_t* request_dest = (usb_device_req_packet_t *)(start + max_td);
    memcpy(request_dest, request, sizeof(usb_device_req_packet_t));

    return td - start;
}

bool UHCIController::get_device_desc(usb_device_desc_t* dev_desc, bool ls_device, 
                                     int dev_address, int packet_size, int size) {
    static usb_device_req_packet_t setup_packet = {
        .request_type = usb::REQUEST_TYPE_DESCRIPTOR,
        .request = usb::REQUEST_CODE_GET_DESCRIPTOR,
        .value = usb::DESCRIPTOR_TYPE_DEVICE,
        .index = 0,
        .length = 0
    };


    const int NUM_TD_DESC = 10;

    setup_packet.length = size;

    SmartPage page(1);
    uhci_queue_t* queue = (uhci_queue_t *)(void *)page;
    int td_cnt = prepare_setup_in_out_queue(&setup_packet, (void *)page, ls_device, NUM_TD_DESC,
        dev_address, packet_size, size);

    // Clear interrupt bit (write clear)
    port_write_16(_port + uhci::REG_STATUS_OFFSET, 0x0001); 

    if(!insert(QUEUE_1MS, queue, true)) {
        return false;
    }

    uhci_transfer_desc_t* first_td = (uhci_transfer_desc_t *)(queue + 1);
    if(!verify_tds(first_td, td_cnt)) {
        return false;
    }

    *dev_desc = *(usb_device_desc_t *)((uint64_t)(first_td + NUM_TD_DESC) + 0x10);
    
    return true;
}

bool UHCIController::set_address(int dev_address, bool ls_device) {
    static usb_device_req_packet_t setup_packet = {
        .request_type = usb::REQUEST_TYPE_SET_ADDR,
        .request = usb::REQUEST_CODE_SET_ADDRESS,
        .value = 0,
        .index = 0,
        .length = 0
    };

    const int NUM_TD_DESC = 2;

    setup_packet.value = dev_address;

    SmartPage page(1);
    auto* queue = (uhci_queue_t *)(void *)page;
    queue->head_ptr = uhci::LINK_PTR_TERMINATE_FLAG;
    queue->previous_link_ptr = 0;
    queue->reserved = 0;

    uhci_transfer_desc_t* td = (uhci_transfer_desc_t *)(uint64_t)(queue + 1);
    uhci_transfer_desc_t* start = td;
    queue->element_link_ptr = Q_TO_UINT32(td);

    // TD 0 (SETUP packet)
    make_setup_packet(td, start + NUM_TD_DESC, ls_device, 0);
    td++;

    // TD 1 (ACK IN packet)
    make_in_packet(td, nullptr, ls_device, 0, 1, 0x800);
    td->link_ptr = uhci::LINK_PTR_TERMINATE_FLAG;
    td->ctrl_status |= uhci::TD_IOC_FLAG;
    td++;

    usb_device_req_packet_t* request = (usb_device_req_packet_t *)(start + NUM_TD_DESC);
    memcpy(request, &setup_packet, sizeof(usb_device_req_packet_t));

    // Clear interrupt bit (write clear)
    port_write_16(_port + uhci::REG_STATUS_OFFSET, 0x0001); 

    if(!insert(QUEUE_1MS, queue, true)) {
        return false;
    }

    return verify_tds(start, 2);
}

int UHCIController::get_language_index(uint16_t lcid, int device_address, bool ls_device) {
    static usb_device_req_packet_t setup_packet = {
        .request_type = usb::REQUEST_TYPE_DESCRIPTOR,
        .request = usb::REQUEST_CODE_GET_DESCRIPTOR,
        .value = usb::DESCRIPTOR_TYPE_STRING,
        .index = 0,
        .length = 0
    };

    int numTd = 3;
    SmartPage page(1);
    int td_cnt = prepare_setup_in_out_queue(&setup_packet, (void *)page, ls_device, 3, device_address, 8, 8);
    auto* queue = (uhci_queue_t *)(void *)page;

    // Clear interrupt bit (write clear)
    port_write_16(_port + uhci::REG_STATUS_OFFSET, 0x0001); 
    if(!insert(QUEUE_1MS, queue, true)) {
        return -1;
    }

    uhci_transfer_desc_t* first_td = (uhci_transfer_desc_t *)(queue + 1);
    if(!verify_tds(first_td, 3)) {
        return -1;
    }

    usb_string_desc_t* string_desc = (usb_string_desc_t *)((uint64_t)(first_td + numTd) + 0x10);

    if(string_desc->length > 8) {
        numTd = string_desc->length / 8 + 2;
        td_cnt = prepare_setup_in_out_queue(&setup_packet, (void *)page, ls_device, numTd, device_address, 8, string_desc->length);
        port_write_16(_port + uhci::REG_STATUS_OFFSET, 0x0001); 
        if(!insert(QUEUE_1MS, queue, true)) {
            return -1;
        }

        if(!verify_tds(first_td, td_cnt)) {
            return -1;
        }

        string_desc = (usb_string_desc_t *)((uint64_t)(first_td + numTd) + 0x10);
    }

    uint8_t numLangs = (string_desc->length - 2) / sizeof(uint16_t);
    for(int i = 0; i < numLangs; i++) {
        if((string_desc->langid[i] & 0x3FF) == lcid) {
            return i;
        }
    } 

    return -1;
}

char* UHCIController::get_string(int device_address, uint16_t port, uint8_t langIndex, uint8_t index) {
    bool ls_device = port_read_16(port) & uhci::PORT_STAT_LOSPD_FLAG;

    usb_device_req_packet_t setup_packet = {
        .request_type = usb::REQUEST_TYPE_DESCRIPTOR,
        .request = usb::REQUEST_CODE_GET_DESCRIPTOR,
        .value = (uint16_t)(usb::DESCRIPTOR_TYPE_STRING | index),
        .index = langIndex,
        .length = 0
    };

    int numTd = 3;
    SmartPage page(1);
    int td_cnt = prepare_setup_in_out_queue(&setup_packet, (void *)page, ls_device, 3, device_address, 8, 8);
    auto* queue = (uhci_queue_t *)(void *)page;

    // Clear interrupt bit (write clear)
    port_write_16(_port + uhci::REG_STATUS_OFFSET, 0x0001); 
    if(!insert(QUEUE_1MS, queue, true)) {
        return nullptr;
    }

    uhci_transfer_desc_t* first_td = (uhci_transfer_desc_t *)(queue + 1);
    if(!verify_tds(first_td, td_cnt)) {
        return nullptr;
    }

    usb_string_desc_t* string_desc = (usb_string_desc_t *)((uint64_t)(first_td + numTd) + 0x10);
    numTd = string_desc->length / 8 + 2;
    td_cnt = prepare_setup_in_out_queue(&setup_packet, (void *)page, ls_device, numTd, device_address, 8, string_desc->length);
    port_write_16(_port + uhci::REG_STATUS_OFFSET, 0x0001); 
    if(!insert(QUEUE_1MS, queue, true)) {
        return nullptr;
    }

    if(!verify_tds(first_td, td_cnt)) {
        return nullptr;
    }

    string_desc = (usb_string_desc_t *)((uint64_t)(first_td + numTd) + 0x10);
    int length = (string_desc->length - 2) / sizeof(uint16_t);
    char* retVal = (char *)kmalloc(length + 1);
    for(int i = 0; i < length; i++) {
        // Kernel only supports ASCII letters anyway, so do this poor man's conversion...
        retVal[i] = string_desc->langid[i] & 0xFF;
    }

    retVal[length] = 0;
    return retVal;
}

void UHCIController::discover_devices() {
    if(_portCount) {
        return;
    }

    int dev_address = 0;
    for(uint16_t port = _port + uhci::REG_PORT1_STAT_CTRL_OFFSET; port_is_valid(port); port += 2) {
        _portCount++;
        dev_address++;
        reset_port(port);
        if(!(port_read_16(port) & uhci::PORT_STATUS_CONN_FLAG)) {
            continue;
        }

        usb_device_desc_t dev_desc;
        bool ls_device = port_read_16(port) & uhci::PORT_STAT_LOSPD_FLAG;
        if(!get_device_desc(&dev_desc, ls_device, 0, 8, 8)) {
            continue;
        }

        reset_port(port);
        if(!set_address(dev_address, ls_device)) {
            continue;
        }

        if(!get_device_desc(&dev_desc, ls_device, dev_address, dev_desc.max_packet_size, dev_desc.length)) {
            continue;
        }

        _connectedDevices.emplace_back(dev_desc, port, dev_address);
    }
}

static module_t* usb_find_module(uint8_t classcode, uint8_t subclass, uint8_t protocol) {
    for(module_t* module = &_ModulesStart; module != &_ModulesEnd; module++) {
        if(module->module_type == MODULE_TYPE_USB) {
            usb_device_module_t *usb_module = (usb_device_module_t*)module->module;
            if(classcode == usb_module->classcode && subclass == usb_module->subclass && protocol == usb_module->protocol) {
                return module;
            }
        }
    }

    return nullptr;
}

void UHCIController::search_for_modules() {
    for(const auto& dev : _connectedDevices) {
        uint8_t dev_class = dev.descriptor()->dev_class;
        uint8_t subclass = dev.descriptor()->subclass;
        uint8_t protocol = dev.descriptor()->protocol;
        if(dev.descriptor()->dev_class == 0) {
            usb_config_desc_t* config = get_configuration(dev.address(), dev.port(), 0);
            usb_interface_desc_t* interface = (usb_interface_desc_t *)usb_find_descriptor_type(config, usb::DESCRIPTOR_RET_TYPE_INTERFACE);
            if(!interface) {
                kfree(config);
                uart_print("Unable to find interface for USB device with no class, skipping...");
                continue;
            }

            dev_class = interface->class_code;
            subclass = interface->subclass;
            protocol = interface->protocol;
            kfree(config);
        }

        module_t* m = usb_find_module(dev_class, subclass, protocol);
        if(!m) {
            uart_printf("No module found for USB device (class %d, subclass %d, protocol %d)", dev_class, subclass, protocol);
        } else {
            ((usb_device_module_t *)m->module)->device_handler(to_generic_controller(), dev.address());
        }
    }
}

usb_config_desc_t* UHCIController::get_configuration(int device_address, uint16_t port, uint8_t index) {
    bool ls_device = port_read_16(port) & uhci::PORT_STAT_LOSPD_FLAG;

    usb_device_req_packet_t setup_packet = {
        .request_type = usb::REQUEST_TYPE_DESCRIPTOR,
        .request = usb::REQUEST_CODE_GET_DESCRIPTOR,
        .value = (uint16_t)(usb::DESCRIPTOR_TYPE_CONFIG | index),
        .index = 0,
        .length = 8
    };

    int numTd = 3;
    SmartPage page(1);
    int td_cnt = prepare_setup_in_out_queue(&setup_packet, (void *)page, ls_device, 3, device_address, 8, 8);
    auto* queue = (uhci_queue_t *)(void *)page;

    // Clear interrupt bit (write clear)
    port_write_16(_port + uhci::REG_STATUS_OFFSET, 0x0001); 
    if(!insert(QUEUE_1MS, queue, true)) {
        return nullptr;
    }

    uhci_transfer_desc_t* first_td = (uhci_transfer_desc_t *)(queue + 1);
    if(!verify_tds(first_td, td_cnt)) {
        return nullptr;
    }

    usb_config_desc_t* config_desc = (usb_config_desc_t *)((uint64_t)(first_td + numTd) + 0x10);
    numTd = (config_desc->desc_length + config_desc->total_length) / 8 + 2;
    td_cnt = prepare_setup_in_out_queue(&setup_packet, (void *)page, ls_device, numTd, device_address, 8, (config_desc->desc_length + config_desc->total_length));
    port_write_16(_port + uhci::REG_STATUS_OFFSET, 0x0001); 
    if(!insert(QUEUE_1MS, queue, true)) {
        return nullptr;
    }

    if(!verify_tds(first_td, td_cnt)) {
        return nullptr;
    }

    config_desc = (usb_config_desc_t *)((uint64_t)(first_td + numTd) + 0x10);
    usb_config_desc_t* returnVal = (usb_config_desc_t *)kmalloc(config_desc->desc_length + config_desc->total_length);
    memcpy(returnVal, config_desc, config_desc->desc_length + config_desc->total_length);
    return returnVal;
}

static bool uhci_send_request(void* context, usb_device_req_packet_t* packet, int device_address, usb_buffer_t* result) {
    UHCIController* controller = (UHCIController *)context;
    return controller->send_request(packet, device_address, result);
}

bool UHCIController::send_request(usb_device_req_packet_t* packet, int device_address, usb_buffer_t* result) {
    UHCIDevice device;
    bool found = false;
    for(auto d : _connectedDevices) {
        if(d.address() == device_address) {
            device = d;
            found = true;
            break;
        }
    }

    if(!found) {
        return false;
    }

    bool ls_device = device.low_speed();
    SmartPage page(1);
    auto* queue = (uhci_queue_t *)(void *)page;
    if(packet->length == 0) {
        int numTd = 3;
        int td_cnt = prepare_setup_in_out_queue(packet, (void *)page, ls_device, 3, device_address, 8, 8);

        // Clear interrupt bit (write clear)
        port_write_16(_port + uhci::REG_STATUS_OFFSET, 0x0001); 
        if(!insert(QUEUE_1MS, queue, true)) {
            return false;
        }
        
        uhci_transfer_desc_t* first_td = (uhci_transfer_desc_t *)(queue + 1);
        if(!verify_tds(first_td, td_cnt)) {
            return false;
        }

        usb_descriptor_base_t* received = (usb_descriptor_base_t *)((uint64_t)(first_td + numTd) + 0x10);
        packet->length = received->length;
        if(received->type == usb::DESCRIPTOR_RET_TYPE_CONFIG) {
            packet->length = ((usb_config_desc_t *)received)->total_length;
        }
    }

    int numTd = packet->length / 8 + 2;
    int td_cnt = prepare_setup_in_out_queue(packet, (void *)page, ls_device, numTd, device_address, 8, packet->length);
    port_write_16(_port + uhci::REG_STATUS_OFFSET, 0x0001); 
    if(!insert(QUEUE_1MS, queue, true)) {
        return false;
    }

    uhci_transfer_desc_t* first_td = (uhci_transfer_desc_t *)(queue + 1);
    if(!verify_tds(first_td, td_cnt)) {
        return false;
    }
    
    result->buf = kmalloc(packet->length);
    result->size = packet->length;
    void* resultAddr = (void *)((uint64_t)(first_td + numTd) + 0x10);
    memcpy(result->buf, resultAddr, packet->length);
    return true;
}

generic_usb_controller_t UHCIController::to_generic_controller() {
    return {
        .real_device = this,
        .send_request = uhci_send_request,
    };
}

UHCIDevice::UHCIDevice(usb_device_desc_t desc, uint16_t port, uint8_t address)
    :_desc(desc)
    ,_port(port)
    ,_address(address)
{
    _lowSpeed = port_read_16(port) & uhci::PORT_STAT_LOSPD_FLAG;
}
