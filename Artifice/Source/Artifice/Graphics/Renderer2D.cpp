#include "Renderer2D.h"

#include <array>

#include "Artifice/Core/Application.h"
#include "RenderHandle.h"
#include "Shader.h"
#include "Material.h"

#include "Device.h"

#include "ResourceCaches.h"


struct QuadVertex
{
    vec3 Position;
    vec4 Color;
    vec2 TexCoord;
    float TexIndex;
    float TilingFactor;
};

struct LineVertex
{
    vec3 Position;
    vec4 Color;
};

struct PushConstant
{
    mat4 ViewProjection;
};

struct Renderer2DData
{
    static const uint32 MaxQuads = 20000;
    static const uint32 MaxVertices = MaxQuads * 4;
    static const uint32 MaxIndices = MaxQuads * 6;
    static const uint32 MaxTextureSlots = 16;
    
    SamplerCache SamplerCache;

    RenderHandle QuadVertexBuffer;
    RenderHandle QuadIndexBuffer;
    RenderHandle WhiteTexture;
    RenderHandle WhiteSampler;


    uint32 QuadIndexCount = 0;

    RenderHandle TextureSlots[MaxTextureSlots];
    uint32 TextureSlotIndex = 1;

    ShaderSystem QuadTextureSystem;
    ShaderInstance QuadShaderInstance;

    vec4 QuadVertexPositions[4];

    QuadVertex* QuadVertexBufferBase;
    QuadVertex* QuadVertexBufferPtr;

    // Lines
    static const uint32_t MaxLines = 10000;
    static const uint32_t MaxLineVertices = MaxLines * 2;
    static const uint32_t MaxLineIndices = MaxLines * 2;

    RenderHandle LineVertexBuffer;
    RenderHandle LineIndexBuffer;

    uint32_t LineIndexCount = 0;
    LineVertex* LineVertexBufferBase = nullptr;
    LineVertex* LineVertexBufferPtr = nullptr;


    ShaderInstance LineShaderInstance;

    // General

    mat4 ViewProjection;
    RenderPassLayout RenderPassLayout;

    ShaderLibrary ShaderLibrary;
    CommandBuffer* CommandBuffer;
    Device* Device;

    uint32 CurrentFrame;

    Renderer2D::Statistics Stats;
};

static Renderer2DData s_Data;


void Renderer2D::Init(Device* device)
{
    AR_PROFILE_FUNCTION();

    s_Data.Device = device;
    s_Data.ShaderLibrary = ShaderLibrary(device);

    LoadShaders();
    
    {
        BufferInfo vb;
        vb.Access = MemoryAccessType::Cpu; // Probably should be CpuToGpu, requires invalidate/flush
        vb.BindFlags = RenderBindFlags::VertexBuffer;
        vb.Size = s_Data.MaxVertices * sizeof(QuadVertex) * AR_FRAME_COUNT;
        BufferInfo ib;
        ib.Access = MemoryAccessType::Cpu; // Probably should be CpuToGpu, requires invalidate/flush
        ib.BindFlags = RenderBindFlags::IndexBuffer;
        ib.Size = s_Data.MaxIndices * sizeof(uint32);

        s_Data.QuadVertexBuffer = device->CreateBuffer(vb);
        s_Data.QuadIndexBuffer = device->CreateBuffer(ib);
        
        s_Data.QuadVertexBufferBase = (QuadVertex*)device->MapBuffer(s_Data.QuadVertexBuffer);

    

        uint32* quad_indices = new uint32[s_Data.MaxIndices];

        uint32 offset = 0;
        for (uint32 i = 0; i < s_Data.MaxIndices; i += 6)
        {
            quad_indices[i + 0] = offset + 0;
            quad_indices[i + 1] = offset + 1;
            quad_indices[i + 2] = offset + 2;

            quad_indices[i + 3] = offset + 2;
            quad_indices[i + 4] = offset + 3;
            quad_indices[i + 5] = offset + 0;

            offset += 4;
        }

        void* ib_ptr = device->MapBuffer(s_Data.QuadIndexBuffer);
        memcpy(ib_ptr, quad_indices, ib.Size);
        device->UnmapBuffer(s_Data.QuadIndexBuffer);
        delete[] quad_indices;
    }

    s_Data.QuadVertexPositions[0] = {-0.5f, -0.5f, 0.0f, 1.0f};
    s_Data.QuadVertexPositions[1] = {0.5f, -0.5f, 0.0f, 1.0f};
    s_Data.QuadVertexPositions[2] = {0.5f, 0.5f, 0.0f, 1.0f};
    s_Data.QuadVertexPositions[3] = {-0.5f, 0.5f, 0.0f, 1.0f};

    SamplerInfo sampler;
    sampler.Filter = VK_FILTER_NEAREST;
    sampler.AddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler.MipMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler.Mips = 1;

    s_Data.WhiteSampler = device->CreateSampler(sampler);

    s_Data.SamplerCache.Init(Application::Get()->GetRenderBackend()->GetDevice());

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

    for (uint32 i = 0; i < s_Data.MaxTextureSlots; i++)
    {
        s_Data.TextureSlots[i] = s_Data.WhiteTexture;
    }

    {
        ShaderSystemLayout layout;
        layout.RegisterCombinedTextureSampler(0, s_Data.MaxTextureSlots);

        s_Data.QuadTextureSystem = ShaderSystem(layout, s_Data.ShaderLibrary.Get("Renderer2D"), 0);
    }
    {
        ShaderPipelineState state;
        state.VertexLayout = {
            {
                Format::RGB32Float,
                Format::RGBA32Float,
                Format::RG32Float,
                Format::R32Float,
                Format::R32Float,
            },
        };
        state.DepthTestEnabled = true;
        state.DepthWriteEnabled = true;
        state.BlendEnabled = true;

        s_Data.QuadShaderInstance = ShaderInstance(s_Data.ShaderLibrary.Get("Renderer2D"), state);
    }


    // LINES

    {
        BufferInfo vb;
        vb.Access = MemoryAccessType::Cpu; // Probably should be CpuToGpu, requires invalidate/flush
        vb.BindFlags = RenderBindFlags::VertexBuffer;
        vb.Size = s_Data.MaxLineVertices * sizeof(LineVertex) * AR_FRAME_COUNT;
        BufferInfo ib;
        ib.Access = MemoryAccessType::Cpu; // Probably should be CpuToGpu, requires invalidate/flush
        ib.BindFlags = RenderBindFlags::IndexBuffer;
        ib.Size = s_Data.MaxLineIndices * sizeof(uint32);

        s_Data.LineVertexBuffer = device->CreateBuffer(vb);
        s_Data.LineIndexBuffer = device->CreateBuffer(ib);

        s_Data.LineVertexBufferBase = (LineVertex*)device->MapBuffer(s_Data.LineVertexBuffer);

        uint32* line_indices = new uint32[Renderer2DData::MaxLineIndices];
        for (uint32 i = 0; i < Renderer2DData::MaxLineIndices; i++)
            line_indices[i] = i;

        void* ib_ptr = device->MapBuffer(s_Data.LineIndexBuffer);
        memcpy(ib_ptr, line_indices, ib.Size);
        device->UnmapBuffer(s_Data.LineIndexBuffer);
        delete[] line_indices;

        {
            ShaderPipelineState state;
            state.VertexLayout = {
                {
                    Format::RGB32Float,
                    Format::RGBA32Float,
                },
            };
            state.DepthTestEnabled = true;
            state.DepthWriteEnabled = true;
            state.BlendEnabled = true;
            state.PrimitiveType = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

            s_Data.LineShaderInstance = ShaderInstance(s_Data.ShaderLibrary.Get("Renderer2D"), state);
        }
    }
}

void Renderer2D::Shutdown()
{
    AR_PROFILE_FUNCTION();

    s_Data.Device->DestroyBuffer(s_Data.QuadVertexBuffer);
    s_Data.Device->DestroyBuffer(s_Data.QuadIndexBuffer);
    s_Data.Device->DestroyTexture(s_Data.WhiteTexture);
    s_Data.Device->DestroySampler(s_Data.WhiteSampler);
    s_Data.QuadShaderInstance.CleanUp();
    s_Data.QuadTextureSystem.Shutdown();
    s_Data.SamplerCache.Reset();

    s_Data.Device->DestroyBuffer(s_Data.LineVertexBuffer);
    s_Data.Device->DestroyBuffer(s_Data.LineIndexBuffer);
    s_Data.LineShaderInstance.CleanUp();

    s_Data.ShaderLibrary.CleanUp();

    s_Data = Renderer2DData();
}

void Renderer2D::BeginScene(const mat4& view_projection, const RenderPassLayout& render_pass_layout, CommandBuffer* cmd)
{
    AR_PROFILE_FUNCTION();

    s_Data.CommandBuffer = cmd;
    s_Data.CurrentFrame = s_Data.Device->GetFrameIndex();

    s_Data.ViewProjection = view_projection;
    s_Data.RenderPassLayout = render_pass_layout;

    s_Data.QuadIndexCount = 0;
    s_Data.QuadVertexBufferPtr = s_Data.QuadVertexBufferBase + s_Data.CurrentFrame * s_Data.MaxVertices;

    s_Data.TextureSlotIndex = 1;

    s_Data.LineIndexCount = 0;
    s_Data.LineVertexBufferPtr = s_Data.LineVertexBufferBase + s_Data.CurrentFrame * s_Data.MaxLineVertices;
}

void Renderer2D::EndScene()
{
    Flush();
    FlushLines();
}


void Renderer2D::Flush()
{
    AR_PROFILE_FUNCTION();

    if (s_Data.QuadIndexCount == 0)
        return; // Nothing to draw

    ScopedAllocator alloc;

    // Draw
    s_Data.CommandBuffer->BindGraphicsPipeline(s_Data.QuadShaderInstance.RequestPassPipeline(s_Data.RenderPassLayout));
    s_Data.CommandBuffer->BindVertexBuffers({s_Data.QuadVertexBuffer}, {s_Data.CurrentFrame * s_Data.MaxVertices * sizeof(QuadVertex)});
    s_Data.CommandBuffer->BindIndexBuffer(s_Data.QuadIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

    PushConstant pc = {s_Data.ViewProjection};

    s_Data.CommandBuffer->PushConstants(ShaderStage::Vertex, sizeof(PushConstant), 0, &pc);

    RenderHandle* textures = alloc.Allocate<RenderHandle>(s_Data.MaxTextureSlots);
    RenderHandle* samplers = alloc.Allocate<RenderHandle>(s_Data.MaxTextureSlots);
    for (uint32 i = 0; i < s_Data.MaxTextureSlots; i++)
    {
        SamplerInfo sampler;
        sampler.Filter = VK_FILTER_NEAREST;
        sampler.AddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler.MipMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sampler.Mips = s_Data.Device->GetTexture(s_Data.TextureSlots[i])->Mips;

        textures[i] = s_Data.TextureSlots[i];
        samplers[i] = s_Data.SamplerCache.Request(sampler);
    }
    s_Data.QuadTextureSystem.SetCombinedTextureSampler(0, textures, samplers);

    s_Data.QuadTextureSystem.Bind(s_Data.CommandBuffer);

    s_Data.CommandBuffer->DrawIndexed(s_Data.QuadIndexCount, 1, 0, 0, 0);

    s_Data.Stats.DrawCalls++;

    s_Data.QuadIndexCount = 0;
    s_Data.QuadVertexBufferPtr = s_Data.QuadVertexBufferBase;

    s_Data.TextureSlotIndex = 1;
}

void Renderer2D::FlushLines()
{
    AR_PROFILE_FUNCTION();

    if (s_Data.LineIndexCount == 0)
        return; // Nothing to draw

    // Draw
    s_Data.CommandBuffer->BindGraphicsPipeline(s_Data.LineShaderInstance.RequestPassPipeline(s_Data.RenderPassLayout));
    s_Data.CommandBuffer->BindVertexBuffers({s_Data.LineVertexBuffer}, {s_Data.CurrentFrame * s_Data.MaxLineVertices * sizeof(LineVertex)});
    s_Data.CommandBuffer->BindIndexBuffer(s_Data.LineIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

    PushConstant pc = {s_Data.ViewProjection};

    s_Data.CommandBuffer->PushConstants(ShaderStage::Vertex, sizeof(PushConstant), 0, &pc);

    s_Data.CommandBuffer->DrawIndexed(s_Data.LineIndexCount, 1, 0, 0, 0);

    s_Data.Stats.DrawCalls++;

    s_Data.LineIndexCount = 0;
    s_Data.LineVertexBufferPtr = s_Data.LineVertexBufferBase;
}

void Renderer2D::DrawLine(const vec3& p0, const vec3& p1, const vec4& color)
{
    AR_PROFILE_FUNCTION();

    if (s_Data.LineIndexCount >= s_Data.MaxLineIndices)
        FlushLines();

    s_Data.LineVertexBufferPtr->Position = p0;
    s_Data.LineVertexBufferPtr->Color = color;
    s_Data.LineVertexBufferPtr++;

    s_Data.LineVertexBufferPtr->Position = p1;
    s_Data.LineVertexBufferPtr->Color = color;
    s_Data.LineVertexBufferPtr++;

    s_Data.LineIndexCount += 2;
    s_Data.Stats.LineCount++;
}

void Renderer2D::DrawQuad(const vec3& position, const vec2& size, const vec4& color)
{
    AR_PROFILE_FUNCTION();

    uint32 quad_vertex_count = 4;
    float tex_index = 0.0f;
    float tiling_factor = 1.0f;
    vec2 tex_coords[] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};

    if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
    {
        Flush();
    }

    mat4 transform = mat4::translation(position) * mat4::scale({size.x, size.y, 1.0f});

    for (uint32 i = 0; i < quad_vertex_count; i++)
    {
        vec4 pos = transform * s_Data.QuadVertexPositions[i];
        s_Data.QuadVertexBufferPtr->Position = vec3(pos.x, pos.y, pos.z);
        s_Data.QuadVertexBufferPtr->Color = color;
        s_Data.QuadVertexBufferPtr->TexCoord = tex_coords[i];
        s_Data.QuadVertexBufferPtr->TexIndex = tex_index;
        s_Data.QuadVertexBufferPtr->TilingFactor = tiling_factor;
        s_Data.QuadVertexBufferPtr++;
    }

    s_Data.QuadIndexCount += 6;

    s_Data.Stats.QuadCount++;
}

void Renderer2D::DrawQuad(const vec3& position, const vec2& size, RenderHandle texture, float tiling_factor)
{
    AR_PROFILE_FUNCTION();

    uint32 quad_vertex_count = 4;
    vec2 tex_coords[] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};

    if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
    {
        Flush();
    }

    float tex_index = 0.0f;
    for (uint32 i = 1; i < s_Data.TextureSlotIndex; i++)
    {
        if (s_Data.TextureSlots[i] == texture)
        {
            tex_index = (float)i;
            break;
        }
    }

    if (tex_index == 0.0f)
    {
        if (s_Data.TextureSlotIndex >= Renderer2DData::MaxTextureSlots)
            Flush();

        tex_index = (float)s_Data.TextureSlotIndex;
        s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
        s_Data.TextureSlotIndex++;
    }

    mat4 transform = mat4::translation(position) * mat4::scale({size.x, size.y, 1.0f});

    for (uint32 i = 0; i < quad_vertex_count; i++)
    {
        vec4 pos = transform * s_Data.QuadVertexPositions[i];
        s_Data.QuadVertexBufferPtr->Position = vec3(pos.x, pos.y, pos.z);
        s_Data.QuadVertexBufferPtr->Color = vec4(1.0f, 1.0f, 1.0f, 1.0f); // TODO: tint color
        s_Data.QuadVertexBufferPtr->TexCoord = tex_coords[i];
        s_Data.QuadVertexBufferPtr->TexIndex = tex_index;
        s_Data.QuadVertexBufferPtr->TilingFactor = tiling_factor;
        s_Data.QuadVertexBufferPtr++;
    }

    s_Data.QuadIndexCount += 6;

    s_Data.Stats.QuadCount++;
}

void Renderer2D::ResetStats()
{
    memset(&s_Data.Stats, 0, sizeof(Statistics));
}

Renderer2D::Statistics Renderer2D::GetStats()
{
    return s_Data.Stats;
}

void Renderer2D::LoadShaders()
{
    AR_PROFILE_FUNCTION();

    // Quads
    {
        Shader::Layout layout;
        layout.DescriptorSetLayoutCount = 1;
        layout.DescriptorSetLayouts[0] = {
            {0, DescriptorType::CombinedImageSampler, s_Data.MaxTextureSlots, ShaderStage::Fragment}};
        layout.PushConstantRangeCount = 1;
        layout.PushConstantRanges[0] = {ShaderStage::Vertex, sizeof(PushConstant), 0};

        s_Data.ShaderLibrary.Load("Renderer2D", "Shaders/Renderer2D.vert.spv", "Shaders/Renderer2D.frag.spv", layout);
    }
    // Lines
    {
        Shader::Layout layout;
        layout.DescriptorSetLayoutCount = 0;
        layout.PushConstantRangeCount = 1;
        layout.PushConstantRanges[0] = {ShaderStage::Vertex, sizeof(PushConstant), 0};

        s_Data.ShaderLibrary.Load("Line", "Shaders/Line.vert.spv", "Shaders/Line.frag.spv", layout);
    }
}