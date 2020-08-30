#include "Shader.h"


Shader::Shader(Device* device, const std::string& vertex_source, const std::string& fragment_source, Layout layout) : m_Device(device)
{
    ShaderModuleInfo vert_info;
    vert_info.Stage = ShaderStage::Vertex;
    vert_info.Code = ReadFile(vertex_source);

    ShaderModuleInfo frag_info;
    frag_info.Stage = ShaderStage::Fragment;
    frag_info.Code = ReadFile(fragment_source);

    SetModule(vert_info);
    SetModule(frag_info);
    SetLayout(layout);
    SetType(ShaderType::Graphics);
    
}

Shader::Shader(Device* device, const std::string& compute_source, Layout layout) : m_Device(device)
{
    ShaderModuleInfo comp_info;
    comp_info.Stage = ShaderStage::Compute;
    comp_info.Code = ReadFile(compute_source);

    SetModule(comp_info);
    SetLayout(layout);
    SetType(ShaderType::Compute);

}

void Shader::CleanUp()
{
    for (uint32 i = 0; i < (uint8)ShaderStage::Count; i++)
    {
        if (!m_ShaderModules[i].IsNull())
        {
            m_Device->DestroyShaderModule(m_ShaderModules[i]);
            m_ShaderModules[i] = RenderHandle();
        }
    }
    if (!m_PipelineLayout.IsNull())
    {
        m_Device->DestroyPipelineLayout(m_PipelineLayout);
        m_PipelineLayout = RenderHandle();
    }

}

void Shader::SetType(ShaderType type)
{
    m_Type = type;
}

void Shader::SetModule(ShaderModuleInfo info)
{
    AR_CORE_ASSERT(m_ShaderModules[(uint8)info.Stage].IsNull(), "Tried setting shader module that was already set");

    m_ShaderModules[(uint8)info.Stage] = m_Device->CreateShaderModule(info);
}

void Shader::SetLayout(Layout layout)
{
    AR_CORE_ASSERT(m_PipelineLayout.IsNull(), "Tried setting shader layout that was already set");

    PipelineLayoutInfo info;
    info.DescriptorSetLayoutCount = layout.DescriptorSetLayoutCount;
    for (uint32 i = 0; i < layout.DescriptorSetLayoutCount; i++)
    {
        info.DescriptorSetLayouts[i] = layout.DescriptorSetLayouts[i];
    }
    info.PushConstantRangeCount = layout.PushConstantRangeCount;
    for (uint32 i = 0; i < layout.PushConstantRangeCount; i++)
    {
        info.PushConstantRanges[i] = layout.PushConstantRanges[i];
    }

    m_PipelineLayout = m_Device->CreatePipelineLayout(info);
    m_Layout = layout;
}

RenderHandle Shader::AllocateDescriptorSet(uint32 set) const
{
    PipelineLayout* layout = m_Device->GetPipelineLayout(m_PipelineLayout);
    return m_Device->CreateDescriptorSet(layout->Info.DescriptorSetLayouts[set]);
}

std::vector<RenderHandle> Shader::AllocateDescriptorSets() const
{
    std::vector<RenderHandle> result;

    PipelineLayout* layout = m_Device->GetPipelineLayout(m_PipelineLayout);
    for (uint32 i = 0; i < layout->Info.DescriptorSetLayoutCount; i++)
    {
        RenderHandle descriptor_set = m_Device->CreateDescriptorSet(layout->Info.DescriptorSetLayouts[i]);
        result.push_back(descriptor_set);
    }

    return result;
}

uint32 Shader::GetDescriptorSetCount() const
{
    return m_Device->GetPipelineLayout(m_PipelineLayout)->Info.DescriptorSetLayoutCount;
}

RenderHandle Shader::AllocateUpdatedDescriptorSet(uint32 set, DescriptorSetUpdate update) const
{
    AR_PROFILE_FUNCTION();
    
    PipelineLayout* layout = m_Device->GetPipelineLayout(m_PipelineLayout);
    RenderHandle result = m_Device->CreateDescriptorSet(layout->Info.DescriptorSetLayouts[set]);
    m_Device->UpdateDescriptorSet(result, update);

    return result;
}

RenderHandle Shader::CreateComputePipeline() const
{
    ComputePipelineInfo cp;
    cp.ComputeShaderModule = m_ShaderModules[(uint8)ShaderStage::Compute];
    cp.PipelineLayout = m_PipelineLayout;

    return m_Device->CreateComputePipeline(cp);
}