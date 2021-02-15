#include "apic.h"
#include "string.h"

constexpr const char* MADT_SIGNATURE = "APIC";
constexpr const char* MADT::signature() {
    return MADT_SIGNATURE;
}

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
