#include "Window.h"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <iostream>

#include "Artifice/Events/ApplicationEvent.h"
#include "Artifice/Events/KeyEvent.h"
#include "Artifice/Events/MouseEvent.h"

#include "Artifice/Core/Log.h"


void Window::PlatformInit()
{
    if (!glfwInit())
    {
        AR_CORE_FATAL("Failed to initialize GLFW.");
    }

    if (!glfwVulkanSupported())
    {
        AR_CORE_FATAL("vulkan not supported");
    }

    glfwSetErrorCallback(glfw_error_callback);
}

void Window::PlatformCleanUp()
{
    glfwTerminate();
}

void Window::PollEvents()
{
    glfwPollEvents();
}


Window::Window()
: m_Resized(false), m_CursorDisabled(false)
{

}

bool Window::Init(const std::string& title, int width, int height)
{
    m_Title = title;
    m_Width = width;
    m_Height = height;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_Handle = glfwCreateWindow(m_Width / 2, m_Height / 2, m_Title.c_str(), nullptr, nullptr);
    
    glfwSetWindowUserPointer(m_Handle, this);
    glfwSetFramebufferSizeCallback(m_Handle, framebuffer_size_callback);
    glfwSetWindowCloseCallback(m_Handle, window_close_callback);
    glfwSetCursorPosCallback(m_Handle, cursor_position_callback);
    glfwSetKeyCallback(m_Handle, key_callback);
    glfwSetMouseButtonCallback(m_Handle, mouse_button_callback);
    glfwSetScrollCallback(m_Handle, scroll_callback);

    return true;
}

void Window::CleanUp()
{
    glfwDestroyWindow(m_Handle);
}

void Window::OnUpdate()
{
    glfwPollEvents();
}

bool Window::ShouldClose()
{
    return glfwWindowShouldClose(m_Handle);
}


//STATIC FUNCTION CALLBACKS
void Window::glfw_error_callback(int error, const char* description)
{
    AR_CORE_ERROR("%s", description);
}

void Window::framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    Window* win = (Window*)glfwGetWindowUserPointer(window);

    win->m_Width = width;
    win->m_Height = height;
    win->m_Resized = true;

    WindowResizeEvent e(width, height);
    win->m_EventCallback(e);
}

void Window::window_close_callback(GLFWwindow* window)
{
    Window* win = (Window*)glfwGetWindowUserPointer(window);

    WindowCloseEvent e;
    win->m_EventCallback(e);
}

void Window::cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    Window* win = (Window*) glfwGetWindowUserPointer(window);
    win->m_MouseX = xpos;
    win->m_MouseY = ypos;

    MouseMovedEvent e((float)xpos, (float)ypos);
    win->m_EventCallback(e);
}

void Window::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    Window* win = (Window*) glfwGetWindowUserPointer(window);

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        win->ToggleCursor();
    }

    switch(action)
    {
        case GLFW_PRESS:
        {
            KeyPressedEvent e(key, 0);
            win->m_EventCallback(e);
            break;
        }
        case GLFW_RELEASE:
        {
            KeyReleasedEvent e(key);
            win->m_EventCallback(e);
            break;
        }
        case GLFW_REPEAT:
        {
            KeyPressedEvent e(key, 1);
            win->m_EventCallback(e);
            break;
        }
    }



    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
    }

    win->m_Keys[key] = action != GLFW_RELEASE;

}

void Window::mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    Window* win = (Window*) glfwGetWindowUserPointer(window);

    switch (action)
    {
        case GLFW_PRESS:
        {
            MouseButtonPressedEvent e(button);
            win->m_EventCallback(e);
            break;
        }
        case GLFW_RELEASE:
        {
            MouseButtonReleasedEvent e(button);
            win->m_EventCallback(e);
            break;
        }
    }

}

void Window::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    Window* win = (Window*) glfwGetWindowUserPointer(window);

    MouseScrolledEvent e((float)xoffset, (float)yoffset);
    win->m_EventCallback(e);
}
