#include <logging.h>
#include <kstring.h>
#include <serial.h>
#include <device.h>
#include <kmath.h>
#include <idt.h>
#include <kerrno.h>
#include <termios.h>

constexpr uint16_t LADJUST =	0x0004;		/* left adjustment */
constexpr uint16_t LONGINT =	0x0010;		/* long integer */
constexpr uint16_t LLONGINT =   0x0020;		/* long long integer */
constexpr uint16_t SHORTINT =	0x0040;		/* short integer */
constexpr uint16_t ZEROPAD =	0x0080;		/* zero (as opposed to blank) pad */
constexpr uint16_t PTRINT =  	0x0200;		/* (unsigned) ptrdiff_t */
constexpr uint16_t SIZEINT =	0x0400;		/* (signed) size_t */
constexpr uint16_t CHARINT =	0x0800;		/* 8 bit integer */
constexpr uint16_t MAXINT = 	0x1000;		/* largest integer size (intmax_t) */

#define PADCHAR (flags & ZEROPAD) ? '0' : ' ';

namespace log {
    using device_type = devices::device_type;

    class log_device : public devices::device {
    public:
        log_device(const char* name)
            :device(device_type::kernel_log, name)
        {
            flags = fs::FS_NODE_FILE;
        }

        void enable() {
            if(!_log_buffer) {
                _log_buffer = (char *)malloc(MAX_SIZE);

                // Arbitrary magic marker
                _log_buffer[0] = 0;
                _log_buffer[1] = 1;
            }

            _enabled = true;
        }

        void disable() {
            _enabled = false;
        }

        ssize_t read(size_t offset, size_t size, uint8_t* buffer) override {
            if(!_log_buffer) {
                return 0;
            }

            size = kstd::min(MAX_SIZE, size);

            if(_size < MAX_SIZE) {
                size = kstd::min(_size, offset + size);
                memcpy(buffer, _log_buffer + offset, size);
                return size;
            }

            if(_position + offset + size < MAX_SIZE) {
                memcpy(buffer, _log_buffer + _position + offset, size);
                return size;
            }

            size_t remaining = MAX_SIZE - _position + offset + size;
            memcpy(buffer, _log_buffer + _position + offset, remaining);
            memcpy(buffer + remaining, _log_buffer, size - remaining);
            return size;
        }

        ssize_t write(size_t offset, size_t size, uint8_t* buffer) override {
            if(offset != 0) {
                return -EINVAL; // No writing at arbitrary offsets
            }

            if(!_enabled) {
                return 0;
            }

            size = kstd::min(size, MAX_SIZE);
            if(_size == MAX_SIZE) {
                if(_position + size <= MAX_SIZE) {
                    memcpy(_log_buffer + _position, buffer, size);
                    _position += size;
                    return size;
                }

                size_t to_write = MAX_SIZE - _position + size;
                size_t remaining = size - to_write;
                memcpy(_log_buffer + _position, buffer, to_write);
                memcpy(_log_buffer, buffer + to_write, remaining);
                _position = remaining;
                return size;
            }

            if(_size + size <= MAX_SIZE) {
                memcpy(_log_buffer + _size, buffer, size);
                _size += size;
                return size;
            }

            size_t to_write = MAX_SIZE - _size + size;
            size_t remaining = size - to_write;
            memcpy(_log_buffer + _size, buffer, to_write);
            memcpy(_log_buffer, buffer + to_write, remaining);
            _position = remaining;
            _size = MAX_SIZE;
            return size;
        }

        int ioctl(uint64_t cmd, uint64_t arg) override {
            return cmd == TIOCGWINSZ ? 0 : -1;
        }
    private:
        static constexpr size_t MAX_SIZE = 0x100000;

        char* _log_buffer {nullptr};
        size_t _position {0};
        size_t _size {0};
        bool _enabled {false};
    };

    log_device* log_dev = nullptr;

    void write_n(const char* str, size_t n) {
        uart::print_n(str, n);
    }
    
    void print_n(int flags, int width, const char* str, int n) {
        if(!(flags & LADJUST)) {
            char pad = PADCHAR;
            for(int i = 0; i < (width - n); i++) {
                write_n(&pad, 1);
            }
        }

        write_n(str, n);

        if((flags & LADJUST)) {
            char pad = PADCHAR;
            for(int i = 0; i < (width - n); i++) {
                write_n(&pad, 1);
            }
        }
    }

    void print_integer(int flags, int width, uint64_t arg, int base = 10, char hexStart = 'A') {
        char buffer[23];
        uint8_t index = 22;
        if(arg == 0) {
            buffer[index--] = '0';
        }

        while(arg > 0) {
            uint8_t remainder = arg % base;
            arg /= base;
            buffer[index--] = remainder > 9 ? (remainder - 10 + hexStart) : remainder + '0';
        }

        index++;
        print_n(flags, width, (const char *)&buffer[index], 23 - index);
    }

    inline int64_t signed_arg(int flags, va_list ap) {
        if(flags & LLONGINT) {
            return (int64_t)va_arg(ap, long long);
        } 
        
        if(flags & LONGINT) {
            return (int64_t)va_arg(ap, long long);
        } 

        return (int64_t)va_arg(ap, int);
    }

    inline uint64_t unsigned_arg(int flags, va_list ap) {
        if(flags & LLONGINT) {
            return (uint64_t)va_arg(ap, unsigned long long);
        } 
        
        if(flags & LONGINT) {
            return (uint64_t)va_arg(ap, unsigned long long);
        }

        return (uint64_t)va_arg(ap, unsigned int);
    }

    constexpr uint32_t add_digit(uint32_t total, uint8_t digit) {
        uint32_t multiplier = 10;
        while(total > 9) {
            total /= 10;
            multiplier += 10;
        }

        total *= multiplier;
        return total + digit;
    }

    void write_f(const char* __restrict fmt, va_list ap) {
        int flags;
        int width;
        char ch;
        while(true) {
            while(*fmt != 0 && *fmt != '%') {
                write_n(fmt++, 1);
            }

            if(*fmt == 0) {
                return;
            }

            fmt++; // Skip '%'
            flags = 0;
            width = 0;
    rflag:  ch = *fmt++;
    reswitch:switch(ch) {
                case '-':
                    flags |= LADJUST;
                    goto rflag;
                case '0':
                    if(!(flags & ZEROPAD)) {
                        flags |= ZEROPAD;
                        goto rflag;
                    }
                    // fall through
                case '1': case '2': case '3': case '4': 
                case '5': case '6': case '7': case '8': case '9':
                    do {
                        width = add_digit(width, ch - '0');
                        ch = *fmt++;
                    } while(ch <= '0' && ch >= '9');
                    goto reswitch;
                case 'h':
                    if(*fmt == 'h') {
                        fmt++;
                        flags |= CHARINT;
                    } else {
                        flags |= SHORTINT;
                    }
                    goto rflag;
                case 'l':
                    if(*fmt == 'l') {
                        fmt++;
                        flags |= LLONGINT;
                    } else {
                        flags |= LONGINT;
                    }
                    goto rflag;
                case 'q':
                    flags |= LLONGINT;
                    goto rflag;
                case 'z':
                    flags |= SIZEINT;
                    goto rflag;
                case 'D':
                    flags |= LONGINT;
                    // fall through
                case 'd':
                case 'i':
                {
                    int64_t arg = signed_arg(flags, ap);
                    if(arg < 0) {
                        write_n("-", 1);
                        arg *= -1;
                    }

                    print_integer(flags, width, (uint64_t)arg);
                    break;
                }
                case 'O':
                    flags |= LONGINT;
                    // fall through
                case 'o':
                {
                    uint64_t arg = unsigned_arg(flags, ap);
                    print_integer(flags, width, arg, 8);
                    break;
                }
                case 's':
                {
                    const char* arg = va_arg(ap, const char*);
                    if(!arg) {
                        print_n(flags, width, "(null)", 6);
                    } else {
                        // Todo %.*s format for specifying length
                        print_n(flags, width, arg, strnlen(arg, __SIZE_MAX__));
                    }
                    break;
                }
                case 'U':
                    flags |= LONGINT;
                    // fall through
                case 'u':
                {
                    uint64_t arg = unsigned_arg(flags, ap);
                    print_integer(flags, width, arg);
                    break;
                }
                case 'X':
                {
                    uint64_t arg = unsigned_arg(flags, ap);
                    print_integer(flags, width, arg, 16, 'A');
                    break;
                }
                case 'x':
                {
                    uint64_t arg = unsigned_arg(flags, ap);
                    print_integer(flags, width, arg, 16, 'a');
                    break;
                }
                case '%':
                    write_n("%", 1);
                    break;
                default:
                    break;
            }
        }
    }

    void write(const char* str, uint8_t r, uint8_t g, uint8_t b) {
        write_n(str, strnlen(str, 1024));
    }

    void info(const char* __restrict format, ...) {
        write("\r\n[INFO]   ");
        va_list args;
        va_start(args, format);
        write_f(format, args);
        va_end(args);
    }

    void error(const char* __restrict format, ...) {
        write("\r\n[ERROR]  ", 255, 0, 0);
        va_list args;
        va_start(args, format);
        write_f(format, args);
        va_end(args);
    }

    void warning(const char* __restrict format, ...) {
        write("\r\n[WARN]  ", 255, 255, 0);
        va_list args;
        va_start(args, format);
        write_f(format, args);
        va_end(args);
    }

    void late_initialize() {
        log_dev = new log_device("klog");
    }

    void enable_klog() {
        log_dev->enable();
    }

    void disable_klog() {
        log_dev->disable();
    }
}