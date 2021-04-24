#include "string.h"
#include <stdint.h>
#include <limits.h>

extern "C" {

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

    void memset(void* dst, int c, size_t n) {
        uint8_t* current = (uint8_t *)dst;
        for(uint64_t i = 0; i < n; i++) {
            *current++ = c;
        }
    }

    int strncmp(const char *_l, const char *_r, size_t n)
    {
        const unsigned char *l=(const unsigned char *)_l, *r=(const unsigned char *)_r;
        if (!n--) return 0;
        for (; *l && *r && n && *l == *r ; l++, r++, n--);
        return *l - *r;
    }

    constexpr size_t ALIGN = sizeof(size_t) - 1;
    constexpr size_t ONES = (size_t)-1/UCHAR_MAX;
    constexpr size_t HIGHS = ONES * (UCHAR_MAX/2+1);
    constexpr size_t HASZERO(size_t x) { return (x)-ONES & ~(x) & HIGHS; }

    char *strncpy (char *__restrict d, const char *__restrict s, size_t n) {
        size_t *wd;
        const size_t *ws;

        if (((uintptr_t)s & ALIGN) == ((uintptr_t)d & ALIGN)) {
            for (; ((uintptr_t)s & ALIGN) && n && (*d=*s); n--, s++, d++);
            if (!n || !*s) goto tail;
            wd=(size_t *)d; ws=(const size_t *)s;
            for (; n>=sizeof(size_t) && !HASZERO(*ws);
                n-=sizeof(size_t), ws++, wd++) *wd = *ws;
            d=(char *)wd; s=(const char *)ws;
        }
        for (; n && (*d=*s); n--, s++, d++);
    tail:
        memset(d, 0, n);
        return d;
    }

}