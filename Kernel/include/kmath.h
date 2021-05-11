#pragma once

#include <type_traits>

namespace kstd {
    template<typename T,
        typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
    T min(T x, T y) {
        return (x > y) ? y : x;
    }

    template<typename T,
        typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
    T max(T x, T y) {
        return (x > y) ? x : y;
    }
}