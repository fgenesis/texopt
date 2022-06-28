#pragma once

namespace detail
{
    template <typename T, size_t N>
    char(&_ArraySizeHelper(T(&a)[N]))[N];
}
#define Countof(a) (sizeof(detail::_ArraySizeHelper(a)))

inline unsigned nextPowerOf2(unsigned in)
{
    in -= 1u;
    in |= in >> 16u;
    in |= in >> 8u;
    in |= in >> 4u;
    in |= in >> 2u;
    in |= in >> 1u;
    return in + 1u;
}

