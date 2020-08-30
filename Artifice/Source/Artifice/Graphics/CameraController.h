#pragma once

#include "Artifice/Core/Timestep.h"
#include "Artifice/Graphics/Camera.h"
#include "Artifice/math/vec3.h"
#include "Artifice/Events/ApplicationEvent.h"
#include "Artifice/Events/MouseEvent.h"


class OrthographicCameraController
{
private:
    OrthographicCamera m_Camera;
    float m_AspectRatio;
    float m_ZoomLevel = 1.0f;

    bool m_Rotation;

    vec3 m_CameraPosition = { 0.0f, 0.0f, 0.0f };
    float m_CameraRotation = 0.0f; //In degrees, in the anti-clockwise direct
    float m_CameraTranslationSpeed = 5.0f;
    float m_CameraRotationSpeed = 180.0f;

    bool m_ShouldIgnoreWindowEvents = false;

public:
    OrthographicCameraController() = default;
    OrthographicCameraController(float aspect_ratio, bool rotation = false);

    void OnUpdate(Timestep ts);
    void OnEvent(Event& e);

    OrthographicCamera& GetCamera() { return m_Camera; }
    const OrthographicCamera& GetCamera() const { return m_Camera; }

    float GetZoomLevel() const { return m_ZoomLevel; }
    void SetZoomLevel(float level) { m_ZoomLevel = level; }
    void SetAspectRatio(float aspect_ratio);
    void SetShouldIgnoreWindowEvents(bool ignore);
private:
    bool OnMouseScrolled(MouseScrolledEvent& e);
    bool OnWindowResize(WindowResizeEvent& e);
};


class FPSCameraController
{
private:
    PerspectiveCamera m_Camera;
    float m_AspectRatio;
    float m_FOV;
    float m_Near;
    float m_Far;

    vec2 m_LastMousePosition = { 0.0f, 0.0f };

    vec3 m_CameraPosition = { 0.0f, 0.0f, 0.0f };
    vec2 m_CameraRotation = { 0.0f, 0.0f }; //In degrees, x: yaw, y: pitch
    float m_CameraTranslationSpeed = 5.0f;
    float m_CameraSensitivity = 0.25f;
public:
    FPSCameraController(float aspect_ratio, float fov, float near, float fear);

    void OnUpdate(Timestep ts);
    void OnEvent(Event& e);

    PerspectiveCamera &GetCamera() { return m_Camera; }
    const PerspectiveCamera &GetCamera() const { return m_Camera; }

    float GetSensitivity() const { return m_CameraSensitivity; }
    void SetSensitivity(float sensitivity) { m_CameraSensitivity = sensitivity; }
    float GetSpeed() const { return m_CameraTranslationSpeed; }
    void SetSpeed(float speed) { m_CameraTranslationSpeed = speed; }
    float GetFOV() const { return m_FOV; }
    void SetFOV(float fov) { m_FOV = fov; }
private:
    bool OnMouseScrolled(MouseScrolledEvent &e);
    bool OnWindowResize(WindowResizeEvent &e);
};