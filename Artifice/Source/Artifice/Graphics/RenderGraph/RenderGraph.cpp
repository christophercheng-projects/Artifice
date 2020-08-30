#include "RenderGraph.h"

#include "Artifice/Debug/Instrumentor.h"
#include "Artifice/Utils/Timer.h"


void RenderGraphPass::ClearSynchronizationStructures()
{
    AR_PROFILE_FUNCTION();

    WaitSemaphores.clear();
    WaitStages.clear();
    SignalSemaphores.clear();
    TextureBarriers.clear();
    BufferBarriers.clear();
    ReleaseTextureBarriers.clear();
    ReleaseBufferBarriers.clear();
    AcquireTextureBarriers.clear();
    AcquireBufferBarriers.clear();
}

void RenderGraphPass::EvaluateInternal(RenderGraphRenderPassCache* render_pass_cache, RenderGraphFramebufferCache* framebuffer_cache, RenderGraphRegistry* registry, CommandBuffer* cmd, Device* device)
{
    AR_PROFILE_FUNCTION();

    // Pre barriers
    for (auto& barrier : AcquireTextureBarriers)
    {
        RenderHandle texture = registry->GetTexture(barrier.Resource);
        cmd->AcquireTextureQueueOwnership(barrier.SourceQueue, BuilderPass.Queue, texture, barrier.Source, barrier.Destination);
    }
    for (auto& barrier : AcquireBufferBarriers)
    {
        RenderHandle buffer = registry->GetBuffer(barrier.Resource);
        cmd->AcquireBufferQueueOwnership(barrier.SourceQueue, BuilderPass.Queue, buffer, barrier.Destination);
    }
    for (auto& barrier : TextureBarriers)
    {
        RenderHandle texture = registry->GetTexture(barrier.Resource);
        cmd->TextureBarrier(texture, barrier.Source, barrier.Destination);
    }
    for (auto& barrier : BufferBarriers)
    {
        RenderHandle buffer = registry->GetBuffer(barrier.Resource);
        cmd->BufferBarrier(buffer, barrier.Source, barrier.Destination);
    }
    // Evaluate
    if (BuilderPass.Type == RenderGraphPassType::Render)
    {
        // Means real render pass
        std::vector<VkClearValue> clear;
        FramebufferInfo fb;
        fb.RenderPassLayout = RenderPassInfo.Layout;
        fb.Width = registry->GetTextureInfo(BuilderPass.ColorAttachments[0]).Width;
        fb.Height = registry->GetTextureInfo(BuilderPass.ColorAttachments[0]).Height;
        for (uint32 i = 0; i < BuilderPass.ColorAttachments.size(); i++)
        {
            fb.ColorTextures[i] = registry->GetTexture(BuilderPass.ColorAttachments[i]);
            fb.Layers[i] = BuilderPass.ColorLayers[i];
            vec4 c = BuilderPass.ColorClearColors[i];
            clear.push_back({c.x, c.y, c.z, c.w});
        }
        for (uint32 i = 0; i < BuilderPass.ResolveAttachments.size(); i++)
        {
            fb.ResolveTextures[i] = registry->GetTexture(BuilderPass.ResolveAttachments[i]);
            clear.push_back({0, 0, 0, 0});
        }
        for (uint32 i = 0; i < BuilderPass.DepthAttachment.size(); i++)
        {
            fb.DepthTexture[i] = registry->GetTexture(BuilderPass.DepthAttachment[i]);
            clear.push_back({1.0f, 0});
        }

        cmd->BeginRenderPass(render_pass_cache->Request(RenderPassInfo), framebuffer_cache->Request(fb), clear);
        Evaluate(registry, cmd);
        cmd->EndRenderPass();
    }
    else
    {
        // Just call commands
        Evaluate(registry, cmd);
    }
    // Post barriers
    for (auto& barrier : ReleaseTextureBarriers)
    {
        RenderHandle texture = registry->GetTexture(barrier.Resource);
        cmd->ReleaseTextureQueueOwnership(BuilderPass.Queue, barrier.DestinationQueue, texture, barrier.Source, barrier.Destination);
    }
    for (auto& barrier : ReleaseBufferBarriers)
    {
        RenderHandle buffer = registry->GetBuffer(barrier.Resource);
        cmd->ReleaseBufferQueueOwnership(BuilderPass.Queue, barrier.DestinationQueue, buffer, barrier.Source);
    }
}


// RENDER GRAPH

RenderGraph::RenderGraph(Device* device) : m_Device(device)
{
    m_Registry.Init(device);
    m_RenderPassCache.Init(device);
    m_FramebufferCache.Init(device);
}

void RenderGraph::Init(Device* device)
{
    m_Device = device;
    m_Registry.Init(device);
    m_RenderPassCache.Init(device);
    m_FramebufferCache.Init(device);
}

void RenderGraph::CleanUp()
{
    Reset();
    m_Registry.CleanUp();
    m_RenderPassCache.Reset();
    m_FramebufferCache.Reset();
}

void RenderGraph::Reset()
{
    AR_PROFILE_FUNCTION();

    m_Blackboard.Reset();
    
    for (auto& pass : m_Passes)
    {
        for (auto& sem : pass.SignalSemaphores)
        {
            m_Device->DestroySemaphore(sem);
        }
    }
    m_Passes.clear();
    m_Registry.Clear();
    m_TextureLifetimes.clear();
    m_BufferLifetimes.clear();

    m_Registry.Advance();
    m_RenderPassCache.Advance();
    m_FramebufferCache.Advance();
}

void RenderGraph::AddPass(const std::string& name, RenderGraphPass::ConstructionFunction construct)
{
    AR_PROFILE_FUNCTION();

    RenderGraphBuilder builder(m_Device, &m_Registry);

    RenderGraphPass pass;
    pass.Name = name;
    pass.Evaluate = construct(&builder);
    pass.BuilderPass = builder.GetPass();
    pass.RenderPassInfo = builder.GetRenderPassInfo();

    m_Passes.push_back(pass);
}

void RenderGraph::Compile()
{
    AR_PROFILE_FUNCTION();

    uint32 rp_count = 0;
    std::vector<std::string> pass_names;

    Timer timer;

    ConstructResourceLifetimes();
    for (auto& pass : m_Passes)
    {
        pass_names.push_back(pass.Name);
        if (pass.BuilderPass.Type == RenderGraphPassType::Render)
            rp_count++;
        
        pass.ClearSynchronizationStructures();
    }
    ConstructSynchronizationStructures();

    m_Statistics = { (uint32)m_Passes.size(), rp_count, (uint32)m_TextureLifetimes.size(), (uint32)m_BufferLifetimes.size(), timer.ElapsedMillis(), 0, 0, 0, 0, 0, pass_names, {} };
}

void RenderGraph::Evaluate()
{
    AR_PROFILE_FUNCTION();

    // Scheduling
    // Need to batch by queue submits
    // Within each queue submit batch, can go wide
    // Need a way to signal rendering finished semaphore for last swapchain touches

    // need to create dependency graph (DAG)

    Timer timer;

    CommandBuffer cmd = m_Device->RequestCommandBuffer(QueueType::Universal);
    QueueType current_queue = QueueType::Universal;
    std::vector<VkSemaphore> last_signals;
    cmd.Begin();
    std::string name = "";

    for (auto& pass : m_Passes)
    {
        if (pass.BuilderPass.Queue != current_queue || last_signals.size())
        {
            cmd.End();
            m_Device->CommitCommandBuffer(cmd);
            m_Device->SubmitQueue(current_queue, {}, {}, true);
            current_queue = pass.BuilderPass.Queue;
            cmd = m_Device->RequestCommandBuffer(pass.BuilderPass.Queue);
            cmd.Begin();
        }
        Timer pass_timer;
        name = pass.Name;
        pass.EvaluateInternal(&m_RenderPassCache, &m_FramebufferCache, &m_Registry, &cmd, m_Device);
        last_signals = pass.SignalSemaphores;
        for (uint32 i = 0; i < pass.WaitSemaphores.size(); i++)
        {
            m_Device->AddWaitSemaphore(pass.BuilderPass.Queue, pass.WaitSemaphores[i], pass.WaitStages[i]);
        }
        for (uint32 i = 0; i < pass.SignalSemaphores.size(); i++)
        {
            m_Device->AddSignalSemaphore(pass.BuilderPass.Queue, pass.SignalSemaphores[i]);
        }
        m_Statistics.PassTimes.push_back(pass_timer.ElapsedMillis());
    }

    cmd.End();
    m_Device->CommitCommandBuffer(cmd);
    
    m_Device->SubmitQueue(current_queue, {}, {}, true);

    m_Statistics.EvaluationTime = timer.ElapsedMillis();
    m_Statistics.AliveFramebufferCount = m_FramebufferCache.GetAlive();
    m_Statistics.AliveRenderPassCount = m_RenderPassCache.GetAlive();
    std::pair<uint32, uint32> tex_buf_count = m_Registry.GetAlive();
    m_Statistics.AliveTextureCount = tex_buf_count.first;
    m_Statistics.AliveBufferCount = tex_buf_count.second;
}

void RenderGraph::ConstructResourceLifetimes()
{
    AR_PROFILE_FUNCTION();

    m_TextureLifetimes.resize(m_Registry.GetTextureCount());
    m_BufferLifetimes.resize(m_Registry.GetBufferCount());

    for (uint32 pass_index = 0; pass_index < m_Passes.size(); pass_index++)
    {
        RenderGraphBuilderPass& pass = m_Passes[pass_index].BuilderPass;
        for (RenderGraphResourceSnapshot& snap : pass.TextureCreatesAndImports)
        {
            uint32 index = m_Registry.GetIndex(snap.Name);
            RenderGraphResourceLifetime& resource = m_TextureLifetimes[index];
            resource.Name = snap.Name;
            resource.InitialState = snap.State;
        }
        for (RenderGraphResourceSnapshot& snap : pass.BufferCreatesAndImports)
        {
            uint32 index = m_Registry.GetIndex(snap.Name);
            RenderGraphResourceLifetime& resource = m_BufferLifetimes[index];
            resource.Name = snap.Name;
            resource.InitialState = snap.State;
        }
        for (RenderGraphResourceSnapshot& snap : pass.TextureReads)
        {
            uint32 index = m_Registry.GetIndex(snap.Name);
            RenderGraphResourceLifetime& resource = m_TextureLifetimes[index];

            RenderGraphResourceLifetime::Snapshot snapshot;
            snapshot.Access = RenderGraphResourceAccess::Read;
            snapshot.State = snap.State;
            snapshot.PassIndex = pass_index;
            snapshot.Queue = pass.Queue;

            resource.Lifetime.push_back(snapshot);
        }
        for (RenderGraphResourceSnapshot& snap : pass.TextureWrites)
        {
            uint32 index = m_Registry.GetIndex(snap.Name);
            RenderGraphResourceLifetime& resource = m_TextureLifetimes[index];

            RenderGraphResourceLifetime::Snapshot snapshot;
            snapshot.Access = RenderGraphResourceAccess::Write;
            snapshot.State = snap.State;
            snapshot.PassIndex = pass_index;
            snapshot.Queue = pass.Queue;

            resource.Lifetime.push_back(snapshot);
        }
        for (RenderGraphResourceSnapshot& snap : pass.TextureReadWrites)
        {
            uint32 index = m_Registry.GetIndex(snap.Name);
            RenderGraphResourceLifetime& resource = m_TextureLifetimes[index];

            RenderGraphResourceLifetime::Snapshot snapshot;
            snapshot.Access = RenderGraphResourceAccess::ReadWrite;
            snapshot.State = snap.State;
            snapshot.PassIndex = pass_index;
            snapshot.Queue = pass.Queue;

            resource.Lifetime.push_back(snapshot);
        }
        for (RenderGraphResourceSnapshot& snap : pass.BufferReads)
        {
            uint32 index = m_Registry.GetIndex(snap.Name);
            RenderGraphResourceLifetime& resource = m_BufferLifetimes[index];

            RenderGraphResourceLifetime::Snapshot snapshot;
            snapshot.Access = RenderGraphResourceAccess::Read;
            snapshot.State = snap.State;
            snapshot.PassIndex = pass_index;
            snapshot.Queue = pass.Queue;

            resource.Lifetime.push_back(snapshot);
        }
        for (RenderGraphResourceSnapshot& snap : pass.BufferWrites)
        {
            uint32 index = m_Registry.GetIndex(snap.Name);
            RenderGraphResourceLifetime& resource = m_BufferLifetimes[index];

            RenderGraphResourceLifetime::Snapshot snapshot;
            snapshot.Access = RenderGraphResourceAccess::Write;
            snapshot.State = snap.State;
            snapshot.PassIndex = pass_index;
            snapshot.Queue = pass.Queue;

            resource.Lifetime.push_back(snapshot);
        }
        for (RenderGraphResourceSnapshot& snap : pass.BufferReadWrites)
        {
            uint32 index = m_Registry.GetIndex(snap.Name);
            RenderGraphResourceLifetime& resource = m_BufferLifetimes[index];

            RenderGraphResourceLifetime::Snapshot snapshot;
            snapshot.Access = RenderGraphResourceAccess::ReadWrite;
            snapshot.State = snap.State;
            snapshot.PassIndex = pass_index;
            snapshot.Queue = pass.Queue;

            resource.Lifetime.push_back(snapshot);
        }
    }
}

void RenderGraph::ConstructSynchronizationStructures()
{
    AR_PROFILE_FUNCTION();

    // Construct texture barriers/semaphores
    for (uint32 resource_index = 0; resource_index < m_TextureLifetimes.size(); resource_index++)
    {
        RenderGraphResourceLifetime& lifetime = m_TextureLifetimes[resource_index];
        if (lifetime.Lifetime.empty())
        {
            continue;
            AR_CORE_WARN("RenderGraph unused texture: %s", lifetime.Name.c_str());
        }

        RenderGraphResourceLifetime::Snapshot& first_snap = lifetime.Lifetime[0];

        Barrier barrier;
        barrier.Resource = lifetime.Name;
        barrier.Source = lifetime.InitialState;
        barrier.Destination = first_snap.State;

        RenderGraphPass& first_pass = m_Passes[first_snap.PassIndex];
        first_pass.TextureBarriers.push_back(barrier);

        for (uint32 pass_it = 1; pass_it < lifetime.Lifetime.size(); pass_it++)
        {
            RenderGraphResourceLifetime::Snapshot& prev_snap = lifetime.Lifetime[pass_it - 1];
            RenderGraphResourceLifetime::Snapshot& curr_snap = lifetime.Lifetime[pass_it];

            RenderGraphPass& prev_pass = m_Passes[prev_snap.PassIndex];
            RenderGraphPass& curr_pass = m_Passes[curr_snap.PassIndex];

            if (prev_snap.Queue != curr_snap.Queue)
            {
                // TODO: Semaphore management
                VkSemaphore semaphore;
                VkSemaphoreCreateInfo semaphore_ci = {};
                semaphore_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
                VK_CALL(vkCreateSemaphore(m_Device->GetHandle(), &semaphore_ci, nullptr, &semaphore));
                // TODO: Semaphore management

                prev_pass.SignalSemaphores.push_back(semaphore);
                curr_pass.WaitSemaphores.push_back(semaphore);
                curr_pass.WaitStages.push_back(curr_snap.State.Stage);

                ReleaseBarrier release;
                release.Resource = lifetime.Name;
                release.Source = prev_snap.State;
                release.Destination = curr_snap.State;
                release.DestinationQueue = curr_snap.Queue;

                prev_pass.ReleaseTextureBarriers.push_back(release);

                AcquireBarrier acquire;
                acquire.Resource = lifetime.Name;
                acquire.Source = prev_snap.State;
                acquire.SourceQueue = prev_snap.Queue;
                acquire.Destination = curr_snap.State;

                curr_pass.AcquireTextureBarriers.push_back(acquire);
            }
            else
            {
                if (prev_snap.State == curr_snap.State && curr_snap.Access == RenderGraphResourceAccess::Read)
                {
                    // Read after read in same layout. No barrier necessary.
                    continue;
                }

                Barrier barrier;
                barrier.Resource = lifetime.Name;
                barrier.Source = prev_snap.State;
                barrier.Destination = curr_snap.State;

                curr_pass.TextureBarriers.push_back(barrier);
            }
        }
    } // End iterate texture lifetimes
    // Construct buffer barriers/semaphores
    for (uint32 resource_index = 0; resource_index < m_BufferLifetimes.size(); resource_index++)
    {
        RenderGraphResourceLifetime& lifetime = m_BufferLifetimes[resource_index];

        if (lifetime.Lifetime.empty())
        {
            continue;
            AR_CORE_WARN("RenderGraph unused buffer: %s", lifetime.Name.c_str());
        }

        RenderGraphResourceLifetime::Snapshot& first_snap = lifetime.Lifetime[0];

        Barrier barrier;
        barrier.Resource = lifetime.Name;
        barrier.Source = lifetime.InitialState;
        barrier.Destination = first_snap.State;

        RenderGraphPass& first_pass = m_Passes[first_snap.PassIndex];
        first_pass.BufferBarriers.push_back(barrier);

        for (uint32 pass_it = 1; pass_it < lifetime.Lifetime.size(); pass_it++)
        {
            RenderGraphResourceLifetime::Snapshot& prev_snap = lifetime.Lifetime[pass_it - 1];
            RenderGraphResourceLifetime::Snapshot& curr_snap = lifetime.Lifetime[pass_it];

            RenderGraphPass& prev_pass = m_Passes[prev_snap.PassIndex];
            RenderGraphPass& curr_pass = m_Passes[curr_snap.PassIndex];

            if (prev_snap.Queue != curr_snap.Queue)
            {
                VkSemaphore semaphore;
                prev_pass.SignalSemaphores.push_back(semaphore);
                curr_pass.WaitSemaphores.push_back(semaphore);
                curr_pass.WaitStages.push_back(curr_snap.State.Stage);

                ReleaseBarrier release;
                release.Resource = lifetime.Name;
                release.Source = prev_snap.State;
                release.Destination = curr_snap.State;
                release.DestinationQueue = curr_snap.Queue;

                prev_pass.ReleaseBufferBarriers.push_back(release);

                AcquireBarrier acquire;
                acquire.Resource = lifetime.Name;
                acquire.Source = prev_snap.State;
                acquire.SourceQueue = prev_snap.Queue;
                acquire.Destination = curr_snap.State;

                curr_pass.AcquireBufferBarriers.push_back(acquire);
            }
            else
            {
                if (prev_snap.State == curr_snap.State && curr_snap.Access == RenderGraphResourceAccess::Read)
                {
                    // Read after read in with guaranteed previous invalidate. No barrier necessary.
                    continue;
                }

                Barrier barrier;
                barrier.Resource = lifetime.Name;
                barrier.Source = prev_snap.State;
                barrier.Destination = curr_snap.State;

                curr_pass.BufferBarriers.push_back(barrier);
            }
        }
    } // End iterate buffer lifetimes
}