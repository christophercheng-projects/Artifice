#include "Camera.h"

#include <iostream>
#include "../math/math_functions.h"
#include "Artifice/Core/Input.h"
#include "Artifice/Core/Application.h"
#include "Artifice/Debug/Instrumentor.h"

OrthographicCamera::OrthographicCamera(float left, float right, float bottom, float top) : m_ProjectionMatrix(mat4::orthographic(left, right, bottom, top, -1.0f, 1.0f)), m_ViewMatrix(1.0f)
{
    AR_PROFILE_FUNCTION();

    m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
}

void OrthographicCamera::SetProjection(float left, float right, float bottom, float top)
{
    AR_PROFILE_FUNCTION();

    m_ProjectionMatrix = mat4::orthographic(left, right, bottom, top, -1.0f, 1.0f);
    m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
}

void OrthographicCamera::RecalculateViewMatrix()
{
    AR_PROFILE_FUNCTION();

    mat4 transform = mat4::translation(m_Position) * mat4::rotation(m_Rotation, vec3(0, 0, 1));

    m_ViewMatrix = transform.inverse();
    m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
}

PerspectiveCamera::PerspectiveCamera(float aspect_ratio, float fov, float near, float far)
 : m_ProjectionMatrix(mat4::perspective(aspect_ratio, fov, near, far)), m_ViewMatrix(1.0f)
{
    AR_PROFILE_FUNCTION();

    m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
}

void PerspectiveCamera::SetProjection(float aspect_ratio, float fov, float near, float far)
{
    AR_PROFILE_FUNCTION();

    m_ProjectionMatrix = mat4::perspective(aspect_ratio, fov, near, far);
    m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
}

void PerspectiveCamera::RecalculateViewMatrix()
{
    AR_PROFILE_FUNCTION();
    mat4 transform = mat4::translation(m_Position) * mat4::rotation(m_Rotation.x, vec3(0, 1, 0)) * mat4::rotation(m_Rotation.y, vec3(1, 0, 0));

    m_ViewMatrix = transform.inverse();
    m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
}


Camera::Camera()
: m_MovementSpeed(10.5f), m_Sensitivity(0.08f), m_Yaw(0.0f), m_Pitch(0.0f), m_Position(vec3(0.0f, 0.0f, 0.0f)), m_InitialMouseX(0.0), m_InitialMouseY(0.0)
{

}

void Camera::Init()
{
    m_Window = Application::Get()->GetWindow();
    m_InitialMouseX = m_Window->GetWidth() / 2.0f;
    m_InitialMouseY = m_Window->GetHeight() / 2.0f;
}

void Camera::UpdateView(float factor)
{
    if (!m_Window->IsCursorDisabled())
    {
        return;
    }

    vec3 position(m_PositionLast);

    if (m_PositionLast != m_Position)
    {
        position = vec3(m_Position * factor + m_PositionLast * (1.0f - factor)); // LERP
    }

    double x = Input::GetMouseX();
    double y = Input::GetMouseY();

    float dx = x - m_InitialMouseX;
    float dy = y - m_InitialMouseY;

    m_Yaw += dx * m_Sensitivity;
    m_Pitch += dy * m_Sensitivity;
    if (m_Pitch > 89.0f)
    {
        m_Pitch = 89.0f;
    }
    else if (m_Pitch < -89.0f)
    {
        m_Pitch = -89.0f;
    }

    while (m_Yaw >= 360.0f)
    {
        m_Yaw -= 360.0f;
    }
    while (m_Yaw <= 0.0f)
    {
        m_Yaw += 360.0f;
    }

    m_InitialMouseX = x;
    m_InitialMouseY = y;

    mat4 r = mat4::rotation(m_Pitch, vec3(1.0f, 0.0f, 0.0f)) * mat4::rotation(m_Yaw, vec3(0.0f, 1.0f, 0.0f));
    mat4 t = mat4::translation(position);

    m_ViewRotation = r;
    m_View = r * t;
}

void Camera::Update(float delta)
{

    if (!m_Window->IsCursorDisabled())
    {
        return;
    }

    m_PositionLast = m_Position;

    vec3 forward =
    vec3(
         -math::cos(math::toRadians(m_Pitch)) * math::sin(math::toRadians(m_Yaw)),
         0.0f,
         math::cos(math::toRadians(m_Pitch)) * math::cos(math::toRadians(m_Yaw))
         ).Normalize();

    vec3 up(0.0f, 1.0f, 0.0f);
    vec3 right = forward.Cross(up).Normalize();

    float speed = m_MovementSpeed * 0.01f;

    if (Input::IsKeyPressed(AR_KEY_LEFT_SHIFT))
    {
        speed *= 6.0f;
    }
    else if (Input::IsKeyPressed(AR_KEY_LEFT_CONTROL))
    {
        speed *= 0.25f;
    }

    if (Input::IsKeyPressed(AR_KEY_W))
    {
        m_Position += forward * speed;
    }
    else if (Input::IsKeyPressed(AR_KEY_S))
    {
        m_Position -= forward * speed;
    }
    if (Input::IsKeyPressed(AR_KEY_A))
    {
        m_Position -= right * speed;
    }
    else if (Input::IsKeyPressed(AR_KEY_D))
    {
        m_Position += right * speed;
    }


    if (Input::IsKeyPressed(AR_KEY_SPACE))
    {
        m_Position -= up * speed;
    }
    else if (Input::IsKeyPressed(AR_KEY_LEFT_ALT))
    {
        m_Position += up * speed;
    }

}
