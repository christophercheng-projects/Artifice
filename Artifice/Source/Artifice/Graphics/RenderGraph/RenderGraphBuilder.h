#pragma once

#include <vector>

#include "Artifice/Graphics/Resources.h"
#include "Artifice/Graphics/RenderHandle.h"

#include "Artifice/math/vec4.h"

#include "RenderGraphRegistry.h"

enum class RenderGraphPassType
{
    Render,
    Other
};

struct RenderGraphResourceSnapshot
{
    std::string Name;
    ResourceState State;
};

struct RenderGraphBuilderPass
{
    QueueType Queue = QueueType::Universal;
    RenderGraphPassType Type = RenderGraphPassType::Render;
    
    std::vector<RenderGraphResourceSnapshot> TextureReads;
    std::vector<RenderGraphResourceSnapshot> TextureWrites;
    std::vector<RenderGraphResourceSnapshot> TextureReadWrites;
    std::vector<RenderGraphResourceSnapshot> BufferReads;
    std::vector<RenderGraphResourceSnapshot> BufferWrites;
    std::vector<RenderGraphResourceSnapshot> BufferReadWrites;

    std::vector<RenderGraphResourceSnapshot> TextureCreatesAndImports;
    std::vector<RenderGraphResourceSnapshot> BufferCreatesAndImports;

    std::vector<std::string> ColorAttachments;
    std::vector<vec4> ColorClearColors;
    std::vector<int32> ColorLayers;
    std::vector<std::string> DepthAttachment;
    std::vector<std::string> ResolveAttachments;

    void AddTextureRead(const std::string& name, ResourceState state)
    {
        TextureReads.push_back({name, state});
    }
    void AddTextureWrite(const std::string& name, ResourceState state)
    {
        TextureWrites.push_back({name, state});
    }
    void AddTextureReadWrite(const std::string& name, ResourceState state)
    {
        TextureReadWrites.push_back({name, state});
    }
    void AddBufferRead(const std::string& name, ResourceState state)
    {
        BufferReads.push_back({name, state});
    }
    void AddBufferWrite(const std::string& name, ResourceState state)
    {
        BufferWrites.push_back({name, state});
    }
    void AddBufferReadWrite(const std::string& name, ResourceState state)
    {
        BufferReadWrites.push_back({name, state});
    }
};

class RenderGraphBuilder
{
private:
    Device* m_Device;
    RenderGraphRegistry* m_Registry;
    RenderGraphBuilderPass m_Pass;
    RenderPassInfo m_RenderPassInfo;

public:
    RenderGraphBuilder() = default;
    RenderGraphBuilder(Device* device, RenderGraphRegistry* registry) : m_Device(device), m_Registry(registry) {}
    RenderGraphBuilderPass GetPass() const { return m_Pass; }
    RenderPassInfo GetRenderPassInfo() const { return m_RenderPassInfo; }

public:
    void SetType(RenderGraphPassType type)
    {
        m_Pass.Type = type;
    }
    void SetQueue(QueueType queue)
    {
        m_Pass.Queue = queue;
    }

    bool Exists(const std::string& name)
    {
        return m_Registry->Exists(name);
    }
    TextureInfo GetTextureInfo(const std::string& name) const
    {
        return m_Registry->GetTextureInfo(name);
    }
    
    void CreateTexture(const std::string& name, TextureInfo info)
    {
        m_Registry->CreateTexture(name, info);
        m_Pass.TextureCreatesAndImports.push_back({name, ResourceState::None()});
    }
    void ImportTexture(const std::string& name, RenderHandle texture, ResourceState previous_state = ResourceState::None())
    {
        m_Registry->ImportTexture(name, texture, m_Device->GetTextureInfo(texture));
        m_Pass.TextureCreatesAndImports.push_back({name, previous_state});
    }
    void CreateBuffer(const std::string& name, BufferInfo info)
    {
        m_Registry->CreateBuffer(name, info);
        m_Pass.BufferCreatesAndImports.push_back({name, ResourceState::None()});
    }
    void ReadTexture(const std::string& name, ResourceState state)
    {
        m_Pass.AddTextureRead(name, state);
        m_Registry->AddTextureBindFlag(name, state.Bind);
        
        if (state == ResourceState::Color())
        {
            TextureInfo info = m_Registry->GetTextureInfo(name);
            uint32 index = m_RenderPassInfo.Layout.ColorCount++;
            m_RenderPassInfo.Layout.Colors[index] = {info.Format, info.Samples};
            m_RenderPassInfo.ColorOptions[index] = {LoadOptions::Load, StoreOptions::Store};
            m_Pass.ColorAttachments.push_back(name);
            m_Pass.ColorClearColors.push_back({0.0f, 0.0f, 0.0f, 1.0f});
        }
        else if (state == ResourceState::Depth())
        {
            TextureInfo info = m_Registry->GetTextureInfo(name);
            uint32 index = m_RenderPassInfo.Layout.DepthCount++;
            m_RenderPassInfo.Layout.Depth[index] = {info.Format, info.Samples};
            m_RenderPassInfo.DepthOptions[index] = {LoadOptions::Load, StoreOptions::Store};
            m_Pass.DepthAttachment.push_back(name);
        }
        else if (state == ResourceState::Resolve())
        {
            uint32 index = m_RenderPassInfo.Layout.ResolveCount++;
            m_RenderPassInfo.ResolveOptions[index] = {LoadOptions::Load, StoreOptions::Store};
            m_Pass.ResolveAttachments.push_back(name);
        }
    }
    void WriteTexture(const std::string& name, ResourceState state, bool clear = false, vec4 clear_color = {0.0f, 0.0f, 0.0f, 1.0f}, int32 layer = -1)
    {
        m_Pass.AddTextureWrite(name, state);
        m_Registry->AddTextureBindFlag(name, state.Bind);

        if (state == ResourceState::Color())
        {
            TextureInfo info = m_Registry->GetTextureInfo(name);
            uint32 index = m_RenderPassInfo.Layout.ColorCount++;
            m_RenderPassInfo.Layout.Colors[index] = {info.Format, info.Samples};
            m_RenderPassInfo.ColorOptions[index] = {clear ? LoadOptions::Clear : LoadOptions::Discard, StoreOptions::Store};
            m_Pass.ColorAttachments.push_back(name);
            m_Pass.ColorClearColors.push_back(clear_color);
            m_Pass.ColorLayers.push_back(layer);

        }
        else if (state == ResourceState::Depth())
        {
            TextureInfo info = m_Registry->GetTextureInfo(name);
            uint32 index = m_RenderPassInfo.Layout.DepthCount++;
            m_RenderPassInfo.Layout.Depth[index] = {info.Format, info.Samples};
            m_RenderPassInfo.DepthOptions[index] = {clear ? LoadOptions::Clear : LoadOptions::Discard, StoreOptions::Store};
            m_Pass.DepthAttachment.push_back(name);
        }
        else if (state == ResourceState::Resolve())
        {
            uint32 index = m_RenderPassInfo.Layout.ResolveCount++;
            m_RenderPassInfo.ResolveOptions[index] = {LoadOptions::Discard, StoreOptions::Store};
            m_Pass.ResolveAttachments.push_back(name);
        }
    }
    void ReadWriteTexture(const std::string& name, ResourceState state, int32 layer = -1)
    {
        m_Pass.AddTextureReadWrite(name, state);
        m_Registry->AddTextureBindFlag(name, state.Bind);

        if (state == ResourceState::Color())
        {
            TextureInfo info = m_Registry->GetTextureInfo(name);
            uint32 index = m_RenderPassInfo.Layout.ColorCount++;
            m_RenderPassInfo.Layout.Colors[index] = {info.Format, info.Samples};
            m_RenderPassInfo.ColorOptions[index] = {LoadOptions::Load, StoreOptions::Store};
            m_Pass.ColorAttachments.push_back(name);
            m_Pass.ColorClearColors.push_back({0.0f, 0.0f, 0.0f, 1.0f});
            m_Pass.ColorLayers.push_back(layer);
        }
        else if (state == ResourceState::Depth())
        {
            TextureInfo info = m_Registry->GetTextureInfo(name);
            uint32 index = m_RenderPassInfo.Layout.DepthCount++;
            m_RenderPassInfo.Layout.Depth[index] = {info.Format, info.Samples};
            m_RenderPassInfo.DepthOptions[index] = {LoadOptions::Load, StoreOptions::Store};
            m_Pass.DepthAttachment.push_back(name);
        }
        else if (state == ResourceState::Resolve())
        {
            uint32 index = m_RenderPassInfo.Layout.ResolveCount++;
            m_RenderPassInfo.ResolveOptions[index] = {LoadOptions::Load, StoreOptions::Store};
            m_Pass.ResolveAttachments.push_back(name);
        }
    }
    void ReadBuffer(const std::string& name, ResourceState state)
    {
        m_Pass.AddBufferRead(name, state);
        m_Registry->AddBufferBindFlag(name, state.Bind);
    }
    void WriteBuffer(const std::string& name, ResourceState state)
    {
        m_Pass.AddBufferWrite(name, state);
        m_Registry->AddBufferBindFlag(name, state.Bind);
    }
    void ReadWriteBuffer(const std::string& name, ResourceState state)
    {
        m_Pass.AddBufferReadWrite(name, state);
        m_Registry->AddBufferBindFlag(name, state.Bind);
    }
};