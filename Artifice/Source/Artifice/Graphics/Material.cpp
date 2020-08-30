#include "Material.h"

#if 0

Material::Material(Shader* shader) : m_Shader(shader)
{
    PipelineLayout* layout = shader->m_Device->GetPipelineLayout(m_Shader->m_PipelineLayout);
    m_DescriptorSetUpdates.resize(layout->Info.DescriptorSetLayoutCount);
    m_DescriptorSets.resize(layout->Info.DescriptorSetLayoutCount);
    if (shader->m_Type == ShaderType::Compute)
    {
        m_ComputePipeline = m_Shader->CreateComputePipeline();
    }
}

Material::Material(Shader* shader, MaterialPipelineState state) : m_Shader(shader), m_MaterialPipelineState(state)
{
    PipelineLayout* layout = shader->m_Device->GetPipelineLayout(m_Shader->m_PipelineLayout);
    m_DescriptorSetUpdates.resize(layout->Info.DescriptorSetLayoutCount);
    m_DescriptorSets.resize(layout->Info.DescriptorSetLayoutCount);

}

void Material::CleanUp()
{
    for (auto mat : m_MaterialInstances)
    {
        mat->CleanUp();
    }
    for (auto set : m_DescriptorSets)
    {
        if (!set.IsNull())
        {
            m_Shader->m_Device->DestroyDescriptorSet(set);
        }
    }

    if (!m_ComputePipeline.IsNull())
    {
        if (m_Shader->m_Type == ShaderType::Compute)
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
    m_DescriptorSetUpdates.clear();
}
void Material::UpdateDescriptorSet(uint32 set, const DescriptorSetUpdate& update)
{
    if (!m_DescriptorSets[set].IsNull())
    {
        m_Shader->m_Device->DestroyDescriptorSet(m_DescriptorSets[set]);
    }
    m_DescriptorSets[set] = m_Shader->AllocateUpdatedDescriptorSet(set, update);
    m_DescriptorSetUpdates[set] = update;
}

RenderHandle Material::RequestPassPipeline(RenderPassLayout layout)
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
        gp.VertexLayout = m_MaterialPipelineState.VertexLayout;
        gp.VertexShaderModule = m_Shader->m_ShaderModules[(uint8)ShaderStage::Vertex];
        gp.FragmentShaderModule = m_Shader->m_ShaderModules[(uint8)ShaderStage::Fragment];
        gp.PipelineLayout = m_Shader->m_PipelineLayout;
        gp.PrimitiveType = m_MaterialPipelineState.PrimitiveType;
        gp.CullMode = m_MaterialPipelineState.CullMode;
        gp.FrontFace = m_MaterialPipelineState.FrontFace;
        gp.Samples = m_MaterialPipelineState.Samples;
        gp.DepthTestEnabled = m_MaterialPipelineState.DepthTestEnabled;
        gp.DepthWriteEnabled = m_MaterialPipelineState.DepthWriteEnabled;
        gp.BlendEnabled = m_MaterialPipelineState.BlendEnabled;
        gp.AdditiveBlendEnabled = m_MaterialPipelineState.AdditiveBlendEnabled;
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
#endif