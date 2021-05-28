#include <keyboard.h>
#include <idt.h>
#include <apic.h>
#include <io.h>
#include <device.h>
#include <timer.h>
#include <panic.h>
#include <logging.h>
#include <fs/directory_entry.h>
#include <kmath.h>

namespace keyboard {
    constexpr uint8_t DATA_PORT         = 0x60;
    constexpr uint8_t CMD_PORT          = 0x64;
    constexpr uint8_t STATUS_PORT       = 0x64;

    // Commands
    constexpr uint8_t READ_CCB      = 0x20;
    constexpr uint8_t WRITE_CCB     = 0x60;
    constexpr uint8_t DISABLE_AUX   = 0xA7;
    constexpr uint8_t ENABLE_AUX    = 0xA8;
    constexpr uint8_t DISABLE_KBD   = 0xAD;
    constexpr uint8_t ENABLE_KBD    = 0xAE;
    constexpr uint8_t SELF_TEST     = 0xAA;

    // Responses
    constexpr uint8_t SELF_TEST_PASS    = 0x55;

    // Status Bits
    constexpr uint8_t OUT_FULL      = 0x01;
    constexpr uint8_t IN_FULL       = 0x02;

    // CCB
    constexpr uint8_t ENABLE_P1_INT = 0x01;
    constexpr uint8_t ENABLE_P2_INT = 0x02;

    class keyboard_device : public devices::device {
    public:
        keyboard_device(const char* name)
            :devices::device(devices::device_type::legacy_hid, name)
        {
            flags = fs::FS_NODE_CHARDEVICE;
            _dirent.set_name(name);
            _dirent.flags = flags;
            _dirent.node = this;
            _device_name = "PS/2 Keyboard Device";
        }

        ssize_t read(size_t offset, size_t size, uint8_t* buffer) override {
            size = kstd::min(size, (size_t)_count);
            if(size == 0) {
                return 0;
            }

            uint8_t i;
            for(; i < size; i++) {
                if(!read_key(buffer++)) {
                    break;
                }
            }

            return i;
        }

        ssize_t write(size_t offset, size_t size, uint8_t* buffer) override {
            size = kstd::min(size, 256 - (size_t)_count);
            if(size == 0) {
                return 0;
            }

            for(uint8_t i = 0; i < size; i++) {
                _buffer[_end++] = *buffer++;
                _count++;
            }

            return size;
        }

    private:
        bool read_key(uint8_t* key) {
            if(_count == 0) {
                return false;
            }

            _count--;
            *key = _buffer[_start++];
            return true;
        }

        uint8_t _start {0};
        uint8_t _end {0};
        uint8_t _count {0};
        uint8_t _buffer[256];
        fs::directory_entry _dirent;
    };

    static keyboard_device* ps2_kbd;

    static uint8_t kbd_read() {
        const int max_attempts = 10000;
        int i = 0;
        while(i++ < max_attempts) {
            uint8_t s = port_read_8(STATUS_PORT);
            if(s & OUT_FULL) {
                return port_read_8(DATA_PORT);
            }
        }

        const char* reasons[1] = {"PS/2 read failure"};
        kernel_panic(reasons, 1);
    }

    static void ps2_kbd_handler(void*, register_context* regs) {
        uint8_t sc = kbd_read();
        ps2_kbd->write(0, 1, &sc);
    }

    void ps2_initialize() {
        // Properly test to see that we have a PS/2 keyboard

        // Disable both PS/2 ports from receiving external data
        port_write_8(CMD_PORT, DISABLE_KBD);
        port_write_8(CMD_PORT, DISABLE_AUX);

        // Clear any current input
        port_read_8(DATA_PORT);

        port_write_8(CMD_PORT, SELF_TEST);
        uint8_t r = kbd_read();
        if(r != SELF_TEST_PASS) {
            log::error("PS/2 keyboard self-test failed");
            return;
        }

        // Ensure keyboard PS/2 port interrupt enabled
        port_write_8(CMD_PORT, READ_CCB);
        uint8_t ccb = kbd_read();
        ccb |= ENABLE_P1_INT;
        port_write_8(CMD_PORT, WRITE_CCB);
        port_write_8(DATA_PORT, ccb);

        port_write_8(CMD_PORT, ENABLE_KBD);
        port_write_8(CMD_PORT, ENABLE_AUX);

        ps2_kbd = new keyboard_device("kbd0");

        idt::register_interrupt_handler(IRQ0 + 1, ps2_kbd_handler);
        apic::io::map_legacy_irq(1);
        port_write_8(0xF0, 1);
    }
}