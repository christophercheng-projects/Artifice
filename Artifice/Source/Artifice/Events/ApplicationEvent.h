#pragma once

#include "Event.h"

#include <sstream>


class WindowResizeEvent : public Event
{
private:
    unsigned int m_Width, m_Height;
public:
    WindowResizeEvent(unsigned int width, unsigned int height)
    : m_Width(width), m_Height(height) {}
    
    inline unsigned int GetWidth() const { return m_Width; }
    inline unsigned int GetHeight() const { return m_Height; }
    
    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "WindowResizeEvent: " << m_Width << ", " << m_Height;
        return ss.str();
    }
    
    inline virtual int GetCategoryFlags() const override { return (EventCategoryApplication); }
    inline static EventType GetStaticType() { return EventType::WindowResize; }
    inline virtual EventType GetEventType() const override { return GetStaticType(); }
};

class  WindowCloseEvent : public Event
{
public:
    WindowCloseEvent() {}
    
    std::string ToString() const override
    {
        std::stringstream ss;
        ss << "WindowClosedEvent";
        return ss.str();
    }
    
    inline virtual int GetCategoryFlags() const override { return (EventCategoryApplication); }
    inline static EventType GetStaticType() { return EventType::WindowClose; }
    inline virtual EventType GetEventType() const override { return GetStaticType(); }
};




class AppTickEvent : public Event
{
public:
    AppTickEvent() {}

    inline virtual int GetCategoryFlags() const override { return (EventCategoryApplication); }
    inline static EventType GetStaticType() { return EventType::AppTick; }
    inline virtual EventType GetEventType() const override { return GetStaticType(); }
};

class  AppUpdateEvent : public Event
{
public:
    AppUpdateEvent() {}
    
    inline virtual int GetCategoryFlags() const override { return (EventCategoryApplication); }
    inline static EventType GetStaticType() { return EventType::AppUpdate; }
    inline virtual EventType GetEventType() const override { return GetStaticType(); }
};

class AppRenderEvent : public Event
{
public:
    AppRenderEvent() {}
    
    inline virtual int GetCategoryFlags() const override { return (EventCategoryApplication); }
    inline static EventType GetStaticType() { return EventType::AppRender; }
    inline virtual EventType GetEventType() const override { return GetStaticType(); }
    };
