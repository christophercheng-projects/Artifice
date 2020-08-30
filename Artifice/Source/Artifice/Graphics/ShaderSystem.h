#pragma once

#include <vector>
#include <map>

#include "ResourceCaches.h"
#include "RenderHandle.h"
#include "Resources.h"
#include "Shader.h"

#include "Artifice/Debug/Instrumentor.h"
#include "Artifice/Core/Core.h"
#include "Artifice/Core/DataBuffer.h"
#include "Artifice/math/math.h"
#include "Artifice/Graphics/Renderer.h"


// Currently, all uniform buffers are dynamic
// No storage buffers yet
// No arrays of textures or other resources yet

class ShaderSystemLayout
{
    friend class ShaderSystem;

private:
    enum class Type
    {
        UniformBuffer,
        InstancedUniformBuffer,
        StorageBuffer,
        CombinedTextureSampler,
        StorageTexture
    };
    struct Binding
    {
        uint32 Binding;
        Type Type;
        uint32 Count;
        // For uniform buffers
        uint32 Size;
        // For instanced uniform buffers and storage buffers
        uint32 Capacity;
    };

    std::vector<Binding> m_Bindings;
    
    ShaderStage m_PushConstantsStage;
    uint32 m_PushConstantsRange = 0;

public:
    ShaderSystemLayout() = default;

    template <class P>
    void RegisterPushConstants(ShaderStage stage)
    {
        m_PushConstantsStage = stage;
        m_PushConstantsRange = sizeof(P);
    }

    // TODO: array of descriptors (esp. textures)

    template <class U>
    void RegisterUniformBuffer(uint32 binding)
    {
        m_Bindings.push_back({binding, Type::UniformBuffer, 1, sizeof(U)});
    }
    template <class U, uint32 COUNT = 1>
    void RegisterStorageBuffer(uint32 binding)
    {
        m_Bindings.push_back({binding, Type::StorageBuffer, 1, sizeof(U), math::max(1u, COUNT)});
    }
    template <class U, uint32 COUNT = 1>
    void RegisterInstancedUniformBuffer(uint32 binding)
    {
        m_Bindings.push_back({binding, Type::InstancedUniformBuffer, 1, sizeof(U), math::max(1u, COUNT)});
    }
    void RegisterCombinedTextureSampler(uint32 binding, uint32 count = 1)
    {
        m_Bindings.push_back({binding, Type::CombinedTextureSampler, count});
    }
    void RegisterStorageTexture(uint32 binding, uint32 count = 1)
    {
        m_Bindings.push_back({binding, Type::StorageTexture, count});
    }
};


class ShaderSystem
{
private:
    ShaderSystemLayout m_Layout;

    Shader* m_Shader;

    uint32 m_Set;
    RenderHandle m_DescriptorSet;

    // Uniform buffers
    struct UniformBuffer
    {
        uint32 Size;
        RenderHandle UniformBuffer;
        void* Pointer;

        DataBuffer Data;
    };
    std::map<uint32, UniformBuffer> m_UniformBuffers;

    // Instanced uniform buffers
    struct InstancedUniformBuffer
    {
        uint32 Size;
        RenderHandle UniformBuffer;
        void* Pointer;
        uint32 Capacity;

        DataBuffer Data;
    };
    std::map<uint32, InstancedUniformBuffer> m_InstancedUniformBuffers;

    // Storage buffers
    struct StorageBuffer
    {
        uint32 Size;
        RenderHandle StorageBuffer;
        void* Pointer;
        uint32 Capacity;

        DataBuffer Data;
    };
    std::map<uint32, StorageBuffer> m_StorageBuffers;

    // Combined texture samplers
    struct CombinedTextureSampler
    {
        std::vector<RenderHandle> Textures;
        std::vector<RenderHandle> Samplers;

        uint32 Count;
    };
    std::map<uint32, CombinedTextureSampler> m_CombinedTextureSamplers;

    // Storage textures
    struct StorageTexture
    {
        std::vector<RenderHandle> Textures;

        uint32 Count;
    };
    std::map<uint32, StorageTexture> m_StorageTextures;

    // Push constants
    struct PushConstants
    {
        DataBuffer Data;
        ShaderStage Stage;
    };

    PushConstants m_PushConstants;
    bool m_HasPushConstants = false;

    bool m_NeedsUpdate = false;
    DescriptorSetUpdate m_Update;

public:
    ShaderSystem() = default;
    ShaderSystem(ShaderSystemLayout layout, Shader* shader, uint32 set);

    void Reset(ShaderSystemLayout layout, Shader* shader, uint32 set);

    void Shutdown();

    template <class P>
    void SetPushConstants(P data, uint32 offset = 0)
    {
        WritePushConstants(&data, sizeof(P), offset);
    }

    template <class U>
    void SetUniformBuffer(uint32 binding, U data)
    {
        WriteUniformBuffer(binding, &data, sizeof(U));
    }
    template <class U>
    void SetStorageBuffer(uint32 binding, uint32 count, U* data)
    {
        WriteStorageBuffer(binding, count, data, sizeof(U));
    }

    template <class U>
    void SetInstancedUniformBuffer(uint32 binding, uint32 count, U* data)
    {
        WriteInstancedUniformBuffer(binding, count, data, sizeof(U));
    }

    template <class U>
    void SetInstancedUniformBuffer(uint32 binding, std::vector<U> data)
    {
        WriteInstancedUniformBuffer(binding, data.size(), data.data(), sizeof(U));
    }

    void SetCombinedTextureSampler(uint32 binding, RenderHandle* textures, RenderHandle* samplers);

    void SetCombinedTextureSampler(uint32 binding, std::vector<RenderHandle> textures, std::vector<RenderHandle> samplers)
    {
        AR_CORE_ASSERT(m_CombinedTextureSamplers[binding].Count == textures.size() && textures.size() == samplers.size(), "");

        SetCombinedTextureSampler(binding, textures.data(), samplers.data());
    }

    void SetCombinedTextureSampler(uint32 binding, RenderHandle texture, RenderHandle sampler)
    {
        SetCombinedTextureSampler(binding, &texture, &sampler);
    }

    void SetStorageTexture(uint32 binding, RenderHandle* textures);

    void SetStorageTexture(uint32 binding, std::vector<RenderHandle> textures)
    {
        AR_CORE_ASSERT(m_CombinedTextureSamplers[binding].Count == textures.size(), "");

        SetStorageTexture(binding, textures.data());
    }

    void SetStorageTexture(uint32 binding, RenderHandle texture)
    {
        SetStorageTexture(binding, &texture);
    }

    void Bind(CommandBuffer* cmd, uint32 instance = 0);
    void BindCompute(CommandBuffer* cmd, uint32 instance = 0);

    DescriptorSetUpdate GetUpdate() const;

private:
    // Internal methods to write to CPU-side buffers
    void WritePushConstants(void* data, uint32 size, uint32 offset);
    void WriteUniformBuffer(uint32 binding, void* data, uint32 size);
    void WriteInstancedUniformBuffer(uint32 binding, uint32 count, void* data, uint32 element_size);
    void WriteStorageBuffer(uint32 binding, uint32 count, void* data, uint32 element_size);
    // Update descriptor set
    void Update();

private: // Debug helpers for asserts
    bool CombinedTextureSamplerExists(uint32 binding) const
    {
        return m_CombinedTextureSamplers.find(binding) != m_CombinedTextureSamplers.end();
    }
    bool StorageTextureExists(uint32 binding) const
    {
        return m_StorageTextures.find(binding) != m_StorageTextures.end();
    }
    bool UniformBufferExists(uint32 binding) const
    {
        return m_UniformBuffers.find(binding) != m_UniformBuffers.end();
    }
    bool InstancedUniformBufferExists(uint32 binding) const
    {
        return m_InstancedUniformBuffers.find(binding) != m_InstancedUniformBuffers.end();
    }
    bool StorageBufferExists(uint32 binding) const
    {
        return m_StorageBuffers.find(binding) != m_StorageBuffers.end();
    }
};