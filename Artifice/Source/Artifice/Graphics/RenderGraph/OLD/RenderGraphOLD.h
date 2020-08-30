#pragma once
#if 0
#include <vector>
#include <unordered_map>
#include <set>

#include "Artifice/Core/Core.h"

#include "Artifice/Graphics/RenderHandle.h"
#include "Artifice/Graphics/Resources.h"

struct RenderCommandList
{

};

struct RenderGraphResource
{

};

struct Blackboard
{
    std::unordered_map<std::string, int> Dictionary;
};

enum class LoadOp { Load, Discard, Clear};

struct ReadInfo
{
    std::string Name;
    RenderBindFlags BindFlag;
};

struct WriteInfo
{
    std::string Name;
    RenderBindFlags BindFlag;
    LoadOp Load;
};


struct AccessInfo
{
    std::string Name;
    RenderBindFlags FirstBindFlag = RenderBindFlags::None;
    RenderBindFlags LastBindFlag;
    LoadOp Load;
};


struct VirtualGraphicsPipeline
{   

};

// each resource
// resource bind flag in each render pass
//  access in each subpass if is attachment
// essentially want resource state of every resource in every subpass
// and every render pass should know its attachments

// 

struct RenderGraphPass
{
    std::vector<std::string> Dependencies;

    std::vector<std::string> Attachments;
    std::vector<AccessInfo> AccessInfos;


    struct Subpass
    {
        std::unordered_set<std::string> Colors;
        std::unordered_set<std::string> DepthStencils;
        std::unordered_set<std::string> Inputs;
        std::unordered_set<std::string> Resolves;
        std::unordered_set<std::string> Preserves;
    };

    std::vector<Subpass> Subpasses;


    std::unordered_map<std::string, VirtualGraphicsPipeline> GraphicsPipelines;
};


struct VirtualBarrier
{
    RenderBindFlags Source;
    RenderBindFlags Destination;
};

struct VirtualTexture
{
    uint32 Width;
    uint32 Height;
    VkFormat Format;

};

struct VirtualBuffer
{
    uint32 Size;
};

struct PassInfo
{
    enum class Type { Graphics, Compute } Type;
    VkRenderPass PassRaw;
    std::vector<VkImageMemoryBarrier> ImageBarriers;
    std::vector<VkBufferMemoryBarrier> BufferBarriers;
};


struct RenderGraph
{
    struct Registry
    {

    };

    struct Builder
    {
        RenderGraph* m_Graph;

        std::unordered_map<std::string, VirtualBuffer> VirtualBuffers;
        std::unordered_map<std::string, VirtualTexture> VirtualTextures;

        std::unordered_map<std::string, RenderGraphPass> Passes;
        std::unordered_map<std::string, std::function<void(Registry *, RenderCommandList *)>> PassCallbacks;

        std::unordered_map<std::string, std::unordered_map<VirtualBarrier, std::vector<std::string>>> VirtualTextureBarriers;
        std::unordered_map<std::string, std::unordered_map<VirtualBarrier, std::vector<std::string>>> VirtualBufferBarriers;
        std::unordered_map<std::string, VkRenderPass> PassesRaw;


        RenderGraphPass* CurrentPass;

        void Build(VkDevice device);
        void Build2();


        void SetCurrentPass(std::string name)
        {
            CurrentPass = &Passes[name];
        }

        void AddPassDependencies(std::vector<std::string> passes)
        {
            for (auto& pass : passes)
            {
                CurrentPass->Dependencies.push_back(pass);
            }
        }

        void CreateTexture(std::string name, VirtualTexture virtual_texture)
        {
            AR_CORE_ASSERT(!VirtualTextures.count(name), "Tried creating virtual texture that already exists");
            VirtualTextures.emplace(name, virtual_texture);
        }

        void CreateBuffer(std::string name, VirtualBuffer virtual_buffer)
        {
            AR_CORE_ASSERT(!VirtualBuffers.count(name), "Tried creating virtual buffer that already exists");
            VirtualBuffers.emplace(name, virtual_buffer);
        }

        void AddColorAttachment(uint32 subpass_index, std::string name, LoadOp load = LoadOp::Discard)
        {
            
        }
        void AddDepthStencilAttachment(uint32 subpass_index, std::string name, LoadOp load = LoadOp::Discard)
        {

        }
        void AddInputAttachment(uint32 subpass_index, std::string name)
        {

        }

    };

    Builder m_Builder;
    std::vector<VkRenderPass> RenderPassesRaw;
    std::vector<std::function<void(Registry*, RenderCommandList*)>> PassCallbacks;
    
    void AddPass(std::string name, std::function<std::function<void(Registry*, RenderCommandList*)>(Builder*)> setup_pass)
    {
        m_Builder.SetCurrentPass(name);

        auto callback = setup_pass(&m_Builder);

        PassCallbacks.push_back(callback);
    }
};


void test()
{
    RenderGraph graph;
    graph.AddRenderPass("Test", [&](RenderGraph::Builder* builder)
    {
        builder->AddPassDependencies({"Test0"});

        builder->CreateTexture("color_attachment", {1280, 720, VK_FORMAT_R8G8B8A8_UNORM});

        builder->AddColorAttachment(0, "color_attachment", LoadOp::Discard);
        builder->Read(1, "sampled_texture", RenderBindFlags::SampledTexture);
        builder->Write(1, "color_attachment", RenderBindFlags::ColorAttachment, LoadOp::Load);


        builder->CreateGraphicsPipeline("pipe", {});


        return [=](RenderGraph::Registry* registry, RenderCommandList* cmd)
        {
            auto pipe = registry->GetGraphicsPipeline("pipe");
            cmd->Draw(pipe);
            cmd->NextSubpass();
            cmd->Draw();
        };
    });
}

VkAccessFlags RenderBindFlagsToAccess(RenderBindFlags flag)
{
    switch (flag)
    {
        case RenderBindFlags::TransferSource:
        case RenderBindFlags::TransferDestination:
        case RenderBindFlags::UniformBuffer:
        case RenderBindFlags::StorageBuffer:
        case RenderBindFlags::VertexBuffer:
        case RenderBindFlags::IndexBuffer:
        case RenderBindFlags::ColorAttachment:
        case RenderBindFlags::DepthStencilAttachment:
        case RenderBindFlags::SampledTexture:
        case RenderBindFlags::StorageTexture:
        default:
            return 0;
    }
}
VkPipelineStageFlags RenderBindFlagsToStage(RenderBindFlags flag)
{
    switch (flag)
    {
        case RenderBindFlags::TransferSource:
        case RenderBindFlags::TransferDestination:
        case RenderBindFlags::UniformBuffer:
        case RenderBindFlags::StorageBuffer:
        case RenderBindFlags::VertexBuffer:
        case RenderBindFlags::IndexBuffer:
        case RenderBindFlags::ColorAttachment:
        case RenderBindFlags::DepthStencilAttachment:
        case RenderBindFlags::SampledTexture:
        case RenderBindFlags::StorageTexture:
        default:
            return 0;
    }
}

VkImageLayout RenderBindFlagToImageLayout(RenderBindFlags flag)
{
    switch (flag)
    {
        case RenderBindFlags::Transfer:
        case RenderBindFlags::UniformBuffer:
        case RenderBindFlags::StorageBuffer:
        case RenderBindFlags::VertexBuffer:
        case RenderBindFlags::IndexBuffer:
        case RenderBindFlags::ColorAttachment:
        case RenderBindFlags::DepthStencilAttachment:
        case RenderBindFlags::SampledTexture:
        case RenderBindFlags::StorageTexture:
        default:
            return VK_IMAGE_LAYOUT_UNDEFINED;
    }
}
#endif