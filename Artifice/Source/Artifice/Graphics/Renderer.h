#pragma once

#include "Artifice/math/math.h"

#include "Artifice/Graphics/Shader.h"
#include "Artifice/Graphics/Device.h"

class Renderer
{
public:
    static void Init(Device* device);
    static void Shutdown();

    static RenderHandle GetWhiteTexture();
    static RenderHandle GetWhiteSampler();
    static RenderHandle GetLinearSampler();

    static ShaderLibrary* GetShaderLibrary();
};

struct PBRPushConstants
{
    vec4 Albedo;
    float Metalness;
    float Roughness;

    uint32 AlbedoTexToggle;
    uint32 NormalTexToggle;
    uint32 MetalnessTexToggle;
    uint32 RoughnessTexToggle;
};