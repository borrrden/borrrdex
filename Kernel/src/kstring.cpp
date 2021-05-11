#include <kstring.h>
#include <stdint.h>
#include <kassert.h>
#include <limits.h>
#include <string.h>
#include <liballoc/liballoc.h>

static void reverse(char* str, size_t length) {
    char* end = str + length - 1;
    for(size_t i = 0; i < length / 2; i++) {
        char c = *end;
        *end-- = *str++;
        *str = c;
    }
}

extern "C" {
    void* memset(void* dst, int c, size_t n) {
        uint8_t* current = (uint8_t *)dst;
        for(uint64_t i = 0; i < n; i++) {
            *current++ = c;
        }

        return dst;
    }

    void* memcpy(void* __restrict dest, const void* __restrict src, size_t n) {
        if(!((uintptr_t)dest & (sizeof(intptr_t) - 1)) && !((uintptr_t)src & sizeof(intptr_t) - 1)) {
            uintptr_t* d = (uintptr_t *)dest;
            uintptr_t* s = (uintptr_t *)src;
            while(n >= sizeof(intptr_t)) {
                *d++ = *s++;
                n -= sizeof(intptr_t);
            }

            dest = d;
            src = s;
        }

        uint8_t* d = (uint8_t *)dest;
        uint8_t* s = (uint8_t *)src;
        while(n--) {
            *d++ = *s++;
        }

        return dest;
    }

    __attribute__((pure)) int memcmp(const void* aptr, const void* bptr, size_t n) {
        uint8_t* a = (uint8_t *)aptr;
        uint8_t* b = (uint8_t *)bptr;
        for(size_t i = 0; i < n; i++) {
            if(*a != *b) {
                return *a - *b;
            }

            a++;
            b++;
        }

        return 0;
    }

    int strcmp(const char *a, const char *b) {
        size_t i = 0;
        while(true) {
            unsigned char a_byte = a[i];
            unsigned char b_byte = b[i];
            if(!a_byte && !b_byte)
                return 0;
            // If only one char is null, one of the following cases applies.
            if(a_byte < b_byte)
                return -1;
            if(a_byte > b_byte)
                return 1;
            i++;
        }
    }

    int strncmp(const char *a, const char *b, size_t max) {
        size_t i = 0;
        while(max--) {
            unsigned char a_byte = a[i];
            unsigned char b_byte = b[i];
            if(!a_byte && !b_byte)
                return 0;
            // If only one char is null, one of the following cases applies.
            if(a_byte < b_byte)
                return -1;
            if(a_byte > b_byte)
                return 1;
            i++;
        }

        return 0;
    }

    constexpr size_t ALIGN = sizeof(size_t) - 1;
    constexpr size_t ONES = (size_t)-1/UCHAR_MAX;
    constexpr size_t HIGHS = ONES * (UCHAR_MAX/2+1);
    constexpr size_t HASZERO(size_t x) { return (x)-ONES & ~(x) & HIGHS; }

    char* strncpy(char* dst, const char* src, size_t max) {
        size_t *wd;
        const size_t *ws;

        if (((uintptr_t)src & ALIGN) == ((uintptr_t)dst & ALIGN)) {
            for (; ((uintptr_t)src & ALIGN) && max && (*dst=*src); max--, src++, dst++);
            if (!max || !*src) goto tail;
            wd=(size_t *)dst; ws=(const size_t *)src;
            for (; max>=sizeof(size_t) && !HASZERO(*ws);
                max-=sizeof(size_t), ws++, wd++) *wd = *ws;
            dst=(char *)wd; src=(const char *)ws;
        }
        for (; max && (*dst=*src); max--, src++, dst++);
    tail:
        memset(dst, 0, max);
        return dst;
    }

    size_t strnlen(const char *str, size_t maxlen) {
        size_t i = 0;
        while(str[i] && maxlen--) {
            i++;
        }

        return i;
    }

    char *strchr(const char *s, int c) {
        size_t i = 0;
        while(s[i]) {
            if(s[i] == c)
                return const_cast<char *>(&s[i]);
            i++;
        }
        if(c == 0)
            return const_cast<char *>(&s[i]);
        return nullptr;
    }

    char *strtok_r(char *__restrict s, const char *__restrict del, char **__restrict m) {
        assert(m != nullptr);

        // We use *m = null to memorize that the entire string was consumed.
        char *tok;
        if(s) {
            tok = s;
        }else if(*m) {
            tok = *m;
        }else {
            return nullptr;
        }

        // Skip initial delimiters.
        // After this loop: *tok is non-null iff we return a token.
        while(*tok && strchr(del, *tok))
            tok++;

        // Replace the following delimiter by a null-terminator.
        // After this loop: *p is null iff we reached the end of the string.
        auto p = tok;
        while(*p && !strchr(del, *p))
            p++;

        if(*p) {
            *p = 0;
            *m = p + 1;
        }else{
            *m = nullptr;
        }
        if(p == tok)
            return nullptr;
        return tok;
    }

    char *strtok(char *__restrict s, const char *__restrict delimiter) {
        static char *saved;
        return strtok_r(s, delimiter, &saved);
    }

    char* strdup(const char* s) {
        auto num_bytes = strnlen(s, 1024);

        char *new_string = (char *)malloc(num_bytes + 1);
        if(!new_string) // TODO: set errno
            return nullptr;

        memcpy(new_string, s, num_bytes);
        new_string[num_bytes] = 0;
        return new_string;
    }

    char* itoa(unsigned long long num, char* str, int base) {
        int i = 0;
        if(num == 0) {
            str[i++] = 0;
            str[i] = 0;
            return str;
        }

        while(num != 0) {
            int rem = num % base;
            str[i++] =  (rem > 9) ? (rem - 10) + 'a' : rem + '0';
            num = num / base;
        }

        str[i] = 0;
        reverse(str, i);
        return str;
    }

    int hex_to_pointer(const char* buffer, size_t buffer_size, uintptr_t& ptr) {
        size_t n = 0;
        ptr = 0;

        while(*buffer && n++ < buffer_size) {
            char c = *buffer++;
            ptr <<= 4;
            if(c >= '0' && c <= '9') {
                ptr |= (c - '0') & 0xF;
            } else if(c >= 'a' && c <= 'f') {
                ptr |= (c - 'a' + 0xA) & 0xF;
            } else if(c >= 'A' && c <= 'F') {
                ptr |= (c - 'A' + 0xA) & 0xF;
            } else {
                return 1;
            }
        }

        return 0;
    }
}