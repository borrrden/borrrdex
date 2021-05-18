#pragma once

#define ALWAYS_INLINE __attribute__((always_inline)) inline

namespace std {
    template<typename T>
    ALWAYS_INLINE T&& move(T& t) {
        return static_cast<T&&>(t);
    }
}