#include "CommandBuffer.h"

#include <stb_image.h>

#include "Artifice/Core/Application.h"
#include "Device.h"
#include "RenderBackend.h"

void CommandBuffer::Begin()
{
    AR_PROFILE_FUNCTION();

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CALL(vkBeginCommandBuffer(m_CommandBuffer, &beginInfo));
}

void CommandBuffer::End()
{
    AR_PROFILE_FUNCTION();

    vkEndCommandBuffer(m_CommandBuffer);
}

void CommandBuffer::BeginRenderPass(RenderHandle render_pass, RenderHandle framebuffer, std::vector<VkClearValue> clear_values)
{
    AR_PROFILE_FUNCTION();

    RenderPass* rp = m_Device->GetRenderPass(render_pass);
    Framebuffer* fb = m_Device->GetFramebuffer(framebuffer);

    VkRenderPassBeginInfo bi = {};
    bi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    bi.renderPass = rp->RenderPassRaw;
    bi.framebuffer = fb->FramebufferRaw;
    bi.renderArea.offset = {0, 0};
    bi.renderArea.extent = {fb->Info.Width, fb->Info.Height};
    bi.clearValueCount = clear_values.size();
    bi.pClearValues = clear_values.data();

    vkCmdBeginRenderPass(m_CommandBuffer, &bi, VK_SUBPASS_CONTENTS_INLINE);
}
void CommandBuffer::EndRenderPass()
{
    AR_PROFILE_FUNCTION();

    vkCmdEndRenderPass(m_CommandBuffer);
}

void CommandBuffer::SetViewport(uint32 width, uint32 height)
{
    AR_PROFILE_FUNCTION();

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)width;
    viewport.height = (float)height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

       vkCmdSetViewport(m_CommandBuffer, 0, 1, &viewport);
}

void CommandBuffer::SetScissor(int32 x, int32 y, uint32 width, uint32 height)
{
    AR_PROFILE_FUNCTION();

    VkRect2D scissor = {};
    scissor.offset = {x, y};
    scissor.extent = {width, height};

    vkCmdSetScissor(m_CommandBuffer, 0, 1, &scissor);
}

void CommandBuffer::BindGraphicsPipeline(RenderHandle pipeline)
{
    AR_PROFILE_FUNCTION();

    GraphicsPipeline* gp = m_Device->GetGraphicsPipeline(pipeline);
    m_CurrentPipelineLayout = m_Device->GetPipelineLayout(gp->Info.PipelineLayout)->PipelineLayoutRaw;

    vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gp->PipelineRaw);
}

void CommandBuffer::BindGraphicsDescriptorSet(uint32 set, RenderHandle descriptor_set, std::vector<uint32> dynamic_offsets)
{
    AR_PROFILE_FUNCTION();

    DescriptorSet* ds = m_Device->GetDescriptorSet(descriptor_set);
    vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_CurrentPipelineLayout, set, 1, &ds->DescriptorSetRaw, dynamic_offsets.size(), dynamic_offsets.data());
}

void CommandBuffer::PushConstants(ShaderStage stage, uint32 size, uint32 offset, const void* data)
{
    AR_PROFILE_FUNCTION();

    vkCmdPushConstants(m_CommandBuffer, stage == ShaderStage::Compute ? m_CurrentComputePipelineLayout : m_CurrentPipelineLayout, RenderBackend::ConvertShaderStage(stage), offset, size, data);
}

void CommandBuffer::BindVertexBuffers(std::vector<RenderHandle> buffers, std::vector<uint64> offsets)
{
    AR_PROFILE_FUNCTION();

    AR_CORE_ASSERT(buffers.size() == offsets.size(), "");

    ScratchAllocator alloc;

    uint32 count = buffers.size();
    VkBuffer* buffers_raw = alloc.Allocate<VkBuffer>(count);
    for (uint32 i = 0; i < count; i++)
    {
        buffers_raw[i] = m_Device->GetBuffer(buffers[i])->BufferRaw;
    }
    vkCmdBindVertexBuffers(m_CommandBuffer, 0, count, buffers_raw, offsets.data());
}

void CommandBuffer::BindIndexBuffer(RenderHandle buffer, uint64 offset, VkIndexType index_type)
{
    AR_PROFILE_FUNCTION();

    vkCmdBindIndexBuffer(m_CommandBuffer, m_Device->GetBuffer(buffer)->BufferRaw, offset, index_type);
}

void CommandBuffer::Draw(uint32 vertex_count, uint32 instance_count, uint32 first_vertex, uint32 first_instance)
{
    AR_PROFILE_FUNCTION();

    vkCmdDraw(m_CommandBuffer, vertex_count, instance_count, first_vertex, first_instance);
}

void CommandBuffer::DrawIndexed(uint32 index_count, uint32 instance_count, uint32 first_index, uint32 vertex_offset, uint32 first_instance)
{
    AR_PROFILE_FUNCTION();

    vkCmdDrawIndexed(m_CommandBuffer, index_count, instance_count, first_index, vertex_offset, first_instance);
}

void CommandBuffer::BindComputePipeline(RenderHandle pipeline)
{
    AR_PROFILE_FUNCTION();

    ComputePipeline* cp = m_Device->GetComputePipeline(pipeline);
    m_CurrentComputePipelineLayout = m_Device->GetPipelineLayout(cp->Info.PipelineLayout)->PipelineLayoutRaw;

    vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, cp->PipelineRaw);
}
void CommandBuffer::BindComputeDescriptorSet(uint32 set, RenderHandle descriptor_set, std::vector<uint32> dynamic_offsets)
{
    AR_PROFILE_FUNCTION();

    DescriptorSet* ds = m_Device->GetDescriptorSet(descriptor_set);
    vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_CurrentComputePipelineLayout, set, 1, &ds->DescriptorSetRaw, dynamic_offsets.size(), dynamic_offsets.data());
}

void CommandBuffer::Dispatch(uint32 group_count_x, uint32 group_count_y, uint32 group_count_z)
{
    AR_PROFILE_FUNCTION();

    vkCmdDispatch(m_CommandBuffer, group_count_x, group_count_y, group_count_z);
}

void CommandBuffer::GenerateTextureMips(RenderHandle texture, ResourceState source, ResourceState destination)
{
    AR_PROFILE_FUNCTION();

    // User must check that mips > 1
    TextureBarrier(texture, source, ResourceState::TransferDestination());
    m_Device->GenerateTextureMips(m_CommandBuffer, texture);
    TextureBarrier(texture, ResourceState::TransferSource(), destination);
}
void CommandBuffer::GenerateTextureMips(RenderHandle texture)
{
    AR_PROFILE_FUNCTION();

    // User must check that mips > 1
    m_Device->GenerateTextureMips(m_CommandBuffer, texture);
    // Hacky but needed for consistency
    TextureBarrier(texture, ResourceState::TransferSource(), ResourceState::TransferDestination());
}

void CommandBuffer::UploadBufferData(RenderHandle buffer, uint32 size, const void* data, ResourceState destination)
{
    AR_PROFILE_FUNCTION();

    m_Device->UploadBufferData(m_CommandBuffer, buffer, size, data);
    BufferBarrier(buffer, {RenderBindFlags::TransferDestination, PipelineStageFlags::Transfer}, destination);
}

void CommandBuffer::UploadTextureData(RenderHandle texture, uint32 size, const void* data, bool generate_mips, ResourceState destination)
{
    AR_PROFILE_FUNCTION();

    // Transition image layout for transfer
    TextureBarrier(texture, {RenderBindFlags::None, PipelineStageFlags::None}, {RenderBindFlags::TransferDestination, PipelineStageFlags::Transfer});

    m_Device->UploadTextureData(m_CommandBuffer, texture, size, data);
    // Current resource state is now
    // {RenderBindFlags::TransferDestination, PipelineStageFlags::Transfer}

    if (generate_mips && m_Device->GetTexture(texture)->Mips > 1)
    {
        m_Device->GenerateTextureMips(m_CommandBuffer, texture);
        // Current resource state is now
        // {RenderBindFlags::TransferSource, PipelineStageFlags::Transfer}
        
        TextureBarrier(texture, {RenderBindFlags::TransferSource, PipelineStageFlags::Transfer}, destination);
        
        return;
    }

    TextureBarrier(texture, {RenderBindFlags::TransferDestination, PipelineStageFlags::Transfer}, destination);
}
void CommandBuffer::UploadTextureData(RenderHandle texture, uint32 size, const void* data, bool generate_mips)
{
    AR_PROFILE_FUNCTION();

    m_Device->UploadTextureData(m_CommandBuffer, texture, size, data);
    // Current resource state is now
    // {RenderBindFlags::TransferDestination, PipelineStageFlags::Transfer}

    if (generate_mips && m_Device->GetTexture(texture)->Mips > 1)
    {
        m_Device->GenerateTextureMips(m_CommandBuffer, texture);
        // Current resource state is now
        // {RenderBindFlags::TransferSource, PipelineStageFlags::Transfer}
        
        // Hacky, but necessary for consistency (TODO: better solution)
        TextureBarrier(texture, ResourceState::TransferSource(), ResourceState::TransferDestination());
    }
}

void CommandBuffer::CopyTexture(RenderHandle source, RenderHandle destination)
{
    AR_PROFILE_FUNCTION();

    Texture* source_texture = m_Device->GetTexture(source);
    Texture* destination_texture = m_Device->GetTexture(destination);
    
    // Assume source
    VkImageCopy region = {};
    region.srcOffset = {0, 0, 0};
    region.dstOffset = {0, 0, 0};
    region.extent = {source_texture->Info.Width, source_texture->Info.Height, source_texture->Info.Depth};
    region.srcSubresource.aspectMask = Device::GetAspectFlagsFromFormat(source_texture->Info.Format);
    region.srcSubresource.baseArrayLayer = 0;
    region.srcSubresource.layerCount = source_texture->Info.Layers;
    region.srcSubresource.mipLevel = 0;
    region.dstSubresource.aspectMask = Device::GetAspectFlagsFromFormat(destination_texture->Info.Format);
    region.dstSubresource.baseArrayLayer = 0;
    region.dstSubresource.layerCount = destination_texture->Info.Layers;
    region.dstSubresource.mipLevel = 0;

    vkCmdCopyImage(m_CommandBuffer, source_texture->Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, destination_texture->Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void CommandBuffer::TextureBarrier(RenderHandle texture, ResourceState source, ResourceState destination, uint32 source_queue_family, uint32 destination_queue_family)
{
    AR_PROFILE_FUNCTION();

    Texture* tex = m_Device->GetTexture(texture);

    VulkanResourceState source_raw = RenderBackend::ConvertResourceState(source);
    VulkanResourceState destination_raw = RenderBackend::ConvertResourceState(destination);

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcAccessMask = source_raw.Access;
    barrier.dstAccessMask = destination_raw.Access;
    barrier.oldLayout = source_raw.ImageLayout;
    barrier.newLayout = destination_raw.ImageLayout;
    barrier.srcQueueFamilyIndex = source_queue_family;
    barrier.dstQueueFamilyIndex = destination_queue_family;
    barrier.image = tex->Image;
    barrier.subresourceRange.aspectMask = Device::GetAspectFlagsFromFormat(tex->Info.Format);
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = tex->Mips;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = tex->Info.Layers;

    VkPipelineStageFlags source_stage = source_raw.Stage;
    VkPipelineStageFlags destination_stage = destination_raw.Stage;
    if (source_stage == 0)
    {
        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    }
    if (destination_stage == 0)
    {
        destination_stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    }

    vkCmdPipelineBarrier(m_CommandBuffer, source_stage, destination_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}
void CommandBuffer::BufferBarrier(RenderHandle buffer, ResourceState source, ResourceState destination, uint32 source_queue_family, uint32 destination_queue_family)
{
    AR_PROFILE_FUNCTION();

    Buffer* buf = m_Device->GetBuffer(buffer);

    VulkanResourceState source_raw = RenderBackend::ConvertResourceState(source);
    VulkanResourceState destination_raw = RenderBackend::ConvertResourceState(destination);

    VkBufferMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier.srcAccessMask = source_raw.Access;
    barrier.dstAccessMask = destination_raw.Access;
    barrier.srcQueueFamilyIndex = source_queue_family;
    barrier.dstQueueFamilyIndex = destination_queue_family;
    barrier.buffer = buf->BufferRaw;
    barrier.offset = 0;
    barrier.size = buf->Info.Size;

    VkPipelineStageFlags source_stage = source_raw.Stage;
    VkPipelineStageFlags destination_stage = destination_raw.Stage;
    if (source_stage == 0)
    {
        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    }
    if (destination_stage == 0)
    {
        destination_stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    }

    vkCmdPipelineBarrier(m_CommandBuffer, source_stage, destination_stage, 0, 0, nullptr, 1, &barrier, 0, nullptr);
}

void CommandBuffer::ReleaseTextureQueueOwnership(QueueType source_queue, QueueType destination_queue, RenderHandle texture, ResourceState source, ResourceState destination)
{
    AR_PROFILE_FUNCTION();

    int32 source_family = m_Device->GetQueueFamilyIndex(source_queue);
    int32 destination_family = m_Device->GetQueueFamilyIndex(destination_queue);

    AR_CORE_ASSERT(source_family != -1, "Unsupported queue type");
    AR_CORE_ASSERT(destination_family != -1, "Unsupported queue type");

    Texture* tex = m_Device->GetTexture(texture);

    VulkanResourceState source_raw = RenderBackend::ConvertResourceState(source);
    VulkanResourceState destination_raw = RenderBackend::ConvertResourceState(destination);

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcAccessMask = source_raw.Access;
    barrier.dstAccessMask = 0;
    barrier.oldLayout = source_raw.ImageLayout;
    barrier.newLayout = destination_raw.ImageLayout;
    barrier.srcQueueFamilyIndex = source_family;
    barrier.dstQueueFamilyIndex = destination_family;
    barrier.image = tex->Image;
    barrier.subresourceRange.aspectMask = Device::GetAspectFlagsFromFormat(tex->Info.Format);
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = tex->Mips;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = tex->Info.Layers;

    VkPipelineStageFlags source_stage = source_raw.Stage;
    if (source_stage == 0)
    {
        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    }

    vkCmdPipelineBarrier(m_CommandBuffer, source_stage, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void CommandBuffer::AcquireTextureQueueOwnership(QueueType source_queue, QueueType destination_queue, RenderHandle texture, ResourceState source, ResourceState destination)
{
    AR_PROFILE_FUNCTION();

    int32 source_family = m_Device->GetQueueFamilyIndex(source_queue);
    int32 destination_family = m_Device->GetQueueFamilyIndex(destination_queue);

    AR_CORE_ASSERT(source_family != -1, "Unsupported queue type");
    AR_CORE_ASSERT(destination_family != -1, "Unsupported queue type");

    Texture* tex = m_Device->GetTexture(texture);

    VulkanResourceState source_raw = RenderBackend::ConvertResourceState(source);
    VulkanResourceState destination_raw = RenderBackend::ConvertResourceState(destination);

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = destination_raw.Access;
    barrier.oldLayout = source_raw.ImageLayout;
    barrier.newLayout = destination_raw.ImageLayout;
    barrier.srcQueueFamilyIndex = source_family;
    barrier.dstQueueFamilyIndex = destination_family;
    barrier.image = tex->Image;
    barrier.subresourceRange.aspectMask = Device::GetAspectFlagsFromFormat(tex->Info.Format);
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = tex->Mips;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = tex->Info.Layers;

    VkPipelineStageFlags destination_stage = destination_raw.Stage;
    if (destination_stage == 0)
    {
        destination_stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    }

    vkCmdPipelineBarrier(m_CommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, destination_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void CommandBuffer::ReleaseBufferQueueOwnership(QueueType source_queue, QueueType destination_queue, RenderHandle buffer, ResourceState source)
{
    AR_PROFILE_FUNCTION();

    int32 source_family = m_Device->GetQueueFamilyIndex(source_queue);
    int32 destination_family = m_Device->GetQueueFamilyIndex(destination_queue);

    AR_CORE_ASSERT(source_family != -1, "Unsupported queue type");
    AR_CORE_ASSERT(destination_family != -1, "Unsupported queue type");

    BufferBarrier(buffer, source, {RenderBindFlags::None, PipelineStageFlags::None}, source_family, destination_family);
}

void CommandBuffer::AcquireBufferQueueOwnership(QueueType source_queue, QueueType destination_queue, RenderHandle buffer, ResourceState destination)
{
    AR_PROFILE_FUNCTION();

    int32 source_family = m_Device->GetQueueFamilyIndex(source_queue);
    int32 destination_family = m_Device->GetQueueFamilyIndex(destination_queue);

    AR_CORE_ASSERT(source_family != -1, "Unsupported queue type");
    AR_CORE_ASSERT(destination_family != -1, "Unsupported queue type");

    BufferBarrier(buffer, {RenderBindFlags::None, PipelineStageFlags::None}, destination, source_family, destination_family);
}
