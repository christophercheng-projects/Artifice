#include "Input.h"

#include <GLFW/glfw3.h>

#include "Artifice/Core/Application.h"


bool Input::IsCursorDisabled()
{
    return Application::Get()->GetWindow()->IsCursorDisabled();
}

bool Input::IsKeyPressed(KeyCode key)
{
    GLFWwindow* window = Application::Get()->GetWindow()->GetHandle();
    int state = glfwGetKey(window, (int)key);
    return state == GLFW_PRESS || state == GLFW_REPEAT;
}

bool Input::IsMouseButtonPressed(MouseCode button)
{
    GLFWwindow* window = Application::Get()->GetWindow()->GetHandle();
    int state = glfwGetMouseButton(window, (int)button);
    return state == GLFW_PRESS;
}

std::pair<float, float> Input::GetMousePosition()
{
    GLFWwindow* window = Application::Get()->GetWindow()->GetHandle();
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    return { (float)xpos, (float)ypos };
}

float Input::GetMouseX()
{
    auto[x, y] = GetMousePosition();
    return x;
}

float Input::GetMouseY()
{
    auto[x, y] = GetMousePosition();
    return y;
}