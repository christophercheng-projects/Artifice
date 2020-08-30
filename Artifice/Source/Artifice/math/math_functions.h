#pragma once

#define _USE_MATH_DEFINES
#include <math.h>


namespace math {
    
    inline float toRadians(float degrees)
    {
        return (float) degrees * M_PI / 180.0f;
    }
    
    inline float sqrt(float value)
    {
        return ::sqrt(value);
    }
    
    inline float sin(float value)
    {
        return ::sin(value);
    }
    
    inline float cos(float value)
    {
        return ::cos(value);
    }
    
    template<typename T>
    inline T min(T value1, T value2)
    {
        return (value1 < value2) ? value1 : value2;
    }
    
    template<typename T>
    inline T max(T value1, T value2)
    {
        return (value1 > value2) ? value1 : value2;
    }

    template<typename T>
    inline T clamp(T value, T min, T max)
    {
        assert(!(max < min));
        return (value < min) ? min : (max < value) ? max : value;
    }
}
