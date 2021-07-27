#pragma once

#include "Event.h"

#include <sstream>

//abstract
class KeyEvent : public Event
{
protected:
    int m_KeyCode;
protected:
    KeyEvent(int keycode)
    : m_KeyCode(keycode) {}
public:
    inline int GetKeyCode() const { return m_KeyCode; }
    
    inline virtual int GetCategoryFlags() const override { return (EventCategoryInput | EventCategoryKeyboard); }
};

class KeyPressedEvent : public KeyEvent
{
private:
    int m_RepeatCount;
public:
    KeyPressedEvent(int keycode, int repeatCount)
    : KeyEvent(keycode), m_RepeatCount(repeatCount) {}
    
    inline int GetRepeatCount() const { return m_RepeatCount; }
    
    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "KeyPressedEvent: " << m_KeyCode << " (" << m_RepeatCount << " repeats)";
        return ss.str();
    }
    
    inline static EventType GetStaticType()  { return EventType::KeyReleased; }
    inline virtual EventType GetEventType() const override { return GetStaticType(); }
};

class KeyReleasedEvent : public KeyEvent
{
public:
    KeyReleasedEvent(int keycode)
    : KeyEvent(keycode) {}
    
    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "KeyReleasedEvent: " << m_KeyCode;
        return ss.str();
    }
    
    inline static EventType GetStaticType()  { return EventType::KeyPressed; }
    inline virtual EventType GetEventType() const override { return GetStaticType(); }
};
