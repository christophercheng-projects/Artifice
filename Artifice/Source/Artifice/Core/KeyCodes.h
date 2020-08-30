#pragma once

#include <iostream>

#include "Artifice/Core/Core.h"


enum class KeyCode : uint16
{
    // From glfw3.h
    Space               = 32,
    Apostrophe          = 39, /* ' */
    Comma               = 44, /* , */
    Minus               = 45, /* - */
    Period              = 46, /* . */
    Slash               = 47, /* / */

    D0                  = 48, /* 0 */
    D1                  = 49, /* 1 */
    D2                  = 50, /* 2 */
    D3                  = 51, /* 3 */
    D4                  = 52, /* 4 */
    D5                  = 53, /* 5 */
    D6                  = 54, /* 6 */
    D7                  = 55, /* 7 */
    D8                  = 56, /* 8 */
    D9                  = 57, /* 9 */

    Semicolon           = 59, /* ; */
    Equal               = 61, /* = */

    A                   = 65,
    B                   = 66,
    C                   = 67,
    D                   = 68,
    E                   = 69,
    F                   = 70,
    G                   = 71,
    H                   = 72,
    I                   = 73,
    J                   = 74,
    K                   = 75,
    L                   = 76,
    M                   = 77,
    N                   = 78,
    O                   = 79,
    P                   = 80,
    Q                   = 81,
    R                   = 82,
    S                   = 83,
    T                   = 84,
    U                   = 85,
    V                   = 86,
    W                   = 87,
    X                   = 88,
    Y                   = 89,
    Z                   = 90,

    LeftBracket         = 91,  /* [ */
    Backslash           = 92,  /* \ */
    RightBracket        = 93,  /* ] */
    GraveAccent         = 96,  /* ` */

    World1              = 161, /* non-US #1 */
    World2              = 162, /* non-US #2 */

    /* Function keys */
    Escape              = 256,
    Enter               = 257,
    Tab                 = 258,
    Backspace           = 259,
    Insert              = 260,
    Delete              = 261,
    Right               = 262,
    Left                = 263,
    Down                = 264,
    Up                  = 265,
    PageUp              = 266,
    PageDown            = 267,
    Home                = 268,
    End                 = 269,
    CapsLock            = 280,
    ScrollLock          = 281,
    NumLock             = 282,
    PrintScreen         = 283,
    Pause               = 284,
    F1                  = 290,
    F2                  = 291,
    F3                  = 292,
    F4                  = 293,
    F5                  = 294,
    F6                  = 295,
    F7                  = 296,
    F8                  = 297,
    F9                  = 298,
    F10                 = 299,
    F11                 = 300,
    F12                 = 301,
    F13                 = 302,
    F14                 = 303,
    F15                 = 304,
    F16                 = 305,
    F17                 = 306,
    F18                 = 307,
    F19                 = 308,
    F20                 = 309,
    F21                 = 310,
    F22                 = 311,
    F23                 = 312,
    F24                 = 313,
    F25                 = 314,

    /* Keypad */
    KP0                 = 320,
    KP1                 = 321,
    KP2                 = 322,
    KP3                 = 323,
    KP4                 = 324,
    KP5                 = 325,
    KP6                 = 326,
    KP7                 = 327,
    KP8                 = 328,
    KP9                 = 329,
    KPDecimal           = 330,
    KPDivide            = 331,
    KPMultiply          = 332,
    KPSubtract          = 333,
    KPAdd               = 334,
    KPEnter             = 335,
    KPEqual             = 336,

    LeftShift           = 340,
    LeftControl         = 341,
    LeftAlt             = 342,
    LeftSuper           = 343,
    RightShift          = 344,
    RightControl        = 345,
    RightAlt            = 346,
    RightSuper          = 347,
    Menu                = 348
};

inline std::ostream& operator<<(std::ostream& os, KeyCode keyCode)
{
    os << static_cast<int32_t>(keyCode);
    return os;
}

// From glfw3.h
#define AR_KEY_SPACE           KeyCode::Space
#define AR_KEY_APOSTROPHE      KeyCode::Apostrophe    /* ' */
#define AR_KEY_COMMA           KeyCode::Comma         /* , */
#define AR_KEY_MINUS           KeyCode::Minus         /* - */
#define AR_KEY_PERIOD          KeyCode::Period        /* . */
#define AR_KEY_SLASH           KeyCode::Slash         /* / */
#define AR_KEY_0               KeyCode::D0
#define AR_KEY_1               KeyCode::D1
#define AR_KEY_2               KeyCode::D2
#define AR_KEY_3               KeyCode::D3
#define AR_KEY_4               KeyCode::D4
#define AR_KEY_5               KeyCode::D5
#define AR_KEY_6               KeyCode::D6
#define AR_KEY_7               KeyCode::D7
#define AR_KEY_8               KeyCode::D8
#define AR_KEY_9               KeyCode::D9
#define AR_KEY_SEMICOLON       KeyCode::Semicolon     /* ; */
#define AR_KEY_EQUAL           KeyCode::Equal         /* = */
#define AR_KEY_A               KeyCode::A
#define AR_KEY_B               KeyCode::B
#define AR_KEY_C               KeyCode::C
#define AR_KEY_D               KeyCode::D
#define AR_KEY_E               KeyCode::E
#define AR_KEY_F               KeyCode::F
#define AR_KEY_G               KeyCode::G
#define AR_KEY_H               KeyCode::H
#define AR_KEY_I               KeyCode::I
#define AR_KEY_J               KeyCode::J
#define AR_KEY_K               KeyCode::K
#define AR_KEY_L               KeyCode::L
#define AR_KEY_M               KeyCode::M
#define AR_KEY_N               KeyCode::N
#define AR_KEY_O               KeyCode::O
#define AR_KEY_P               KeyCode::P
#define AR_KEY_Q               KeyCode::Q
#define AR_KEY_R               KeyCode::R
#define AR_KEY_S               KeyCode::S
#define AR_KEY_T               KeyCode::T
#define AR_KEY_U               KeyCode::U
#define AR_KEY_V               KeyCode::V
#define AR_KEY_W               KeyCode::W
#define AR_KEY_X               KeyCode::X
#define AR_KEY_Y               KeyCode::Y
#define AR_KEY_Z               KeyCode::Z
#define AR_KEY_LEFT_BRACKET    KeyCode::LeftBracket   /* [ */
#define AR_KEY_BACKSLASH       KeyCode::Backslash     /* \ */
#define AR_KEY_RIGHT_BRACKET   KeyCode::RightBracket  /* ] */
#define AR_KEY_GRAVE_ACCENT    KeyCode::GraveAccent   /* ` */
#define AR_KEY_WORLD_1         KeyCode::World1        /* non-US #1 */
#define AR_KEY_WORLD_2         KeyCode::World2        /* non-US #2 */

/* Function keys */
#define AR_KEY_ESCAPE          KeyCode::Escape
#define AR_KEY_ENTER           KeyCode::Enter
#define AR_KEY_TAB             KeyCode::Tab
#define AR_KEY_BACKSPACE       KeyCode::Backspace
#define AR_KEY_INSERT          KeyCode::Insert
#define AR_KEY_DELETE          KeyCode::Delete
#define AR_KEY_RIGHT           KeyCode::Right
#define AR_KEY_LEFT            KeyCode::Left
#define AR_KEY_DOWN            KeyCode::Down
#define AR_KEY_UP              KeyCode::Up
#define AR_KEY_PAGE_UP         KeyCode::PageUp
#define AR_KEY_PAGE_DOWN       KeyCode::PageDown
#define AR_KEY_HOME            KeyCode::Home
#define AR_KEY_END             KeyCode::End
#define AR_KEY_CAPS_LOCK       KeyCode::CapsLock
#define AR_KEY_SCROLL_LOCK     KeyCode::ScrollLock
#define AR_KEY_NUM_LOCK        KeyCode::NumLock
#define AR_KEY_PRINT_SCREEN    KeyCode::PrintScreen
#define AR_KEY_PAUSE           KeyCode::Pause
#define AR_KEY_F1              KeyCode::F1
#define AR_KEY_F2              KeyCode::F2
#define AR_KEY_F3              KeyCode::F3
#define AR_KEY_F4              KeyCode::F4
#define AR_KEY_F5              KeyCode::F5
#define AR_KEY_F6              KeyCode::F6
#define AR_KEY_F7              KeyCode::F7
#define AR_KEY_F8              KeyCode::F8
#define AR_KEY_F9              KeyCode::F9
#define AR_KEY_F10             KeyCode::F10
#define AR_KEY_F11             KeyCode::F11
#define AR_KEY_F12             KeyCode::F12
#define AR_KEY_F13             KeyCode::F13
#define AR_KEY_F14             KeyCode::F14
#define AR_KEY_F15             KeyCode::F15
#define AR_KEY_F16             KeyCode::F16
#define AR_KEY_F17             KeyCode::F17
#define AR_KEY_F18             KeyCode::F18
#define AR_KEY_F19             KeyCode::F19
#define AR_KEY_F20             KeyCode::F20
#define AR_KEY_F21             KeyCode::F21
#define AR_KEY_F22             KeyCode::F22
#define AR_KEY_F23             KeyCode::F23
#define AR_KEY_F24             KeyCode::F24
#define AR_KEY_F25             KeyCode::F25

/* Keypad */
#define AR_KEY_KP_0            KeyCode::KP0
#define AR_KEY_KP_1            KeyCode::KP1
#define AR_KEY_KP_2            KeyCode::KP2
#define AR_KEY_KP_3            KeyCode::KP3
#define AR_KEY_KP_4            KeyCode::KP4
#define AR_KEY_KP_5            KeyCode::KP5
#define AR_KEY_KP_6            KeyCode::KP6
#define AR_KEY_KP_7            KeyCode::KP7
#define AR_KEY_KP_8            KeyCode::KP8
#define AR_KEY_KP_9            KeyCode::KP9
#define AR_KEY_KP_DECIMAL      KeyCode::KPDecimal
#define AR_KEY_KP_DIVIDE       KeyCode::KPDivide
#define AR_KEY_KP_MULTIPLY     KeyCode::KPMultiply
#define AR_KEY_KP_SUBTRACT     KeyCode::KPSubtract
#define AR_KEY_KP_ADD          KeyCode::KPAdd
#define AR_KEY_KP_ENTER        KeyCode::KPEnter
#define AR_KEY_KP_EQUAL        KeyCode::KPEqual

#define AR_KEY_LEFT_SHIFT      KeyCode::LeftShift
#define AR_KEY_LEFT_CONTROL    KeyCode::LeftControl
#define AR_KEY_LEFT_ALT        KeyCode::LeftAlt
#define AR_KEY_LEFT_SUPER      KeyCode::LeftSuper
#define AR_KEY_RIGHT_SHIFT     KeyCode::RightShift
#define AR_KEY_RIGHT_CONTROL   KeyCode::RightControl
#define AR_KEY_RIGHT_ALT       KeyCode::RightAlt
#define AR_KEY_RIGHT_SUPER     KeyCode::RightSuper
#define AR_KEY_MENU            KeyCode::Menu