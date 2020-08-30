#include "CameraController.h"

#include "Artifice/Core/Input.h"
#include "Artifice/Debug/Instrumentor.h"
#include "Artifice/math/math_functions.h"


OrthographicCameraController::OrthographicCameraController(float aspect_ratio, bool rotation)
    : m_AspectRatio(aspect_ratio), m_Camera(-m_AspectRatio * m_ZoomLevel, m_AspectRatio * m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel), m_Rotation(rotation)
{
    m_Camera.SetProjection(-m_AspectRatio * m_ZoomLevel, m_AspectRatio * m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel);
}

void OrthographicCameraController::OnUpdate(Timestep ts)
{
    AR_PROFILE_FUNCTION();

    if (Input::IsKeyPressed(AR_KEY_A))
    {
        m_CameraPosition.x -= math::cos(math::toRadians(m_CameraRotation)) * m_CameraTranslationSpeed * ts;
        m_CameraPosition.y -= math::sin(math::toRadians(m_CameraRotation)) * m_CameraTranslationSpeed * ts;
    }
    else if (Input::IsKeyPressed(AR_KEY_D))
    {
        m_CameraPosition.x += math::cos(math::toRadians(m_CameraRotation)) * m_CameraTranslationSpeed * ts;
        m_CameraPosition.y += math::sin(math::toRadians(m_CameraRotation)) * m_CameraTranslationSpeed * ts;
    }

    if (Input::IsKeyPressed(AR_KEY_W))
    {
        m_CameraPosition.x += -math::sin(math::toRadians(m_CameraRotation)) * m_CameraTranslationSpeed * ts;
        m_CameraPosition.y += math::cos(math::toRadians(m_CameraRotation)) * m_CameraTranslationSpeed * ts;
    }
    else if (Input::IsKeyPressed(AR_KEY_S))
    {
        m_CameraPosition.x -= -math::sin(math::toRadians(m_CameraRotation)) * m_CameraTranslationSpeed * ts;
        m_CameraPosition.y -= math::cos(math::toRadians(m_CameraRotation)) * m_CameraTranslationSpeed * ts;
    }

    if (m_Rotation)
    {
        if (Input::IsKeyPressed(AR_KEY_Q))
            m_CameraRotation += m_CameraRotationSpeed * ts;
        if (Input::IsKeyPressed(AR_KEY_E))
            m_CameraRotation -= m_CameraRotationSpeed * ts;

        if (m_CameraRotation > 180.0f)
            m_CameraRotation -= 360.0f;
        else if (m_CameraRotation <= -180.0f)
            m_CameraRotation += 360.0f;

        m_Camera.SetRotation(m_CameraRotation);
    }

    m_Camera.SetPosition(m_CameraPosition);

    m_CameraTranslationSpeed = m_ZoomLevel;
}

void OrthographicCameraController::SetAspectRatio(float aspect_ratio)
{
    m_AspectRatio = aspect_ratio;
    m_Camera.SetProjection(-m_AspectRatio * m_ZoomLevel, m_AspectRatio * m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel);
}
void OrthographicCameraController::SetShouldIgnoreWindowEvents(bool ignore)
{
    m_ShouldIgnoreWindowEvents = ignore;
}

void OrthographicCameraController::OnEvent(Event& e)
{
    AR_PROFILE_FUNCTION();

    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<MouseScrolledEvent>([this](MouseScrolledEvent& e) -> bool { return this->OnMouseScrolled(e); });

    if (!m_ShouldIgnoreWindowEvents)
    {
        dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& e) -> bool { return this->OnWindowResize(e); });
    }
}

bool OrthographicCameraController::OnMouseScrolled(MouseScrolledEvent& e)
{
    AR_PROFILE_FUNCTION();

    m_ZoomLevel += e.GetYOffset() * 0.25f;
    m_ZoomLevel = math::max(m_ZoomLevel, 0.25f);
    m_Camera.SetProjection(-m_AspectRatio * m_ZoomLevel, m_AspectRatio * m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel);
    return false;
}

bool OrthographicCameraController::OnWindowResize(WindowResizeEvent& e)
{
    AR_PROFILE_FUNCTION();
     
    m_AspectRatio = (float)e.GetWidth() / (float)e.GetHeight();
    m_Camera.SetProjection(-m_AspectRatio * m_ZoomLevel, m_AspectRatio * m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel);

    return false;
}


FPSCameraController::FPSCameraController(float aspect_ratio, float fov, float near, float far)
    : m_AspectRatio(aspect_ratio), m_FOV(fov), m_Near(near), m_Far(far), m_Camera(aspect_ratio, fov, near, far)
{
    m_Camera.SetProjection(m_AspectRatio, m_FOV, m_Near, m_Far);
}

void FPSCameraController::OnUpdate(Timestep ts)
{
    AR_PROFILE_FUNCTION();

    if (!Input::IsCursorDisabled())
    {
        return;
    }

    // Position

    vec3 forward = -1.0f * vec3(-math::cos(math::toRadians(-m_CameraRotation.y)) * math::sin(math::toRadians(-m_CameraRotation.x)), 0.0f, math::cos(math::toRadians(-m_CameraRotation.y)) * math::cos(math::toRadians(-m_CameraRotation.x))).Normalize();

    vec3 up(0.0f, 1.0f, 0.0f);
    vec3 right = forward.Cross(up).Normalize();

    if (Input::IsKeyPressed(AR_KEY_A))
        m_CameraPosition -= right * m_CameraTranslationSpeed * ts;
    else if (Input::IsKeyPressed(AR_KEY_D))
        m_CameraPosition += right * m_CameraTranslationSpeed * ts;

    if (Input::IsKeyPressed(AR_KEY_W))
        m_CameraPosition += forward * m_CameraTranslationSpeed * ts;
    else if (Input::IsKeyPressed(AR_KEY_S))
        m_CameraPosition -= forward * m_CameraTranslationSpeed * ts;

    if (Input::IsKeyPressed(AR_KEY_SPACE))
        m_CameraPosition += up * m_CameraTranslationSpeed * ts;
    else if (Input::IsKeyPressed(AR_KEY_LEFT_SHIFT))
        m_CameraPosition -= up * m_CameraTranslationSpeed * ts;

    // Rotation

    vec2 mouse_pos = { (float)Input::GetMouseX(), (float)Input::GetMouseY() };
    vec2 mouse_delta = mouse_pos - m_LastMousePosition;
    m_LastMousePosition = mouse_pos;

    m_CameraRotation -= mouse_delta * m_CameraSensitivity;

    if (m_CameraRotation.x > 180.0f) m_CameraRotation.x -= 360.0f;
    else if (m_CameraRotation.x <= -180.0f) m_CameraRotation.x += 360.0f;

    if (m_CameraRotation.y > 89.0f) m_CameraRotation.y = 89.0f;
    else if (m_CameraRotation.y < -89.0f) m_CameraRotation.y = -89.0f;

    m_Camera.SetRotation(m_CameraRotation);
    m_Camera.SetPosition(m_CameraPosition);
}

void FPSCameraController::OnEvent(Event &e)
{
    AR_PROFILE_FUNCTION();

    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<MouseScrolledEvent>([this](MouseScrolledEvent& e) -> bool { return this->OnMouseScrolled(e); });
    dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& e) -> bool { return this->OnWindowResize(e); });
}

bool FPSCameraController::OnMouseScrolled(MouseScrolledEvent &e)
{
    AR_PROFILE_FUNCTION();

    m_CameraTranslationSpeed -= e.GetYOffset() * 0.25f;
    m_CameraTranslationSpeed = math::max(m_CameraTranslationSpeed, 0.25f);

    return false;
}

bool FPSCameraController::OnWindowResize(WindowResizeEvent &e)
{
    AR_PROFILE_FUNCTION();

    m_AspectRatio = (float)e.GetWidth() / (float)e.GetHeight();
    m_Camera.SetProjection(m_AspectRatio, m_FOV, m_Near, m_Far);

    return false;
}
