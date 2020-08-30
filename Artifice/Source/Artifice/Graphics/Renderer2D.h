#pragma once

#include "Artifice/math/math.h"

#include "Artifice/Graphics/Camera.h"

#include "Artifice/Graphics/Resources.h"
#include "Artifice/Graphics/RenderGraph/RenderGraph.h"
#include "Artifice/Graphics/RenderHandle.h"


class Renderer2D
{
public:
    static void Init(Device* device);
    static void Shutdown();


    

    static void Flush();
    static void FlushLines();


    static void BeginScene(const mat4& view_projection, const RenderPassLayout& render_pass_layout, CommandBuffer* cmd);
    static void EndScene();

    static void DrawLine(const vec3& p0, const vec3& p1, const vec4& color = vec4(1.0f));

    static void DrawQuad(const vec3& position, const vec2& size, const vec4& color);
    static void DrawQuad(const vec3& position, const vec2& size, RenderHandle texture, float tiling_factor = 1.0f);

    struct Statistics
    {
        uint32 DrawCalls = 0;
        uint32 QuadCount = 0;
        uint32 LineCount = 0;
        uint32 GetTotalVertexCount() const { return QuadCount * 4 + LineCount * 2; }
        uint32 GetTotalIndexCount() const { return QuadCount * 6 + LineCount * 2; }
    };
    static void ResetStats();
    static Statistics GetStats();
private:
    static void LoadShaders();
};

#if 0

    TextureInfo tex;
    tex.MipsEnabled = false;
    tex.Width = 1;
    tex.Height = 1;
    tex.Format = VK_FORMAT_R8G8B8A8_UNORM;
    tex.Type = TextureType::Texture2D;
    tex.BindFlags = RenderBindFlags::SampledTexture | RenderBindFlags::TransferDestination;

    s_Data.WhiteTexture = device->CreateTexture(tex);
    uint32 tex_data = 0xffffffff;

    CommandBuffer cmd = device->RequestCommandBuffer(QueueType::Universal);
    cmd.Begin();
    cmd.UploadTextureData(s_Data.WhiteTexture, sizeof(uint32), &tex_data, false, ResourceState::SampledFragment());
    cmd.End();
    device->CommitCommandBuffer(cmd);
    device->SubmitQueue(QueueType::Universal, {}, {}, true);



    DescriptorSetUpdate ds1;
    ds1.Add(0, DescriptorType::CombinedImageSampler, 1, 0, {
        {s_Data.WhiteTexture, ResourceState::SampledFragment(), s_Data.WhiteSampler}
    });

    s_Data.TextureMaterialInstance.UpdateDescriptorSet(1, ds1);

#endif