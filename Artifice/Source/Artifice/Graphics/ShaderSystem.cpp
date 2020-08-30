#include "ShaderSystem.h"


ShaderSystem::ShaderSystem(ShaderSystemLayout layout, Shader* shader, uint32 set)
{
    Reset(layout, shader, set);
}
void ShaderSystem::Reset(ShaderSystemLayout layout, Shader* shader, uint32 set)
{
    Shutdown();

    m_Layout = layout;
    m_Shader = shader;
    m_Set = set;


    for (auto& layout_binding : layout.m_Bindings)
    {
        if (layout_binding.Type == ShaderSystemLayout::Type::UniformBuffer)
        {
            UniformBuffer& res = m_UniformBuffers[layout_binding.Binding];

            res.Size = m_Shader->GetDevice()->GetAlignedUniformBufferSize(layout_binding.Size);

            BufferInfo ub;
            ub.Access = MemoryAccessType::Cpu;
            ub.BindFlags = RenderBindFlags::UniformBuffer;
            ub.Size = res.Size * AR_FRAME_COUNT;

            res.UniformBuffer = m_Shader->GetDevice()->CreateBuffer(ub);
            res.Pointer = m_Shader->GetDevice()->MapBuffer(res.UniformBuffer);

            res.Data.Allocate(res.Size);
        }
        else if (layout_binding.Type == ShaderSystemLayout::Type::InstancedUniformBuffer)
        {
            InstancedUniformBuffer& res = m_InstancedUniformBuffers[layout_binding.Binding];

            res.Size = m_Shader->GetDevice()->GetAlignedUniformBufferSize(layout_binding.Size);
            res.Capacity = layout_binding.Capacity;

            BufferInfo ub;
            ub.Access = MemoryAccessType::Cpu;
            ub.BindFlags = RenderBindFlags::UniformBuffer;
            ub.Size = res.Size * res.Capacity * AR_FRAME_COUNT;

            res.UniformBuffer = m_Shader->GetDevice()->CreateBuffer(ub);
            res.Pointer = m_Shader->GetDevice()->MapBuffer(res.UniformBuffer);

            res.Data.Allocate(res.Size * res.Capacity);
        }
        else if (layout_binding.Type == ShaderSystemLayout::Type::StorageBuffer)
        {
            StorageBuffer& res = m_StorageBuffers[layout_binding.Binding];

            res.Size = m_Shader->GetDevice()->GetAlignedStorageBufferSize(layout_binding.Size);
            res.Capacity = layout_binding.Capacity;

            BufferInfo sb;
            sb.Access = MemoryAccessType::Cpu;
            sb.BindFlags = RenderBindFlags::StorageBuffer;
            sb.Size = res.Size * res.Capacity * AR_FRAME_COUNT;

            res.StorageBuffer = m_Shader->GetDevice()->CreateBuffer(sb);
            res.Pointer = m_Shader->GetDevice()->MapBuffer(res.StorageBuffer);

            res.Data.Allocate(res.Size * res.Capacity);
        }
        else if (layout_binding.Type == ShaderSystemLayout::Type::CombinedTextureSampler)
        {
            // Initialize with default white texture/sampler
            // This ensures that subsequent calls to Update are valid
            // Kinda sketchy, as this might break a samplerCube, for example.
            CombinedTextureSampler& res = m_CombinedTextureSamplers[layout_binding.Binding];
            res.Count = layout_binding.Count;
            res.Textures.resize(res.Count, Renderer::GetWhiteTexture());
            res.Samplers.resize(res.Count, Renderer::GetWhiteSampler());
        }
        else if (layout_binding.Type == ShaderSystemLayout::Type::StorageTexture)
        {
            // Initialize with default white texture/sampler
            // This ensures that subsequent calls to Update are valid
            // Kinda sketchy, as this might break a samplerCube, for example.
            StorageTexture& res = m_StorageTextures[layout_binding.Binding];
            res.Count = layout_binding.Count;
            res.Textures.resize(res.Count);
        }
    }

    if (layout.m_PushConstantsRange > 0)
    {
        m_PushConstants.Data.Allocate(layout.m_PushConstantsRange);
        m_PushConstants.Stage = layout.m_PushConstantsStage;
        m_HasPushConstants = true;
    }

    //Update();
    m_NeedsUpdate = true;
}

void ShaderSystem::Shutdown()
{
    for (auto& pair : m_UniformBuffers)
    {
        m_Shader->GetDevice()->DestroyBuffer(pair.second.UniformBuffer);
    }
    for (auto& pair : m_InstancedUniformBuffers)
    {
        m_Shader->GetDevice()->DestroyBuffer(pair.second.UniformBuffer);
    }
    for (auto& pair : m_StorageBuffers)
    {
        m_Shader->GetDevice()->DestroyBuffer(pair.second.StorageBuffer);
    }
    if (!m_DescriptorSet.IsNull())
    {
        m_Shader->GetDevice()->DestroyDescriptorSet(m_DescriptorSet);
        m_DescriptorSet.SetNull();
    }
    m_UniformBuffers.clear();
    m_InstancedUniformBuffers.clear();
    m_CombinedTextureSamplers.clear();
    m_StorageTextures.clear();
    m_StorageBuffers.clear();
    m_PushConstants = {};
}

void ShaderSystem::WritePushConstants(void* data, uint32 size, uint32 offset)
{
    AR_PROFILE_FUNCTION();

    AR_CORE_ASSERT(m_HasPushConstants, "Tried using unregistered push constants");
    AR_CORE_ASSERT(size + offset <= m_PushConstants.Data.GetSize(), "Tried writing beyond push constant range");

    m_PushConstants.Data.Write(data, size, offset);
}

void ShaderSystem::WriteUniformBuffer(uint32 binding, void* data, uint32 size)
{
    AR_PROFILE_FUNCTION();

    AR_CORE_ASSERT(UniformBufferExists(binding), "Tried using unregistered binding (UniformBuffer)");

    UniformBuffer& bind = m_UniformBuffers[binding];

    // Copy data to CPU buffer
    bind.Data.Write(data, size, 0);

    // No update needed:
    // We always use the same buffer, so the descriptor set remains valid
}

void ShaderSystem::WriteInstancedUniformBuffer(uint32 binding, uint32 count, void* data, uint32 element_size)
{
    AR_PROFILE_FUNCTION();

    AR_CORE_ASSERT(InstancedUniformBufferExists(binding), "Tried using unregistered binding (InstancedUniformBuffer)");

    InstancedUniformBuffer& bind = m_InstancedUniformBuffers[binding];

    if (count > bind.Capacity)
    {
        if (!bind.UniformBuffer.IsNull())
        {
            // Deferred delete ensures this is safe
            m_Shader->GetDevice()->DestroyBuffer(bind.UniformBuffer);
        }

        bind.Capacity = count;

        BufferInfo ub;
        ub.Access = MemoryAccessType::Cpu;
        ub.BindFlags = RenderBindFlags::UniformBuffer;
        ub.Size = bind.Size * bind.Capacity * AR_FRAME_COUNT;

        bind.UniformBuffer = m_Shader->GetDevice()->CreateBuffer(ub);
        bind.Pointer = m_Shader->GetDevice()->MapBuffer(bind.UniformBuffer);

        bind.Data.Allocate(bind.Size * bind.Capacity);

        // Changing buffers needs a descriptor set update
        m_NeedsUpdate = true;
    }

    // Copy data to CPU buffer

    for (uint32 i = 0; i < count; i++)
    {
        bind.Data.Write((byte*)data + i * element_size, element_size, bind.Size * i);
    }
}

void ShaderSystem::WriteStorageBuffer(uint32 binding, uint32 count, void* data, uint32 element_size)
{
    AR_PROFILE_FUNCTION();

    AR_CORE_ASSERT(StorageBufferExists(binding), "Tried using unregistered binding (StorageBuffer)");

    StorageBuffer& bind = m_StorageBuffers[binding];

    if (count > bind.Capacity)
    {
        if (!bind.StorageBuffer.IsNull())
        {
            // Deferred delete ensures this is safe
            m_Shader->GetDevice()->DestroyBuffer(bind.StorageBuffer);
        }

        bind.Capacity = count;

        BufferInfo sb;
        sb.Access = MemoryAccessType::Cpu;
        sb.BindFlags = RenderBindFlags::StorageBuffer;
        sb.Size = bind.Size * bind.Capacity * AR_FRAME_COUNT;

        bind.StorageBuffer = m_Shader->GetDevice()->CreateBuffer(sb);
        bind.Pointer = m_Shader->GetDevice()->MapBuffer(bind.StorageBuffer);

        bind.Data.Allocate(bind.Size * bind.Capacity);

        // Changing buffers needs a descriptor set update
        m_NeedsUpdate = true;
    }

    // Copy data to CPU buffer

    for (uint32 i = 0; i < count; i++)
    {
        bind.Data.Write((byte*)data + i * element_size, element_size, bind.Size * i);
    }
}

void ShaderSystem::SetCombinedTextureSampler(uint32 binding, RenderHandle* textures, RenderHandle* samplers)
{
    AR_PROFILE_FUNCTION();

    AR_CORE_ASSERT(CombinedTextureSamplerExists(binding), "Tried using unregistered binding (CombinedTextureSampler)");

    uint32 count = m_CombinedTextureSamplers[binding].Count;

    memcpy(m_CombinedTextureSamplers[binding].Textures.data(), textures, count * sizeof(RenderHandle));
    memcpy(m_CombinedTextureSamplers[binding].Samplers.data(), samplers, count * sizeof(RenderHandle));


    // Setting a texture always triggers an update
    m_NeedsUpdate = true;
}
void ShaderSystem::SetStorageTexture(uint32 binding, RenderHandle* textures)
{
    AR_PROFILE_FUNCTION();

    AR_CORE_ASSERT(StorageTextureExists(binding), "Tried using unregistered binding (StorageTexture)");

    uint32 count = m_StorageTextures[binding].Count;

    memcpy(m_StorageTextures[binding].Textures.data(), textures, count * sizeof(RenderHandle));

    // Setting a texture always triggers an update
    m_NeedsUpdate = true;
}


void ShaderSystem::Bind(CommandBuffer* cmd, uint32 instance)
{
    AR_PROFILE_FUNCTION();

    // Only update at bind time to minimize update calls
    // Might not be best path but should be OK.
    if (m_NeedsUpdate)
    {
        //Update();
        m_Update = GetUpdate();
    }

    // Bind

    uint32 frame_index = m_Shader->GetDevice()->GetFrameIndex();

    // We need to:
    // 1. Copy data from CPU buffer to GPU buffer
    // 2. Calculate dynamic offsets for bind time
    // Note: Do these together, because copying data also requires knowledge of offsets
    // Copying has to be done at bind time, because we dynamically need the correct offsets
    // depending on the current frame index.  This way, we properly sync between data and frame.

    // Currently, this is pretty inefficient.

    std::map<uint32, uint32> offsets;

    for (auto& pair : m_UniformBuffers)
    {
        uint32 binding = pair.first;
        UniformBuffer& res = pair.second;

        uint32 offset = frame_index * res.Size;

        // 1.
        res.Data.Read((byte*)res.Pointer + offset, res.Size);
        // 2.
        offsets[binding] = offset;
    }
    for (auto& pair : m_InstancedUniformBuffers)
    {
        uint32 binding = pair.first;
        InstancedUniformBuffer& res = pair.second;

        uint32 frame_offset = frame_index * res.Size * res.Capacity;
        uint32 instance_offset = instance * res.Size;
        uint32 offset = frame_offset + instance_offset;

        // 1.
        res.Data.Read((byte*)res.Pointer + offset, res.Size, instance_offset);
        // 2.
        offsets[binding] = offset;
    }
    for (auto& pair : m_StorageBuffers)
    {
        uint32 binding = pair.first;
        StorageBuffer& res = pair.second;

        uint32 offset = frame_index * res.Size * res.Capacity;

        // 1.
        res.Data.Read((byte*)res.Pointer + offset, res.Size, offset);
        // 2.
        offsets[binding] = offset;
    }

    // Map -> Vector
    std::vector<uint32> dynamic_offsets;
    dynamic_offsets.reserve(offsets.size());
    for (auto& pair : offsets)
    {
        dynamic_offsets.push_back(pair.second);
    }

    // Bind descriptor set
    //cmd->BindGraphicsDescriptorSet(m_Set, m_DescriptorSet, dynamic_offsets);
    m_Shader->BindGraphicsDescriptorSet(cmd, m_Set, m_Update, dynamic_offsets);

    // Push constants
    if (m_HasPushConstants)
    {
        cmd->PushConstants(m_PushConstants.Stage, m_PushConstants.Data.GetSize(), 0, m_PushConstants.Data.Data);
    }
}

void ShaderSystem::BindCompute(CommandBuffer* cmd, uint32 instance)
{
    AR_PROFILE_FUNCTION();

    // Only update at bind time to minimize update calls
    // Might not be best path but should be OK.
    if (m_NeedsUpdate)
    {
        //Update();
        m_Update = GetUpdate();
    }

    // Bind

    uint32 frame_index = m_Shader->GetDevice()->GetFrameIndex();

    // We need to:
    // 1. Copy data from CPU buffer to GPU buffer
    // 2. Calculate dynamic offsets for bind time
    // Note: Do these together, because copying data also requires knowledge of offsets
    // Copying has to be done at bind time, because we dynamically need the correct offsets
    // depending on the current frame index.  This way, we properly sync between data and frame.

    // Currently, this is pretty inefficient.

    std::map<uint32, uint32> offsets;

    for (auto& pair : m_UniformBuffers)
    {
        uint32 binding = pair.first;
        UniformBuffer& res = pair.second;

        uint32 offset = frame_index * res.Size;

        // 1.
        res.Data.Read((byte*)res.Pointer + offset, res.Size);
        // 2.
        offsets[binding] = offset;
    }
    for (auto& pair : m_InstancedUniformBuffers)
    {
        uint32 binding = pair.first;
        InstancedUniformBuffer& res = pair.second;

        uint32 frame_offset = frame_index * res.Size * res.Capacity;
        uint32 instance_offset = instance * res.Size;
        uint32 offset = frame_offset + instance_offset;

        // 1.
        res.Data.Read((byte*)res.Pointer + offset, res.Size, instance_offset);
        // 2.
        offsets[binding] = offset;
    }
    for (auto& pair : m_StorageBuffers)
    {
        uint32 binding = pair.first;
        StorageBuffer& res = pair.second;

        uint32 offset = frame_index * res.Size * res.Capacity;

        // 1.
        res.Data.Read((byte*)res.Pointer + offset, res.Size * res.Capacity);
        // 2.
        offsets[binding] = offset;
    }

    // Map -> Vector
    std::vector<uint32> dynamic_offsets;
    dynamic_offsets.reserve(offsets.size());
    for (auto& pair : offsets)
    {
        dynamic_offsets.push_back(pair.second);
    }

    // Bind descriptor set
    //cmd->BindComputeDescriptorSet(m_Set, m_DescriptorSet, dynamic_offsets);
    m_Shader->BindComputeDescriptorSet(cmd, m_Set, m_Update, dynamic_offsets);

    // Push constants
    if (m_HasPushConstants)
    {
        cmd->PushConstants(m_PushConstants.Stage, m_PushConstants.Data.GetSize(), 0, m_PushConstants.Data.Data);
    }
}

DescriptorSetUpdate ShaderSystem::GetUpdate() const
{
    // Update descriptor set
    DescriptorSetUpdate ds;
    for (auto& pair : m_UniformBuffers)
    {
        ds.AddUniformBufferDynamic(pair.first, pair.second.UniformBuffer, 0, pair.second.Size);
    }
    for (auto& pair : m_InstancedUniformBuffers)
    {
        ds.AddUniformBufferDynamic(pair.first, pair.second.UniformBuffer, 0, pair.second.Size);
    }
    for (auto& pair : m_StorageBuffers)
    {
        ds.AddStorageBufferDynamic(pair.first, pair.second.StorageBuffer, 0, pair.second.Size);
    }
    for (auto& pair : m_CombinedTextureSamplers)
    {
        std::vector<DescriptorSetUpdate::ImageUpdate> images(pair.second.Count);
        for (uint32 i = 0; i < pair.second.Count; i++)
        {
            images[i] = {pair.second.Textures[i], RenderBindFlags::SampledTexture, pair.second.Samplers[i]};
        }
        ds.Add(pair.first, DescriptorType::CombinedImageSampler, pair.second.Count, 0, images);
    }
    for (auto& pair : m_StorageTextures)
    {
        std::vector<DescriptorSetUpdate::ImageUpdate> images(pair.second.Count);
        for (uint32 i = 0; i < pair.second.Count; i++)
        {
            images[i] = {pair.second.Textures[i], RenderBindFlags::StorageTexture};
        }
        ds.Add(pair.first, DescriptorType::StorageImage, pair.second.Count, 0, images);
    }
    
    return ds;
}

void ShaderSystem::Update()
{
    AR_PROFILE_FUNCTION();

    // Update descriptor set
    DescriptorSetUpdate ds;
    for (auto& pair : m_UniformBuffers)
    {
        ds.AddUniformBufferDynamic(pair.first, pair.second.UniformBuffer, 0, pair.second.Size);
    }
    for (auto& pair : m_InstancedUniformBuffers)
    {
        ds.AddUniformBufferDynamic(pair.first, pair.second.UniformBuffer, 0, pair.second.Size);
    }
    for (auto& pair : m_StorageBuffers)
    {
        ds.AddStorageBufferDynamic(pair.first, pair.second.StorageBuffer, 0, pair.second.Size);
    }
    for (auto& pair : m_CombinedTextureSamplers)
    {
        std::vector<DescriptorSetUpdate::ImageUpdate> images(pair.second.Count);
        for (uint32 i = 0; i < pair.second.Count; i++)
        {
            images[i] = {pair.second.Textures[i], RenderBindFlags::SampledTexture, pair.second.Samplers[i]};
        }
        ds.Add(pair.first, DescriptorType::CombinedImageSampler, pair.second.Count, 0, images);
    }
    for (auto& pair : m_StorageTextures)
    {
        std::vector<DescriptorSetUpdate::ImageUpdate> images(pair.second.Count);
        for (uint32 i = 0; i < pair.second.Count; i++)
        {
            images[i] = {pair.second.Textures[i], RenderBindFlags::StorageTexture};
        }
        ds.Add(pair.first, DescriptorType::StorageImage, pair.second.Count, 0, images);
    }
    if (!m_DescriptorSet.IsNull())
    {
        m_Shader->GetDevice()->DestroyDescriptorSet(m_DescriptorSet);
    }
    m_DescriptorSet = m_Shader->AllocateUpdatedDescriptorSet(m_Set, ds);

    m_NeedsUpdate = false;
}