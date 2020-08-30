#pragma once

#include <Artifice.h>

struct DigitsStorageBuffer
{
    int Size = 1600 * 4;
    float Pixels[1600 * 900 * 4];
};
struct DigitsPushConstants
{
    vec4 Color;
    vec2 LastMousePosition;
    vec2 CurrentMousePosition;
    float Size;
    uint32 Clear = false;
};

class SandboxDigits : public Layer
{
private:
    ShaderLibrary m_ShaderLibrary;
    ShaderInstance m_ShaderInstance;
    ShaderSystem m_ShaderSystem;

    RenderHandle m_StorageTexture;
    DigitsStorageBuffer m_Pixels;
    bool m_MouseDown = false;

    DigitsPushConstants m_Options;
    vec2 m_LastMousePosition;

public:
    SandboxDigits();
    virtual ~SandboxDigits() = default;

    virtual void OnAttach() override;
    virtual void OnDetach() override;

    void OnUpdate(Timestep ts) override;
    virtual void OnConstructRenderGraph(RenderGraph* graph) override;
    virtual void OnImGuiRender() override;
    void OnEvent(Event& e) override;

private:
};