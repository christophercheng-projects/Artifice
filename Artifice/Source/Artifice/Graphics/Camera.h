#pragma once

#include "Artifice/Core/Window.h"

#include "../math/mat4.h"
#include "../math/vec3.h"
#include "../math/vec2.h"

class OrthographicCamera
{
private:
    mat4 m_ProjectionMatrix;
    mat4 m_ViewMatrix;
    mat4 m_ViewProjectionMatrix;

    vec3 m_Position = { 0.0f, 0.0f, 0.0f };
    float m_Rotation = 0.0f;
public:
    OrthographicCamera() = default;
    OrthographicCamera(float left, float right, float bottom, float top);

    void SetProjection(float left, float right, float bottom, float top);

    const vec3& GetPosition() const { return m_Position; }
    void SetPosition(const vec3& position) { m_Position = position; RecalculateViewMatrix(); }

    float GetRotation() const { return m_Rotation; }
    void SetRotation(float rotation) { m_Rotation = rotation; RecalculateViewMatrix(); }

    const mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
    const mat4& GetViewMatrix() const { return m_ViewMatrix; }
    const mat4& GetViewProjectionMatrix() const { return m_ViewProjectionMatrix; }
private:
    void RecalculateViewMatrix();
};

class PerspectiveCamera
{
private:
    mat4 m_ProjectionMatrix;
    mat4 m_ViewMatrix;
    mat4 m_ViewProjectionMatrix;

    vec3 m_Position = { 0.0f, 0.0f, 0.0f };
    vec2 m_Rotation = { 0.0f, 0.0f };

public:
    PerspectiveCamera() = default;
    PerspectiveCamera(float aspect_ratio, float fov, float near, float far);

    void SetProjection(float aspect_ratio, float fov, float near, float far);

    const vec3& GetPosition() const { return m_Position; }
    void SetPosition(const vec3& position) { m_Position = position; RecalculateViewMatrix(); }

    const vec2& GetRotation() const { return m_Rotation; }
    void SetRotation(const vec2& rotation) { m_Rotation = rotation; RecalculateViewMatrix(); }

    const mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
    const mat4& GetViewMatrix() const { return m_ViewMatrix; }
    mat4 GetViewRotationMatrix() const { return (mat4::rotation(m_Rotation.x, vec3(0, 1, 0)) * mat4::rotation(m_Rotation.y, vec3(1, 0, 0))).inverse(); }
    const mat4& GetViewProjectionMatrix() const { return m_ViewProjectionMatrix; }
private:
    void RecalculateViewMatrix();
};

class Camera
{
private:
    mat4 m_View;
    mat4 m_ViewRotation;

    vec3 m_PositionLast;
    vec3 m_Position;


    Window* m_Window;

    float m_MovementSpeed;

    float m_Sensitivity;

    double m_InitialMouseX;
    double m_InitialMouseY;


    float m_Pitch;
    float m_Yaw;

public:
    Camera();
    void Init();
    void Update(float delta);
    void UpdateView(float factor);

    inline void SetPosition(vec3 position) { m_Position = position; m_PositionLast = position; }

    inline const vec3& GetPosition() const { return m_Position; }
    inline const mat4& GetView() { return m_View; }
    inline const mat4& GetViewRotation() { return m_ViewRotation; }
};
