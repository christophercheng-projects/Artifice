#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <functional>

#include <vulkan/vulkan.h>

#include "Artifice/Core/Core.h"
#include "Artifice/Core/Variant.h"
#include "Artifice/Graphics/Resources.h"

#include "Artifice/Graphics/Device.h"

#include "RenderGraphBuilder.h"
#include "RenderGraphRegistry.h"

#define RENDER_GRAPH_PRIMARY_SWAPCHAIN_NAME "Primary Swapchain"


struct Barrier
{
    std::string Resource;
    ResourceState Source;
    ResourceState Destination;
};
struct ReleaseBarrier
{
    std::string Resource;
    ResourceState Source;
    ResourceState Destination;
    QueueType DestinationQueue;
};
struct AcquireBarrier
{
    std::string Resource;
    ResourceState Source;
    ResourceState Destination;
    QueueType SourceQueue;
};

enum class RenderGraphResourceAccess
{
    Read, Write, ReadWrite
};

struct RenderGraphResourceLifetime
{
    struct Snapshot
    {
        RenderGraphResourceAccess Access;
        ResourceState State;
        QueueType Queue;
        uint32 PassIndex;
    };
    std::string Name;
    ResourceState InitialState;
    std::vector<Snapshot> Lifetime; 
};

// RENDER GRAPH PASS

struct RenderGraphPass
{
    using EvaluationFunction = std::function<void(RenderGraphRegistry*, CommandBuffer*)>;
    using ConstructionFunction = std::function<EvaluationFunction(RenderGraphBuilder*)>;

    std::string Name;
    
    RenderGraphBuilderPass BuilderPass;

    RenderPassInfo RenderPassInfo;

    std::set<uint32> PassDependencies;

    std::vector<VkSemaphore> WaitSemaphores;
    std::vector<PipelineStageFlags> WaitStages;
    std::vector<VkSemaphore> SignalSemaphores;

    std::vector<Barrier> TextureBarriers;
    std::vector<Barrier> BufferBarriers;
    std::vector<ReleaseBarrier> ReleaseTextureBarriers;
    std::vector<ReleaseBarrier> ReleaseBufferBarriers;
    std::vector<AcquireBarrier> AcquireTextureBarriers;
    std::vector<AcquireBarrier> AcquireBufferBarriers;

    EvaluationFunction Evaluate;


    void ClearSynchronizationStructures();
    void EvaluateInternal(RenderGraphRenderPassCache* render_pass_cache, RenderGraphFramebufferCache* framebuffer_cache, RenderGraphRegistry* registry, CommandBuffer* cmd, Device* device);
};


class Blackboard
{
    friend class RenderGraph;

private:
    std::unordered_map<std::string, Variant> m_ValueStorage;
public:
    Blackboard() = default;

    void Write(std::string key, Variant value)
    {
        m_ValueStorage[key] = value;
    }

    Variant* Read(std::string key)
    {
        auto find = m_ValueStorage.find(key);
        if (find == m_ValueStorage.end())
        {
            return nullptr;
        }

        return &find->second;
    }

private:
    void Reset()
    {
        m_ValueStorage.clear();
    }
};


// RNEDER GRAPH

class RenderGraph
{
private:
    Device* m_Device;
    Blackboard m_Blackboard;
    RenderGraphRegistry m_Registry;
    RenderGraphRenderPassCache m_RenderPassCache;
    RenderGraphFramebufferCache m_FramebufferCache;
    std::vector<RenderGraphPass> m_Passes;

    std::vector<RenderGraphResourceLifetime> m_TextureLifetimes;
    std::vector<RenderGraphResourceLifetime> m_BufferLifetimes;

public:
    RenderGraph() = default;
    RenderGraph(Device* device);

    void Init(Device* device);

    void CleanUp();

    void Reset();

    void WriteBlackboard(std::string key, Variant value)
    {
        m_Blackboard.Write(key, value);
    }

    Variant* ReadBlackboard(std::string key)
    {
        return m_Blackboard.Read(key);
    }

    void AddPass(const std::string& name, RenderGraphPass::ConstructionFunction construct);

    void Compile();

    void Evaluate();

private:
    void ConstructResourceLifetimes();
    void ConstructSynchronizationStructures();

public:
    struct Statistics
    {
        uint32 TotalPassCount = 0;
        uint32 RenderPassCount = 0;
        uint32 TextureCount = 0;
        uint32 BufferCount = 0;
        float ConstructionTime = 0;
        float EvaluationTime = 0;

        uint32 AliveFramebufferCount = 0;
        uint32 AliveRenderPassCount = 0;
        uint32 AliveTextureCount = 0;
        uint32 AliveBufferCount = 0;

        std::vector<std::string> PassNames;
        std::vector<float> PassTimes;
    };

    Statistics GetStats() const { return m_Statistics; }
    void ResetStats() { memset(&m_Statistics, 0, sizeof(Statistics)); }

private:
    Statistics m_Statistics;

};