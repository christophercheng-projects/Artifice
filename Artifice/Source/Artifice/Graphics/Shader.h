#pragma once

#include <vector>
#include <string>

#include "Artifice/Core/Core.h"
#include "Artifice/Utils/FileUtils.h"

#include "Artifice/Graphics/Resources.h"
#include "Artifice/Graphics/RenderHandle.h"

#include "Artifice/Graphics/Device.h"
#include "Artifice/Graphics/DescriptorSetCache.h"


enum class ShaderType
{
    Graphics,
    Compute
};

class Shader
{
    friend class ShaderInstance;

public:
    struct Layout
    {
        DescriptorSetLayout DescriptorSetLayouts[AR_MAX_DESCRIPTOR_SETS];
        uint32 DescriptorSetLayoutCount;
        PushConstantRange PushConstantRanges[AR_MAX_PUSH_CONSTANT_RANGES];
        uint32 PushConstantRangeCount;
    };

private:
    Device* m_Device;

    RenderHandle m_ShaderModules[(uint8)ShaderStage::Count] = {};
    RenderHandle m_PipelineLayout;

    Layout m_Layout;

    ShaderType m_Type;

public:
    Shader() = default;
    Shader(Device* device) : m_Device(device) {}
    Shader(Device* device, const std::string& vertex_source, const std::string& fragment_source, Layout layout);
    Shader(Device* device, const std::string& compute_source, Layout layout);

    Device* GetDevice() { return m_Device; }

    void CleanUp();

    void SetType(ShaderType type);
    void SetModule(ShaderModuleInfo info);
    void SetLayout(Layout layout);
    // allocate a descriptor set with this shader layout
    // on the backend side, descriptor set allocators are associated with descriptor set layouts
    RenderHandle AllocateDescriptorSet(uint32 set) const;
    std::vector<RenderHandle> AllocateDescriptorSets() const;
    RenderHandle AllocateUpdatedDescriptorSet(uint32 set, DescriptorSetUpdate update) const;
    
    RenderHandle CreateComputePipeline() const;

    ShaderType GetType() const { return m_Type; }
    uint32 GetDescriptorSetCount() const;
private:
public:

    // Transient descriptor sets.
    // TODO: cache these
    void BindGraphicsDescriptorSet(CommandBuffer* cmd, uint32 set, DescriptorSetUpdate update, std::vector<uint32> dynamic_offsets = {})
    {
        RenderHandle ds = m_Device->GetDescriptorSetCache()->Request({m_Layout.DescriptorSetLayouts[set], update});
        cmd->BindGraphicsDescriptorSet(set, ds, dynamic_offsets);
    }
    void BindComputeDescriptorSet(CommandBuffer* cmd, uint32 set, DescriptorSetUpdate update, std::vector<uint32> dynamic_offsets = {})
    {
        RenderHandle ds = m_Device->GetDescriptorSetCache()->Request({m_Layout.DescriptorSetLayouts[set], update});
        cmd->BindComputeDescriptorSet(set, ds, dynamic_offsets);
    }
};

struct ShaderPipelineState
{
    VertexLayout VertexLayout;

    VkPrimitiveTopology PrimitiveType = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkCullModeFlags CullMode = VK_CULL_MODE_NONE;
    VkFrontFace FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    VkSampleCountFlagBits Samples = VK_SAMPLE_COUNT_1_BIT;
    bool DepthTestEnabled = true;
    bool DepthWriteEnabled = true;
    bool BlendEnabled = false;
    bool AdditiveBlendEnabled = false;

    bool operator==(const ShaderPipelineState& other) const
    {
        if (VertexLayout != other.VertexLayout)
            return false;
        if (PrimitiveType != other.PrimitiveType)
            return false;
        if (CullMode != other.CullMode)
            return false;
        if (FrontFace != other.FrontFace)
            return false;
        if (Samples != other.Samples)
            return false;
        if (DepthTestEnabled != other.DepthTestEnabled)
            return false;
        if (DepthWriteEnabled != other.DepthWriteEnabled)
            return false;
        if (BlendEnabled != other.BlendEnabled)
            return false;
        if (AdditiveBlendEnabled != other.AdditiveBlendEnabled)
            return false;

        return true;
    }
    bool operator!=(const ShaderPipelineState& other) const
    {
        return !(*this == other);
    }
    bool operator<(const ShaderPipelineState& other) const
    {
        if (VertexLayout != other.VertexLayout)
            return VertexLayout < other.VertexLayout;
        if (PrimitiveType != other.PrimitiveType)
            return PrimitiveType < other.PrimitiveType;
        if (CullMode != other.CullMode)
            return CullMode < other.CullMode;
        if (FrontFace != other.FrontFace)
            return FrontFace < other.FrontFace;
        if (Samples != other.Samples)
            return Samples < other.Samples;
        if (DepthTestEnabled != other.DepthTestEnabled)
            return DepthTestEnabled < other.DepthTestEnabled;
        if (DepthWriteEnabled != other.DepthWriteEnabled)
            return DepthWriteEnabled < other.DepthWriteEnabled;
        if (BlendEnabled != other.BlendEnabled)
            return BlendEnabled < other.BlendEnabled;
        if (AdditiveBlendEnabled != other.AdditiveBlendEnabled)
            return AdditiveBlendEnabled < other.AdditiveBlendEnabled;

        return false;
    }
};

class ShaderInstance
{
private:
    Shader* m_Shader;

    // Compute
    RenderHandle m_ComputePipeline;

    // Graphics
    ShaderPipelineState m_PipelineState;
    std::vector<RenderHandle> m_PassPipelines;
    std::vector<RenderPassLayout> m_PassLayouts;
public:
    ShaderInstance() = default;

     // Compute
    ShaderInstance(Shader* shader) 
        : m_Shader(shader)
    {
        AR_CORE_ASSERT(m_Shader->GetType() == ShaderType::Compute, "");
        
        m_ComputePipeline = m_Shader->CreateComputePipeline();
    }
    // Graphics
    ShaderInstance(Shader* shader, ShaderPipelineState state)
        : m_Shader(shader), m_PipelineState(state)
    {
        AR_CORE_ASSERT(m_Shader->GetType() == ShaderType::Graphics, "");
    }

    void CleanUp()
    {
        if (!m_ComputePipeline.IsNull())
        {
            if (m_Shader->GetType() == ShaderType::Compute)
            {
                m_Shader->m_Device->DestroyComputePipeline(m_ComputePipeline);
            }
        }

        for (uint32 i = 0; i < m_PassPipelines.size(); i++)
        {
            m_Shader->m_Device->DestroyGraphicsPipeline(m_PassPipelines[i]);
        }
        m_PassPipelines.clear();
        m_PassLayouts.clear();

        m_ComputePipeline = RenderHandle();
    }

    void BindComputePipeline(CommandBuffer* cmd)
    {
        cmd->BindComputePipeline(RequestComputePipeline());
    }

    void BindGraphicsPipeline(CommandBuffer* cmd, RenderPassLayout rpl)
    {
        cmd->BindGraphicsPipeline(RequestPassPipeline(rpl));
    }

     // Compute
    RenderHandle RequestComputePipeline()
    {
        return m_ComputePipeline;
    }
    // Graphics
    RenderHandle RequestPassPipeline(RenderPassLayout layout)
    {
        int32 index = -1;
        for (int32 i = 0; i < m_PassLayouts.size(); i++)
        {
            if (layout == m_PassLayouts[i])
            {
                index = i;
            }
        }

        if (index == -1)
        {
            GraphicsPipelineInfo gp;
            gp.VertexLayout = m_PipelineState.VertexLayout;
            gp.VertexShaderModule = m_Shader->m_ShaderModules[(uint8)ShaderStage::Vertex];
            gp.FragmentShaderModule = m_Shader->m_ShaderModules[(uint8)ShaderStage::Fragment];
            gp.PipelineLayout = m_Shader->m_PipelineLayout;
            gp.PrimitiveType = m_PipelineState.PrimitiveType;
            gp.CullMode = m_PipelineState.CullMode;
            gp.FrontFace = m_PipelineState.FrontFace;
            gp.Samples = m_PipelineState.Samples;
            gp.DepthTestEnabled = m_PipelineState.DepthTestEnabled;
            gp.DepthWriteEnabled = m_PipelineState.DepthWriteEnabled;
            gp.BlendEnabled = m_PipelineState.BlendEnabled;
            gp.AdditiveBlendEnabled = m_PipelineState.AdditiveBlendEnabled;
            gp.RenderPassLayout = layout;

            RenderHandle pipeline = m_Shader->m_Device->CreateGraphicsPipeline(gp);

            m_PassPipelines.push_back(pipeline);
            m_PassLayouts.push_back(layout);

            return pipeline;
        }
        else
        {
            return m_PassPipelines[index];
        }
    }
};


class ShaderLibrary
{
private:
    Device* m_Device;
    std::unordered_map<std::string, Shader*> m_Shaders;
public:
    ShaderLibrary() = default;
    ShaderLibrary(Device* device) : m_Device(device) {}
    void CleanUp()
    {
        for (auto& shader : m_Shaders)
        {
            shader.second->CleanUp();
            delete shader.second;
        }
        m_Shaders.clear();
    }
    Shader* Load(const std::string& name, const std::string& vertex_path, const std::string& fragment_path, Shader::Layout layout)
    {
        Add(name, new Shader(m_Device, vertex_path, fragment_path, layout));
        return Get(name);
    }
    Shader* Load(const std::string& name, const std::string& compute_path, Shader::Layout layout)
    {
        Add(name, new Shader(m_Device, compute_path, layout));
        return Get(name);
    }
    Shader* Get(const std::string& name)
    {
        AR_CORE_ASSERT(Exists(name), "Shader not found!");
        return m_Shaders[name];
    }
private:
    bool Exists(const std::string& name) const
    {
        return m_Shaders.find(name) != m_Shaders.end();
    }
    void Add(std::string name, Shader* shader)
    {
        AR_CORE_ASSERT(!Exists(name), "Shader exists already!");
        m_Shaders[name] = shader;
    }
};