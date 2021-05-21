#pragma once

#include <type_traits>

namespace kstd {
    template<typename T,
        typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
    inline T min(T x, T y) {
        return (x > y) ? y : x;
    }

    template<typename T,
        typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
    inline T max(T x, T y) {
        return (x > y) ? x : y;
    }

    template<typename T,
        typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
    inline T round_up(T x, T interval) {
        return (x + interval - 1) & ~interval;
    }

    template<typename T,
        typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
    inline T intervals_needed(T x, T interval) {
        return (x + interval - 1) / interval;
    }

    inline void* offset_by(void* start, size_t amount) {
        return (void *)((uintptr_t)start + amount);
    }
}