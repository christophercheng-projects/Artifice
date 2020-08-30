#pragma once

#include <string>
#include <functional>

#include <GLFW/glfw3.h>

#include "Artifice/Core/Core.h"

#include "Artifice/Events/Event.h"


class Window
{
    using EventCallbackFn = std::function<void(Event&)>;
public:
    static void PlatformInit();
    static void PlatformCleanUp();
    static void PollEvents();
private:
    bool m_Keys[1024];

    std::string m_Title;

    GLFWwindow* m_Handle;

    EventCallbackFn m_EventCallback;

    int m_Width;
    int m_Height;

    double m_MouseX;
    double m_MouseY;

    bool m_Resized;
    bool m_CursorDisabled;
public:
    Window();

    bool Init(const std::string& title, int width, int height);
    void CleanUp();

    inline void SetEventCallback(const EventCallbackFn& callback) { m_EventCallback = callback; }

    void OnUpdate();
    bool ShouldClose();

    inline GLFWwindow* GetHandle() const { return m_Handle; }

    void SetSize(uint32 width, uint32 height) { glfwSetWindowSize(m_Handle, width, height); }

    inline int GetWidth() const { return m_Width; }
    inline int GetHeight() const { return m_Height; }
    inline bool IsResized() const { return m_Resized; }
    inline void SetNotResized() { m_Resized = false; }
    inline double GetMouseX() const { return m_MouseX; }
    inline double GetMouseY() const { return m_MouseY; }
    inline bool IsKeyPressed(int keycode) const { return m_Keys[keycode]; }
    inline bool IsCursorDisabled() const { return m_CursorDisabled; }

    inline void DisableCursor() { glfwSetInputMode(m_Handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED); m_CursorDisabled = true; }
    inline void EnableCursor() { glfwSetInputMode(m_Handle, GLFW_CURSOR, GLFW_CURSOR_NORMAL); m_CursorDisabled = false; }
    inline void ToggleCursor() { m_CursorDisabled ? EnableCursor() : DisableCursor(); }

private:
    static void glfw_error_callback(int error, const char* description);

    static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
    static void window_close_callback(GLFWwindow* window);
    static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
};
