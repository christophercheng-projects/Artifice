#pragma once

#include "Event.h"

#include <sstream>


class MouseMovedEvent : public Event
{
private:
    float m_X, m_Y;
public:
    MouseMovedEvent(float x, float y)
    : m_X(x), m_Y(y) {}
    
    inline float GetX() const { return m_X; }
    inline float GetY() const { return m_Y; }
    
    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "MouseMovedEvent: " << GetX() << ", " << GetY();
        return ss.str();
    }
    
    inline virtual int GetCategoryFlags() const override { return (EventCategoryInput | EventCategoryMouse); }
    inline static EventType GetStaticType() { return EventType::MouseMoved; }
    inline virtual EventType GetEventType() const override { return GetStaticType(); }
};

class MouseScrolledEvent : public Event
{
private:
    float m_XOffset, m_YOffset;
public:
    MouseScrolledEvent(float xOffset, float yOffset)
    : m_XOffset(xOffset), m_YOffset(yOffset) {}
    
    inline float GetXOffset() const { return m_XOffset; }
    inline float GetYOffset() const { return m_YOffset; }
    
    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "MouseScrolledEvent: " << GetXOffset() << ", " << GetYOffset();
        return ss.str();
    }
    
    inline virtual int GetCategoryFlags() const override { return (EventCategoryInput | EventCategoryMouse); }
    inline static EventType GetStaticType() { return EventType::MouseScrolled; }
    inline virtual EventType GetEventType() const override { return GetStaticType(); }
};

class MouseButtonEvent : public Event
{
protected:
    int m_Button;
protected:
    MouseButtonEvent(int button)
    : m_Button(button) {}
public:
    inline int GetMouseButton() const { return m_Button; }
    
    inline virtual int GetCategoryFlags() const override { return (EventCategoryInput | EventCategoryMouse); }
};

class MouseButtonPressedEvent : public MouseButtonEvent
{
public:
    MouseButtonPressedEvent(int button)
    : MouseButtonEvent(button) {}
    
    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "MouseButtonPressedEvent: " << m_Button;
        return ss.str();
    }
    
    inline static EventType GetStaticType() { return EventType::MouseButtonPressed; }
    inline virtual EventType GetEventType() const override { return GetStaticType(); }
};

class MouseButtonReleasedEvent : public MouseButtonEvent
{
public:
    MouseButtonReleasedEvent(int button)
    : MouseButtonEvent(button) {}
    
    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "MouseButtonReleasedEvent: " << m_Button;
        return ss.str();
    }
    
    inline static EventType GetStaticType() { return EventType::MouseButtonReleased; }
    inline virtual EventType GetEventType() const override { return GetStaticType(); }
};
