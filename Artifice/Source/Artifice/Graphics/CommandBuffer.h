#pragma once

#include <vulkan/vulkan.h>

#include "Resources.h"

class Device;


class CommandBuffer
{
private:
    Device* m_Device;
    QueueType m_Type;
public:
    QueueType GetType() const { return m_Type; }

public:
    VkCommandBuffer m_CommandBuffer;
    VkPipelineLayout m_CurrentPipelineLayout;
    VkPipelineLayout m_CurrentComputePipelineLayout;

public:
    CommandBuffer() = default;
    CommandBuffer(Device* device, QueueType type, VkCommandBuffer command_buffer) : m_Device(device), m_Type(type), m_CommandBuffer(command_buffer) {}

    void Begin();
    void End();

public: // Graphics commands
    void BeginRenderPass(RenderHandle render_pass, RenderHandle framebuffer, std::vector<VkClearValue> clear_values);
    void EndRenderPass();

    void SetViewport(uint32 width, uint32 height);
    void SetScissor(int32 x, int32 y, uint32 width, uint32 height);

    void BindGraphicsPipeline(RenderHandle pipeline);

    void BindGraphicsDescriptorSet(uint32 set, RenderHandle descriptor_set, std::vector<uint32> dynamic_offset = {});

    void PushConstants(ShaderStage stage, uint32 size, uint32 offset, const void* data);

    void BindVertexBuffers(std::vector<RenderHandle> buffers, std::vector<uint64> offsets);
    void BindIndexBuffer(RenderHandle buffer, uint64 offset, VkIndexType index_type);

    // Draw commands
    void Draw(uint32 vertex_count, uint32 instance_count, uint32 first_vertex, uint32 first_instance);
    void DrawIndexed(uint32 index_count, uint32 instance_count, uint32 first_index, uint32 vertex_offset, uint32 first_instance);
public: // Compute commands
    void BindComputePipeline(RenderHandle pipeline);
    void BindComputeDescriptorSet(uint32 set, RenderHandle descriptor_set, std::vector<uint32> dynamic_offset = {});

    // Dispatch commands
    void Dispatch(uint32 group_count_x, uint32 group_count_y, uint32 group_count_z);

public: // Transfer commands
    void UploadBufferData(RenderHandle buffer, uint32 size, const void* data, ResourceState destination);
    void UploadTextureData(RenderHandle texture, uint32 size, const void* data, bool generate_mips, ResourceState destination);
    void UploadTextureData(RenderHandle texture, uint32 size, const void* data, bool generate_mips);
    void CopyTexture(RenderHandle source, RenderHandle destination);

    void GenerateTextureMips(RenderHandle texture, ResourceState source, ResourceState destination);
    void GenerateTextureMips(RenderHandle texture);

public: // Synchronization commands
    // Barriers
    void TextureBarrier(RenderHandle texture, ResourceState source, ResourceState destination, uint32 source_queue_family = VK_QUEUE_FAMILY_IGNORED, uint32 destination_queue_family = VK_QUEUE_FAMILY_IGNORED);
    void BufferBarrier(RenderHandle buffer, ResourceState source, ResourceState destination, uint32 source_queue_family = VK_QUEUE_FAMILY_IGNORED, uint32 destination_queue_family = VK_QUEUE_FAMILY_IGNORED);

    // Synchronize resource ownerships between queue families
    void ReleaseTextureQueueOwnership(QueueType source_queue, QueueType destination_queue, RenderHandle texture, ResourceState source, ResourceState destination);
    void AcquireTextureQueueOwnership(QueueType source_queue, QueueType destination_queue, RenderHandle texture, ResourceState source, ResourceState destination);
    void ReleaseBufferQueueOwnership(QueueType source_queue, QueueType destination_queue, RenderHandle buffer, ResourceState source);
    void AcquireBufferQueueOwnership(QueueType source_queue, QueueType destination_queue, RenderHandle buffer, ResourceState destination);
};