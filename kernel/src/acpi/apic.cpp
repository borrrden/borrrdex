#include "apic.h"
#include "string.h"

size_t MADT::count() const {
    uint8_t* start = _data->entries;
    uint8_t* current = start;
    size_t len = _data->h.length - sizeof(_data->h);

    size_t total = 0;
    while(current - start < len) {
        int_controller_header_t* h = (int_controller_header_t *)current;
        total++;
        current += h->length;
    }

    return total;
}

int_controller_header_t* MADT::get(size_t index) const {
    uint8_t* start = _data->entries;
    uint8_t* current = start;
    size_t len = _data->h.length - sizeof(_data->h);
    size_t currentIdx = 0;
    while(current - start < len) {
        int_controller_header_t* h = (int_controller_header_t *)current;
        if(currentIdx++ == index) {
            return h;
        }

        current += h->length;
    }

    return nullptr;
}

bool MADT::is_valid() const {
    return _data && strncmp(MADT::signature, _data->h.signature, 4) == 0;
}

namespace lapic {
    // LAPIC Registers Intel SDM Volume 3 Table 10-1
    constexpr uint16_t REG_OFFSET_ID                        = 0x0030;
    constexpr uint16_t REG_OFFSET_VERSION                   = 0x0040;
    constexpr uint16_t REG_OFFSET_TASK_PRIORITY             = 0x0080;
    constexpr uint16_t REG_OFFSET_ARBITRATION_PRIORITY      = 0x0090;
    constexpr uint16_t REG_OFFSET_PROCESSOR_PRIORITY        = 0x00A0;
    constexpr uint16_t REG_OFFSET_EOI                       = 0x00B0;
    constexpr uint16_t REG_OFFSET_REMOTE_READ               = 0x00C0;
    constexpr uint16_t REG_OFFSET_LOGICAL_DESTINATION       = 0x00D0;
    constexpr uint16_t REG_OFFSET_DESTINATION_FORMAT        = 0x00E0;
    constexpr uint16_t REG_OFFSET_SPURIOUS_INT_VECTOR       = 0x00F0;
    constexpr uint16_t REG_OFFSET_IN_SERVICE                = 0x0100;
    constexpr uint16_t REG_OFFSET_TRIGGER_MODE              = 0x0180;
    constexpr uint16_t REG_OFFSET_INTERRUPT_REQUEST         = 0x0200;
    constexpr uint16_t REG_OFFSET_ERROR_STATUS              = 0x0280;
    constexpr uint16_t REG_OFFSET_CMCI                      = 0x02F0;
    constexpr uint16_t REG_OFFSET_INTERRUPT_COMMAND         = 0x0300;
    constexpr uint16_t REG_OFFSET_LVT_TIMER                 = 0x0320;
    constexpr uint16_t REG_OFFSET_LVT_THERMAL_SENSOR        = 0x0330;
    constexpr uint16_t REG_OFFSET_LVT_PERF_MONITOR          = 0x0340;
    constexpr uint16_t REG_OFFSET_LVT_LINT0                 = 0x0350;
    constexpr uint16_t REG_OFFSET_LVT_LINT1                 = 0x0360;
    constexpr uint16_t REG_OFFSET_LVT_ERROR                 = 0x0370;
    constexpr uint16_t REG_OFFSET_INITIAL_COUNT             = 0x0380;
    constexpr uint16_t REG_OFFSET_CURRENT_COUNT             = 0x0390;
    constexpr uint16_t REG_OFFSET_DIVIDE_CONFIG             = 0x03E0;
}

LAPIC::LAPIC(void* memoryAddress)
    :_memoryAddress((uint8_t *)memoryAddress)
{

}

uint32_t LAPIC::id() const {
    return *((volatile uint32_t *)(_memoryAddress + lapic::REG_OFFSET_ID));
}

void LAPIC::set_id(uint32_t val) {
    *((volatile uint32_t *)(_memoryAddress + lapic::REG_OFFSET_ID)) = val;
}

uint32_t LAPIC::version() const {
    return *((volatile uint32_t *)(_memoryAddress + lapic::REG_OFFSET_VERSION));
}

uint32_t LAPIC::task_priority() const {
    return *((volatile uint32_t *)(_memoryAddress + lapic::REG_OFFSET_TASK_PRIORITY));
}

void LAPIC::set_task_priority(uint32_t val) {
    *((volatile uint32_t *)(_memoryAddress + lapic::REG_OFFSET_TASK_PRIORITY)) = val;
}

uint32_t LAPIC::arbitration_priority() const {
    return *((volatile uint32_t *)(_memoryAddress + lapic::REG_OFFSET_ARBITRATION_PRIORITY));
}

uint32_t LAPIC::processor_priority() const {
    return *((volatile uint32_t *)(_memoryAddress + lapic::REG_OFFSET_PROCESSOR_PRIORITY));
}

void LAPIC::eoi(uint32_t val) {
    *((volatile uint32_t *)(_memoryAddress + lapic::REG_OFFSET_EOI)) = val;
}

uint32_t LAPIC::remote_read() const {
    return *((volatile uint32_t *)(_memoryAddress + lapic::REG_OFFSET_REMOTE_READ));
}

uint32_t LAPIC::logical_destination() const {
    return *((volatile uint32_t *)(_memoryAddress + lapic::REG_OFFSET_LOGICAL_DESTINATION));
}

void LAPIC::set_logical_destination(uint32_t val) {
    *((volatile uint32_t *)(_memoryAddress + lapic::REG_OFFSET_LOGICAL_DESTINATION)) = val;
}

uint32_t LAPIC::destination_format() const {
    return *((volatile uint32_t *)(_memoryAddress + lapic::REG_OFFSET_DESTINATION_FORMAT));
}

void LAPIC::set_destination_format(uint32_t val) {
    *((volatile uint32_t *)(_memoryAddress + lapic::REG_OFFSET_DESTINATION_FORMAT)) = val;
}

uint32_t LAPIC::spurious_interrupt_vector() const {
    return *((volatile uint32_t *)(_memoryAddress + lapic::REG_OFFSET_SPURIOUS_INT_VECTOR));
}

void LAPIC::set_spurious_interrupt_vector(uint32_t val) {
    *((volatile uint32_t *)(_memoryAddress + lapic::REG_OFFSET_SPURIOUS_INT_VECTOR)) = val;
}

uint32_t LAPIC::error_status() const {
    return *((volatile uint32_t *)(_memoryAddress + lapic::REG_OFFSET_ERROR_STATUS));
}

void LAPIC::interrupt_command(uint32_t* lower, uint32_t* upper) const {
    if(lower) {
        *lower = *((volatile uint32_t *)(_memoryAddress + lapic::REG_OFFSET_INTERRUPT_COMMAND));
    }

    if(upper) {
        *upper = *((volatile uint32_t *)(_memoryAddress + lapic::REG_OFFSET_INTERRUPT_COMMAND + 0x10));
    }
}

void LAPIC::set_interrupt_command(uint32_t lower, uint32_t upper) {
    *((volatile uint32_t *)(_memoryAddress + lapic::REG_OFFSET_INTERRUPT_COMMAND + 0x10)) = upper;
    *((volatile uint32_t *)(_memoryAddress + lapic::REG_OFFSET_INTERRUPT_COMMAND)) = lower;
}

uint32_t LAPIC::lvt_timer() const {
    return *((volatile uint32_t *)(_memoryAddress + lapic::REG_OFFSET_LVT_TIMER));
}

void LAPIC::set_lvt_timer(uint32_t val) {
    *((volatile uint32_t *)(_memoryAddress + lapic::REG_OFFSET_LVT_TIMER)) = val;
}

uint32_t LAPIC::lvt_thermal_sensor() const {
    return *((volatile uint32_t *)(_memoryAddress + lapic::REG_OFFSET_LVT_THERMAL_SENSOR));
}

void LAPIC::set_lvt_thermal_sensor(uint32_t val) {
    *((volatile uint32_t *)(_memoryAddress + lapic::REG_OFFSET_LVT_THERMAL_SENSOR)) = val;
}

uint32_t LAPIC::lvt_perf_monitor() const {
    return *((volatile uint32_t *)(_memoryAddress + lapic::REG_OFFSET_LVT_PERF_MONITOR));
}

void LAPIC::set_lvt_perf_monitor(uint32_t val) {
    *((volatile uint32_t *)(_memoryAddress + lapic::REG_OFFSET_LVT_PERF_MONITOR)) = val;
}

uint32_t LAPIC::lvt_lint0() const {
    return *((volatile uint32_t *)(_memoryAddress + lapic::REG_OFFSET_LVT_LINT0));
}

void LAPIC::set_lvt_lint0(uint32_t val) {
    *((volatile uint32_t *)(_memoryAddress + lapic::REG_OFFSET_LVT_LINT0)) = val;
}

uint32_t LAPIC::lvt_lint1() const {
    return *((volatile uint32_t *)(_memoryAddress + lapic::REG_OFFSET_LVT_LINT1));
}

void LAPIC::set_lvt_lint1(uint32_t val) {
    *((volatile uint32_t *)(_memoryAddress + lapic::REG_OFFSET_LVT_LINT1)) = val;
}

uint32_t LAPIC::lvt_error() const {
    return *((volatile uint32_t *)(_memoryAddress + lapic::REG_OFFSET_LVT_ERROR));
}

void LAPIC::set_lvt_error(uint32_t val) {
    *((volatile uint32_t *)(_memoryAddress + lapic::REG_OFFSET_LVT_ERROR)) = val;
}

uint32_t LAPIC::initial_count() const {
    return *((volatile uint32_t *)(_memoryAddress + lapic::REG_OFFSET_INITIAL_COUNT));
}

void LAPIC::set_initial_count(uint32_t val) {
    *((volatile uint32_t *)(_memoryAddress + lapic::REG_OFFSET_INITIAL_COUNT)) = val;
}

uint32_t LAPIC::current_count() const {
    return *((volatile uint32_t *)(_memoryAddress + lapic::REG_OFFSET_CURRENT_COUNT));
}

uint32_t LAPIC::divide_config() const {
    return *((volatile uint32_t *)(_memoryAddress + lapic::REG_OFFSET_DIVIDE_CONFIG));
}

void LAPIC::set_divide_config(uint32_t val) {
    *((volatile uint32_t *)(_memoryAddress + lapic::REG_OFFSET_DIVIDE_CONFIG)) = val;
}