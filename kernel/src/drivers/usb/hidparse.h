#pragma once

#include <stdint.h>
#include <stddef.h>
#include <vector>
#include <stack>

namespace hid {
    // Entries all from Human Interface Devices (HID) Version 1.11
    constexpr uint8_t SIZE_MASK = 0x3;
    constexpr uint8_t SIZE_0    = 0x0;
    constexpr uint8_t SIZE_1    = 0x1;
    constexpr uint8_t SIZE_2    = 0x2;
    constexpr uint8_t SIZE_4    = 0x3;

    constexpr uint8_t TYPE_MAIN     = 0x00;
    constexpr uint8_t TYPE_GLOBAL   = 0x04;
    constexpr uint8_t TYPE_LOCAL    = 0x08;
    constexpr uint8_t TYPE_MASK     = 0x0C;

    constexpr uint8_t ITEM_MASK         = 0xFC;

    // Global Items
    constexpr uint8_t ITEM_UPAGE        = 0x04;
    constexpr uint8_t ITEM_LOG_MIN      = 0x14;
    constexpr uint8_t ITEM_LOG_MAX      = 0x24;
    constexpr uint8_t ITEM_PHY_MIN      = 0x34;
    constexpr uint8_t ITEM_PHY_MAX      = 0x44;
    constexpr uint8_t ITEM_UNIT_EXP     = 0x54;
    constexpr uint8_t ITEM_UNIT         = 0x64;
    constexpr uint8_t ITEM_REP_SIZE     = 0x74;
    constexpr uint8_t ITEM_REP_ID       = 0x84;
    constexpr uint8_t ITEM_REP_COUNT    = 0x94;
    constexpr uint8_t ITEM_PUSH         = 0xA4;
    constexpr uint8_t ITEM_POP          = 0xB4;

    // Local Items
    constexpr uint8_t ITEM_USAGE        = 0x08;
    constexpr uint8_t ITEM_USAGE_MIN    = 0x18;
    constexpr uint8_t ITEM_USAGE_MAX    = 0x28;
    constexpr uint8_t ITEM_DESIG_IDX    = 0x38;
    constexpr uint8_t ITEM_DESIG_MIN    = 0x48;
    constexpr uint8_t ITEM_DESIG_MAX    = 0x58;
    constexpr uint8_t ITEM_STRING       = 0x78;
    constexpr uint8_t ITEM_STRING_MIN   = 0x88;
    constexpr uint8_t ITEM_STRING_MAX   = 0x98;
    constexpr uint8_t ITEM_DELIMETER    = 0xA8;

    constexpr uint8_t ITEM_COLLECTION       = 0xA0;
    constexpr uint8_t ITEM_END_COLLECTION   = 0xC0;
    constexpr uint8_t ITEM_FEATURE          = 0xB0;
    constexpr uint8_t ITEM_INPUT            = 0x80;
    constexpr uint8_t ITEM_OUTPUT           = 0x90;

    constexpr uint16_t IO_ATTR_CONST         = 1 << 0;
    constexpr uint16_t IO_ATTR_VARIABLE      = 1 << 1;
    constexpr uint16_t IO_ATTR_RELATIVE      = 1 << 2;
    constexpr uint16_t IO_ATTR_WRAP          = 1 << 3;
    constexpr uint16_t IO_ATTR_NONLINEAR     = 1 << 4;
    constexpr uint16_t IO_ATTR_NO_PREF_STATE = 1 << 5;
    constexpr uint16_t IO_ATTR_HAS_NULL      = 1 << 6;
    constexpr uint16_t IO_ATTR_VOLATILE      = 1 << 7;
    constexpr uint16_t IO_ATTR_BUFFERED      = 1 << 8;
}

struct HidReportElementTemplate {
    uint32_t usage;
    uint32_t usage_min;
    uint32_t usage_max;

    uint8_t report_size;
    uint8_t report_count;
};

struct HidReportElement {
    uint32_t usage;

    int32_t logical_min;
    int32_t logical_max;

    uint8_t report_size;
};

struct HidGlobals {
    int32_t logical_min;
    int32_t logical_max;
    int32_t physical_min;
    int32_t physical_max;

    uint32_t report_size;
    uint32_t report_count;
    uint32_t report_id;
};  

class HidInputReport {
public:
    HidInputReport(int report_id, std::vector<HidReportElement>&& elements)
        :_reportId(report_id)
        ,_elements(elements) { }

    size_t element_count() const { return _elements.size(); }
    const HidReportElement* elements() const { return _elements.data(); }

private:
    int _reportId;
    std::vector<HidReportElement> _elements;
};

class HidParser {
public:
    HidParser(void* reportDesc, int size);

    void parse();
    
    size_t input_report_count() const { return _inputReports.size(); }
    const HidInputReport* input_reports() const { return _inputReports.data(); }
private:
    const uint8_t* _reportDesc;
    int _reportDescSize;
    
    const uint8_t* _position;
    uint8_t _item;
    int _value;

    HidGlobals _global;
    HidReportElementTemplate _current;
    int _reportId;

    std::vector<HidInputReport> _inputReports;
};