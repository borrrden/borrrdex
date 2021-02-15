#include "stdio.h"
#include "string.h"
#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include "proc/syscall.h"

#define LADJUST 	0x0004		/* left adjustment */
#define LONGINT 	0x0010		/* long integer */
#define LLONGINT    0x0020		/* long long integer */
#define SHORTINT 	0x0040		/* short integer */
#define ZEROPAD 	0x0080		/* zero (as opposed to blank) pad */
#define PTRINT   	0x0200		/* (unsigned) ptrdiff_t */
#define SIZEINT 	0x0400		/* (signed) size_t */
#define CHARINT 	0x0800		/* 8 bit integer */
#define MAXINT  	0x1000		/* largest integer size (intmax_t) */
#define TTY         0x0001      /* Is this going to a TTY output? */

#define PADCHAR (flags & ZEROPAD) ? '0' : ' ';

uint32_t _syscall(uint64_t syscall, uint64_t a1, uint64_t a2, uint64_t a3);

inline int do_write(char* buffer, size_t *size, char c, int flags) {
    if(size == 0) {
        return 0;
    }

    if(flags & TTY) {
        putchar(c);
    } else {
        *buffer++ = c;
    }

    *size -= 1;
    return 1;
}

int printn(char* buffer, size_t* size, int flags, int width, const char* str, int n) {
    if(!(flags & LADJUST)) {
        char pad = PADCHAR;
        for(int i = 0; i < (width - n); i++) {
            do_write(buffer, size, pad, flags);
        }
    }

    for(size_t i = 0; i < n; i++) {
        do_write(buffer, size, str[i], flags);
    }

    if((flags & LADJUST)) {
        char pad = PADCHAR;
        for(int i = 0; i < (width - n); i++) {
            do_write(buffer, size, pad, flags);
        }
    }

    int written = width > n ? width : n;
    return written;
}

int print_integer(char* buffer, size_t *size, int flags, int width, uint64_t arg, int base, char hexStart) {
    char num_buffer[23];
    uint8_t index = 22;
    if(arg == 0) {
        do_write(buffer, size, '0', flags);
        return 1;
    }

     while(arg > 0) {
        uint8_t remainder = arg % base;
        arg /= base;
        num_buffer[index--] = remainder > 9 ? (remainder - 10 + hexStart) : remainder + '0';
    }

    index++;
    return printn(buffer, size, flags, width, (const char *)&num_buffer[index], 23 - index);
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

uint32_t add_digit(uint32_t total, uint8_t digit) {
    uint32_t multiplier = 10;
    while(total > 9) {
        total /= 10;
        multiplier += 10;
    }

    total *= multiplier;
    return total + digit;
}

static int vxnprintf(char* buffer, size_t size, const char* fmt, va_list ap, int flags) {
    int width;
    char ch;
    int written = 0;
    while(true) {
        while(*fmt != 0 && *fmt != '%') {
            written += do_write(buffer, &size, *fmt, flags);
        }

        if(*fmt == 0) {
            return written;
        }

        fmt++; // Skip '%'
        flags &= TTY;
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
                    written += do_write(buffer, &size, '-', flags);
                    arg *= -1;
                }

                written += print_integer(buffer, &size, flags, width, (uint64_t)arg, 10, 0);
                break;
            }
            case 'O':
			    flags |= LONGINT;
                // fall through
            case 'o':
            {
                uint64_t arg = unsigned_arg(flags, ap);
                written += print_integer(buffer, &size, flags, width, arg, 8, 0);
                break;
            }
            case 's':
            {
                const char* arg = va_arg(ap, const char*);
                if(!arg) {
                    written += printn(buffer, &size, flags, width, "(null)", 6);
                } else {
                    // Todo %.*s format for specifying length
                    written += printn(buffer, &size, flags, width, arg, strnlen(arg, __UINT32_MAX__));
                }
                break;
            }
            case 'U':
                flags != LONGINT;
                // fall through
            case 'u':
            {
                uint64_t arg = unsigned_arg(flags, ap);
                written += print_integer(buffer, &size, flags, width, arg, 10, 0);
                break;
            }
            case 'X':
            {
                uint64_t arg = unsigned_arg(flags, ap);
                written += print_integer(buffer, &size, flags, width, arg, 16, 'A');
                break;
            }
            case 'x':
            {
                uint64_t arg = unsigned_arg(flags, ap);
                written += print_integer(buffer, &size, flags, width, arg, 16, 'a');
                break;
            }
            case '%':
                written += do_write(buffer, &size, '%', flags);
                break;
            default:
                break;
        }
    }
}

int putchar(char c) {
    _syscall(SYSCALL_WRITE, STDOUT_HANDLE, (uint64_t)&c, 1);
}


int puts(const char* str) {
    _syscall(SYSCALL_WRITE, STDOUT_HANDLE, (uint64_t)str, strnlen(str, UINT32_MAX));
}

int printf(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int written = vxnprintf(NULL, 0x7fffffff, fmt, ap, TTY);
    va_end(ap);
    return written;
}