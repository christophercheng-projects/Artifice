#pragma once

#include <iostream>

#include "Artifice/Core/Core.h"

enum class MouseCode : uint16
{
    // From glfw3.h
    Button0                = 0,
    Button1                = 1,
    Button2                = 2,
    Button3                = 3,
    Button4                = 4,
    Button5                = 5,
    Button6                = 6,
    Button7                = 7,

    ButtonLast             = Button7,
    ButtonLeft             = Button0,
    ButtonRight            = Button1,
    ButtonMiddle           = Button2
};

inline std::ostream& operator<<(std::ostream& os, MouseCode mouseCode)
{
    os << (int)mouseCode;
    return os;
}

#define AR_MOUSE_BUTTON_0      MouseCode::Button0
#define AR_MOUSE_BUTTON_1      MouseCode::Button1
#define AR_MOUSE_BUTTON_2      MouseCode::Button2
#define AR_MOUSE_BUTTON_3      MouseCode::Button3
#define AR_MOUSE_BUTTON_4      MouseCode::Button4
#define AR_MOUSE_BUTTON_5      MouseCode::Button5
#define AR_MOUSE_BUTTON_6      MouseCode::Button6
#define AR_MOUSE_BUTTON_7      MouseCode::Button7
#define AR_MOUSE_BUTTON_LAST   MouseCode::ButtonLast
#define AR_MOUSE_BUTTON_LEFT   MouseCode::ButtonLeft
#define AR_MOUSE_BUTTON_RIGHT  MouseCode::ButtonRight
#define AR_MOUSE_BUTTON_MIDDLE MouseCode::ButtonMiddle