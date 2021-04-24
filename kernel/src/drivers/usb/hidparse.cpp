#include "hidparse.h"
#include "libk/string.h"
#include "graphics/BasicRenderer.h"
#include "pci/PciDeviceType.h"
#include <queue>

// HID Usage Tables 1.12
constexpr uint8_t PAGE_GENERIC_DESKTOP  = 0x01;
constexpr uint8_t PAGE_SIM_CONTROLS     = 0x02;
constexpr uint8_t PAGE_VR_CONTROLS      = 0x03;
constexpr uint8_t PAGE_SPORTS_CONTROLS  = 0x04;
constexpr uint8_t PAGE_GAME_CONTROLS    = 0x05;
constexpr uint8_t PAGE_GEN_DEV_CONTROLS = 0x06;
constexpr uint8_t PAGE_KEYBOARD         = 0x07;
constexpr uint8_t PAGE_LED              = 0x08;
constexpr uint8_t PAGE_BUTTON           = 0x09;
constexpr uint8_t PAGE_ORDINAL          = 0x0A;
constexpr uint8_t PAGE_TELEPHONY        = 0x0B;
constexpr uint8_t PAGE_CONSUMER         = 0x0C;
constexpr uint8_t PAGE_DIGITIZERS       = 0x0D;
constexpr uint8_t PAGE_UNICODE          = 0x10;
constexpr uint8_t PAGE_ALPHA_DISPLAY    = 0x14;
constexpr uint8_t PAGE_MEDICAL_INST     = 0x40;

constexpr uint8_t USAGE_POINTER         = 0x01;
constexpr uint8_t USAGE_MOUSE           = 0x02;
constexpr uint8_t USAGE_JOYSTICK        = 0x04;
constexpr uint8_t USAGE_GAMEPAD         = 0x05;
constexpr uint8_t USAGE_KEYBOARD        = 0x06;
constexpr uint8_t USAGE_KEYPAD          = 0x07;
constexpr uint8_t USAGE_MULTI_AXIS_CONT = 0x08;
constexpr uint8_t USAGE_TABLET_SYS_CONT = 0x09;
constexpr uint8_t USAGE_X               = 0x30;
constexpr uint8_t USAGE_Y               = 0x31;
constexpr uint8_t USAGE_Z               = 0x32;
constexpr uint8_t USAGE_RX              = 0x33;
constexpr uint8_t USAGE_RY              = 0x34;
constexpr uint8_t USAGE_RZ              = 0x35;
constexpr uint8_t USAGE_SLIDER          = 0x36;
constexpr uint8_t USAGE_DIAL            = 0x37;
constexpr uint8_t USAGE_WHEEL           = 0x38;
constexpr uint8_t USAGE_HAT_SWITCH      = 0x39;
constexpr uint8_t USAGE_COUNTED_BUF     = 0x3A;
constexpr uint8_t USAGE_BYTE_COUNT      = 0x3B;
constexpr uint8_t USAGE_MOTION_WAKE     = 0x3C;
constexpr uint8_t USAGE_START           = 0x3D;
constexpr uint8_t USAGE_SELECT          = 0x3E;
constexpr uint8_t USAGE_VX              = 0x40;
constexpr uint8_t USAGE_VY              = 0x41;
constexpr uint8_t USAGE_VZ              = 0x42;
constexpr uint8_t USAGE_VBRX            = 0x43;
constexpr uint8_t USAGE_VBRY            = 0x44;
constexpr uint8_t USAGE_VBRZ            = 0x45;
constexpr uint8_t USAGE_VNO             = 0x46;
constexpr uint8_t USAGE_FEAT_NOTIFY     = 0x47;
constexpr uint8_t USAGE_RES_MULT        = 0x48;
constexpr uint8_t USAGE_SYS_CONTROL     = 0x80;
constexpr uint8_t USAGE_SYS_POWER_DWN   = 0x81;
constexpr uint8_t USAGE_SYS_SLEEP       = 0x82;
constexpr uint8_t USAGE_SYS_WAKEUP      = 0x83;
constexpr uint8_t USAGE_SYS_CTXT_MENU   = 0x84;
constexpr uint8_t USAGE_SYS_MAIN_MENU   = 0x85;
constexpr uint8_t USAGE_SYS_APP_MENU    = 0x86;
constexpr uint8_t USAGE_SYS_MENU_HELP   = 0x87;
constexpr uint8_t USAGE_SYS_MENU_EXIT   = 0x88;
constexpr uint8_t USAGE_SYS_MENU_SEL    = 0x89;
constexpr uint8_t USAGE_SYS_MENU_RIGHT  = 0x8A;
constexpr uint8_t USAGE_SYS_MENU_LEFT   = 0x8B;
constexpr uint8_t USAGE_SYS_MENU_UP     = 0x8C;
constexpr uint8_t USAGE_SYS_MENU_DOWN   = 0x8D;
constexpr uint8_t USAGE_SYS_CLD_RESTART = 0x8E;
constexpr uint8_t USAGE_SYS_WRM_RESTART = 0x8F;
constexpr uint8_t USAGE_DPAD_UP         = 0x90;
constexpr uint8_t USAGE_DPAD_DOWN       = 0x91;
constexpr uint8_t USAGE_DPAD_RIGHT      = 0x92;
constexpr uint8_t USAGE_DPAD_LEFT       = 0x93;
constexpr uint8_t USAGE_SYS_DOCK        = 0xA0;
constexpr uint8_t USAGE_SYS_UNDOCK      = 0xA1;
constexpr uint8_t USAGE_SYS_SETUP       = 0xA2;
constexpr uint8_t USAGE_SYS_BREAK       = 0xA3;
constexpr uint8_t USAGE_SYS_DBG_BREAK   = 0xA4;
constexpr uint8_t USAGE_APP_BREAK       = 0xA5;
constexpr uint8_t USAGE_APP_DBG_BREAK   = 0xA6;
constexpr uint8_t USAGE_SYS_SPK_MUTE    = 0xA7;
constexpr uint8_t USAGE_SYS_HIBERNATE   = 0xA8;
constexpr uint8_t USAGE_SYS_DISP_INVERT = 0xB0;
constexpr uint8_t USAGE_SYS_DISP_INT    = 0xB1;
constexpr uint8_t USAGE_SYS_DISP_EXT    = 0xB2;
constexpr uint8_t USAGE_SYS_DISP_BOTH   = 0xB3;
constexpr uint8_t USAGE_SYS_DISP_DUAL   = 0xB4;
constexpr uint8_t USAGE_SYS_DISP_TOGGLE = 0xB5;
constexpr uint8_t USAGE_SYS_DISP_SWAP   = 0xB6;
constexpr uint8_t USAGE_SYS_DISP_AUTOSCALE  = 0xB7;

constexpr uint8_t COLLECTION_PHYSICAL       = 0x00;
constexpr uint8_t COLLECTION_APPLICATION    = 0x01;
constexpr uint8_t COLLECTION_LOGICAL        = 0x02;
constexpr uint8_t COLLECTION_REPORT         = 0x03;
constexpr uint8_t COLLECTION_NAMED_ARRAY    = 0x04;
constexpr uint8_t COLLECTION_USAGE_SWITCH   = 0x05;

static DeviceTypeEntry<uint8_t> PageNames[] = {
    { PAGE_GENERIC_DESKTOP, "Generic Desktop" },
    { PAGE_SIM_CONTROLS, "Simulation Controls" },
    { PAGE_VR_CONTROLS, "VR Controls" },
    { PAGE_SPORTS_CONTROLS, "Sports Controls" },
    { PAGE_GAME_CONTROLS, "Game Controls" },
    { PAGE_GEN_DEV_CONTROLS, "Generic Device Controls" },
    { PAGE_KEYBOARD, "Keyboard/Keypad" },
    { PAGE_LED, "LED" },
    { PAGE_BUTTON, "Button" },
    { PAGE_ORDINAL, "Ordinal" },
    { PAGE_TELEPHONY, "Telephony" },
    { PAGE_CONSUMER, "Consumer" },
    { PAGE_DIGITIZERS, "Digitizers" },
    { PAGE_UNICODE, "Unicode" },
    { PAGE_ALPHA_DISPLAY, "Alphanumeric Display" },
    { PAGE_MEDICAL_INST, "Medical Instrument" },
};

static DeviceTypeEntry<uint16_t> UsageNames[] = {
    { hash(PAGE_GENERIC_DESKTOP, 0x01), "Pointer" },
    { hash(PAGE_GENERIC_DESKTOP, 0x02), "Mouse" },
    { hash(PAGE_GENERIC_DESKTOP, 0x04), "Joystick" },
    { hash(PAGE_GENERIC_DESKTOP, 0x05), "Game Pad" },
    { hash(PAGE_GENERIC_DESKTOP, 0x06), "Keyboard" },
    { hash(PAGE_GENERIC_DESKTOP, 0x07), "Keypad" },
    { hash(PAGE_GENERIC_DESKTOP, 0x08), "Multi-axis Controller" },
    { hash(PAGE_GENERIC_DESKTOP, 0x09), "Tablet PC System Controls" },
    { hash(PAGE_GENERIC_DESKTOP, 0x30), "X" },
    { hash(PAGE_GENERIC_DESKTOP, 0x31), "Y" },
    { hash(PAGE_GENERIC_DESKTOP, 0x32), "Z" },
    { hash(PAGE_GENERIC_DESKTOP, 0x33), "Rx" },
    { hash(PAGE_GENERIC_DESKTOP, 0x34), "Ry" },
    { hash(PAGE_GENERIC_DESKTOP, 0x35), "Rz" },
    { hash(PAGE_GENERIC_DESKTOP, 0x36), "Slider" },
    { hash(PAGE_GENERIC_DESKTOP, 0x33), "Rx" },
};

static DeviceTypeEntry<uint8_t> CollectionTypes[] = {
    { COLLECTION_PHYSICAL, "Physical" },
    { COLLECTION_APPLICATION, "Application" },
    { COLLECTION_LOGICAL, "Logical" },
    { COLLECTION_REPORT, "Report" },
    { COLLECTION_NAMED_ARRAY, "Named Array" },
    { COLLECTION_USAGE_SWITCH, "Usage Switch" }
};

struct HidNode {
    uint16_t page;
    uint16_t usage;

    HidNode(uint16_t page, uint16_t usage)
        :page(page)
        ,usage(usage) {}
};

HidParser::HidParser(void* reportDesc, int size) 
    :_reportDesc((const uint8_t *)reportDesc)
    ,_reportDescSize(size)
    ,_position((const uint8_t *)reportDesc)
{

}

static int ITEM_SIZES[4] = { 0, 1, 2, 4 };
void HidParser::parse() {
    std::queue<HidNode> usageQueue;

   while(_position - _reportDesc < _reportDescSize) {
       _item = *_position++;
       int size = ITEM_SIZES[_item & hid::SIZE_MASK];
       _value = 0;
       memcpy(&_value, _position, size);
       _position += size;

       switch(_item & hid::ITEM_MASK) {
            case hid::ITEM_UPAGE:
            {
                usageQueue.emplace((uint16_t)_value, (uint16_t)0xFF);
                const char* pageName = binary_search(PageNames, (uint8_t)_value, 0, sizeof(PageNames) / sizeof(DeviceTypeEntry<uint8_t>) - 1);
                GlobalRenderer->Printf("Usage Page(%s)", pageName);
                break;
            }
            case hid::ITEM_USAGE:
            {
                usageQueue.back().usage = (uint16_t)_value;
                uint16_t hash = ::hash((uint8_t)usageQueue.back().page, (uint8_t)_value);
                const char* usageName = binary_search(UsageNames, hash, 0, sizeof(UsageNames) / sizeof(DeviceTypeEntry<uint16_t>) - 1);
                GlobalRenderer->Printf("Usage(%s)", usageName);
                break;
            }
            case hid::ITEM_INPUT:
            {
                GlobalRenderer->Printf("Input(");
                GlobalRenderer->Printf("%s", _value & hid::IO_ATTR_CONST ? "Const, " : "Data, ");
                GlobalRenderer->Printf("%s", _value & hid::IO_ATTR_VARIABLE ? "Variable, " : "Array, ");
                GlobalRenderer->Printf("%s", _value & hid::IO_ATTR_RELATIVE ? "Relative, " : "Absolute, ");
                GlobalRenderer->Printf("%s", _value & hid::IO_ATTR_WRAP ? "Wrap, " : "No Wrap, ");
                GlobalRenderer->Printf("%s", _value & hid::IO_ATTR_NONLINEAR ? "Non-Linear, " : "Linear, ");
                GlobalRenderer->Printf("%s", _value & hid::IO_ATTR_NO_PREF_STATE ? "No Preferred State, " : "Preferred State, ");
                GlobalRenderer->Printf("%s", _value & hid::IO_ATTR_HAS_NULL ? "Has Null, " : "No Null, ");
                GlobalRenderer->Printf("%s", _value & hid::IO_ATTR_BUFFERED ? "Buffered Bytes)" : "Bitfield)");

                HidNode node = usageQueue.front();
                // usageQueue.pop();
                // HidInputReport report((int)_global.report_id, std::move(_current));
                // _current.clear();

                break;
            }
            case hid::ITEM_OUTPUT:
                GlobalRenderer->Printf("Output(");
                GlobalRenderer->Printf("%s", _value & hid::IO_ATTR_CONST ? "Const, " : "Data, ");
                GlobalRenderer->Printf("%s", _value & hid::IO_ATTR_VARIABLE ? "Variable, " : "Array, ");
                GlobalRenderer->Printf("%s", _value & hid::IO_ATTR_RELATIVE ? "Relative, " : "Absolute, ");
                GlobalRenderer->Printf("%s", _value & hid::IO_ATTR_WRAP ? "Wrap, " : "No Wrap, ");
                GlobalRenderer->Printf("%s", _value & hid::IO_ATTR_NONLINEAR ? "Non-Linear, " : "Linear, ");
                GlobalRenderer->Printf("%s", _value & hid::IO_ATTR_NO_PREF_STATE ? "No Preferred State, " : "Preferred State, ");
                GlobalRenderer->Printf("%s", _value & hid::IO_ATTR_HAS_NULL ? "Has Null, " : "No Null, ");
                GlobalRenderer->Printf("%s", _value & hid::IO_ATTR_VOLATILE ? "Volatile, " : "Non-Volatile, ");
                GlobalRenderer->Printf("%s", _value & hid::IO_ATTR_BUFFERED ? "Buffered Bytes)" : "Bitfield)");
                break;
            case hid::ITEM_REP_COUNT:
                GlobalRenderer->Printf("Report Count(%d)", _value);
                _global.report_count = (uint32_t)_value;
                break;
            case hid::ITEM_REP_SIZE:
                GlobalRenderer->Printf("Report Size(%d)", _value);
                _global.report_size = (uint32_t)_value;
                break;
            case hid::ITEM_LOG_MIN:
                GlobalRenderer->Printf("Logical Minimum(%d)", _value);
                _global.logical_min = _value;
                break;
            case hid::ITEM_LOG_MAX:
                GlobalRenderer->Printf("Logical Maximum(%d)", _value);
                _global.logical_max = _value;
                break;
            case hid::ITEM_USAGE_MIN:
                GlobalRenderer->Printf("Usage Minimum(%d)", _value);
                break;
            case hid::ITEM_USAGE_MAX:
                GlobalRenderer->Printf("Usage Maximum(%d)", _value);
                break;
            case hid::ITEM_COLLECTION:
            {
                const char* colType = binary_search(CollectionTypes, (uint8_t)_value, 0, sizeof(CollectionTypes) / sizeof(DeviceTypeEntry<uint8_t>) - 1);
                GlobalRenderer->Printf("Collection(%s)", colType);
                break;
            }
            case hid::ITEM_END_COLLECTION:
                GlobalRenderer->Printf("End Collection()");
                break;
            default:
                continue;
        }

        uint8_t type = _item & hid::TYPE_MASK;
        if(type == hid::TYPE_MAIN) {
            GlobalRenderer->Printf(" - Main\n");
        } else {
            GlobalRenderer->Printf(type == hid::TYPE_GLOBAL ? " - Global\n" : " - Local\n");
        }
   }
}