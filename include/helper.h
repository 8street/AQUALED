#pragma once
#ifndef HELPER_H
#    define HELPER_H

#    include <algorithm>

template<typename T> T clamp(T value, T min, T max)
{
    if (min > max)
    {
        std::swap(min, max);
    }
    if (value > max)
    {
        value = max;
    }
    else if (value < min)
    {
        value = min;
    }
    return value;
}

template<typename T> T rounding(T value, T out_value, T diff)
{
    if (value - out_value <= diff)
    {
        return out_value;
    }
    return value;
}

// inline constexpr uint8_t operator "" _u8( unsigned long long arg ) noexcept
//{
//     return static_cast< uint8_t >( arg );
// }

#endif