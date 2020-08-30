#pragma once

#include <vector>
#include <stddef.h>

#include "Shader.h"
#include "RenderHandle.h"
#include "Resources.h"

#include "Artifice/math/math.h"

#include "Artifice/Core/Core.h"

#include "ShaderSystem.h"


class PBRMaterialSystem : public ShaderSystem
{
private:
    struct PushData
    {
        vec4 Albedo;
        float Metalness;
        float Roughness;

        uint32 AlbedoTexToggle;
        uint32 NormalTexToggle;
        uint32 MetalnessTexToggle;
        uint32 RoughnessTexToggle;
    };

public:
    PBRMaterialSystem() = default;
    PBRMaterialSystem(const std::string& shader_name)
    {
        ShaderSystemLayout layout;
        layout.RegisterCombinedTextureSampler(0);
        layout.RegisterCombinedTextureSampler(1);
        layout.RegisterCombinedTextureSampler(2);
        layout.RegisterCombinedTextureSampler(3);
        layout.RegisterPushConstants<PushData>(ShaderStage::VertexFragment);

        Reset(layout, Renderer::GetShaderLibrary()->Get(shader_name), 1);
    }
    void SetAlbedoTexture(RenderHandle texture, RenderHandle sampler)
    {
        SetCombinedTextureSampler(0, texture, sampler);
    }
    void SetNormalTexture(RenderHandle texture, RenderHandle sampler)
    {
        SetCombinedTextureSampler(1, texture, sampler);
    }
    void SetMetalnessTexture(RenderHandle texture, RenderHandle sampler)
    {
        SetCombinedTextureSampler(2, texture, sampler);
    }
    void SetRoughnessTexture(RenderHandle texture, RenderHandle sampler)
    {
        SetCombinedTextureSampler(3, texture, sampler);
    }
    void SetAlbedo(vec3 albedo)
    {
        SetPushConstants<vec4>(vec4(albedo, 0.0f), offsetof(struct PushData, Albedo));
    }
    void SetMetalness(float metalness)
    {
        SetPushConstants<float>(metalness, offsetof(struct PushData, Metalness));
    }
    void SetRoughness(float roughness)
    {
        SetPushConstants<float>(roughness, offsetof(struct PushData, Roughness));
    }
    void SetAlbedoTexToggle(bool use_texture)
    {
        SetPushConstants<uint32>(use_texture ? 1 : 0, offsetof(struct PushData, AlbedoTexToggle));
    }
    void SetNormalTexToggle(bool use_texture)
    {
        SetPushConstants<uint32>(use_texture ? 1 : 0, offsetof(struct PushData, NormalTexToggle));
    }
    void SetMetalnessTexToggle(bool use_texture)
    {
        SetPushConstants<uint32>(use_texture ? 1 : 0, offsetof(struct PushData, MetalnessTexToggle));
    }
    void SetRoughnessTexToggle(bool use_texture)
    {
        SetPushConstants<uint32>(use_texture ? 1 : 0, offsetof(struct PushData, RoughnessTexToggle));
    }
};