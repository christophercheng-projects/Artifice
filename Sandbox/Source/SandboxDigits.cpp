#include "SandboxDigits.h"

#include <imgui.h>
#include <stb_image.h>



SandboxDigits::SandboxDigits()
{
}

void SandboxDigits::OnAttach()
{
    AR_PROFILE_FUNCTION();

    m_Options.Color = {1.0f, 1.0f, 1.0f, 1.0f};
    m_Options.Size = 10.0f;

    Device* device = Application::Get()->GetRenderBackend()->GetDevice();

    TextureInfo tex;
    tex.Type = TextureType::Texture2D;
    tex.BindFlags = RenderBindFlags::SampledTexture | RenderBindFlags::StorageTexture;
    tex.Width = 1600;
    tex.Height = 900;
    tex.Format = VK_FORMAT_R16G16B16A16_SFLOAT;
    m_StorageTexture = device->CreateTexture(tex);

    RenderGraph graph(device);
    graph.AddPass("Transition", [&](RenderGraphBuilder* builder) {

        builder->SetQueue(QueueType::Universal);
        builder->SetType(RenderGraphPassType::Other);

        builder->ImportTexture("Storage", m_StorageTexture, ResourceState::None());
        builder->ReadWriteTexture("Storage", ResourceState::SampledFragment());
        
        return [](RenderGraphRegistry* registry, CommandBuffer* cmd) {

        };
    });
    graph.Compile();
    graph.Evaluate();
    graph.Reset();

    m_ShaderLibrary = ShaderLibrary(device);

    {
        Shader::Layout layout;
        layout.PushConstantRangeCount = 1;
        layout.PushConstantRanges[0] = {
            ShaderStage::Compute, sizeof(DigitsPushConstants), 0
        };
        layout.DescriptorSetLayoutCount = 1;
        layout.DescriptorSetLayouts[0] = {
            {0, DescriptorType::StorageBufferDynamic, 1, ShaderStage::Compute},
            {1, DescriptorType::StorageImage, 1, ShaderStage::Compute},
        };

        m_ShaderLibrary.Load("Digits", "Assets/Shaders/Digits.comp.spv", layout);
    }
    {
        ShaderSystemLayout layout;
        layout.RegisterStorageBuffer<DigitsStorageBuffer>(0);
        layout.RegisterStorageTexture(1);
        layout.RegisterPushConstants<DigitsPushConstants>(ShaderStage::Compute);

        m_ShaderSystem = ShaderSystem(layout, m_ShaderLibrary.Get("Digits"), 0);
    }

    m_ShaderInstance = ShaderInstance(m_ShaderLibrary.Get("Digits"));

    m_ShaderSystem.SetStorageTexture(1, m_StorageTexture);
}

void SandboxDigits::OnDetach()
{
    AR_PROFILE_FUNCTION();

    Device* device = Application::Get()->GetRenderBackend()->GetDevice();
    device->DestroyTexture(m_StorageTexture);

    m_ShaderInstance.CleanUp();
    m_ShaderSystem.Shutdown();
    m_ShaderLibrary.CleanUp();
}

void SandboxDigits::OnConstructRenderGraph(RenderGraph* graph)
{
    AR_PROFILE_FUNCTION();

    const uint32 workgroup_size = 32;

    Device* device = Application::Get()->GetRenderBackend()->GetDevice();

    graph->AddPass("Import", [&](RenderGraphBuilder* builder) {

        builder->SetQueue(QueueType::Universal);
        builder->SetType(RenderGraphPassType::Other);

        builder->ImportTexture("Color", m_StorageTexture, ResourceState::SampledFragment());

        return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {

        };
    });
    if (m_MouseDown || m_Options.Clear)
    {
        graph->AddPass("Digits", [&](RenderGraphBuilder* builder) {
            TextureInfo swap = builder->GetTextureInfo(RENDER_GRAPH_PRIMARY_SWAPCHAIN_NAME);

            builder->SetQueue(QueueType::Universal);
            builder->SetType(RenderGraphPassType::Other);
    #if 0
            TextureInfo color;
            color.Type = TextureType::Texture2D;
            color.Width = 1600;
            color.Height = 900;
            color.Format = VK_FORMAT_R16G16B16A16_SFLOAT;
            builder->CreateTexture("Color", color);
    #endif
            builder->WriteTexture("Color", ResourceState::StorageTextureCompute());

            return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {
                m_ShaderInstance.BindComputePipeline(cmd);

                m_ShaderSystem.SetStorageBuffer<DigitsStorageBuffer>(0, 1, &m_Pixels);
                m_ShaderSystem.SetPushConstants<DigitsPushConstants>(m_Options);

                //m_ShaderSystem.SetStorageTexture(1, registry->GetTexture("Color"));
                m_ShaderSystem.BindCompute(cmd);

                cmd->Dispatch(1600 / workgroup_size, 900 / workgroup_size, 1);
            };
        });
    }
    graph->AddPass("Composite", [&](RenderGraphBuilder* builder) {
        builder->SetQueue(QueueType::Universal);
        builder->SetType(RenderGraphPassType::Render);

        builder->ReadTexture("Color", ResourceState::SampledFragment());
        builder->ReadWriteTexture(RENDER_GRAPH_PRIMARY_SWAPCHAIN_NAME, ResourceState::Color());

        TextureInfo color = builder->GetTextureInfo(RENDER_GRAPH_PRIMARY_SWAPCHAIN_NAME);
        RenderPassLayout rpl = builder->GetRenderPassInfo().Layout;

        return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {
            cmd->SetViewport(color.Width, color.Height);
            cmd->SetScissor(0, 0, color.Width, color.Height);

            SceneRenderer::SetExposure(1.0f);
            SceneRenderer::DrawComposite(registry->GetTexture("Color"), rpl, cmd);
        };
    });
}

void SandboxDigits::OnUpdate(Timestep ts)
{
    AR_PROFILE_FUNCTION();

}

void SandboxDigits::OnImGuiRender()
{
    AR_PROFILE_FUNCTION();

    ImGui::Begin("Options");
    
    ImGui::SliderFloat4("Color", (float*)&m_Options.Color, 0.0f, 1.0f);
    ImGui::SliderFloat("Size", &m_Options.Size, 0.0f, 100.0f);
    
    ImGui::End();

}

void SandboxDigits::OnEvent(Event& e)
{
    AR_PROFILE_SCOPE();

    if (Input::IsMouseButtonPressed(MouseCode::Button0))
    {
        
        auto [x, y] = Input::GetMousePosition();
        x *= 2.0f;
        y *= 2.0f;

        vec2 xy = {x, y};
        if (m_MouseDown)
        {
            m_Options.LastMousePosition = m_LastMousePosition;
            m_Options.CurrentMousePosition = xy;
            m_Options.Clear = false;
        }
        else
        {
            m_Options.LastMousePosition = xy;
            m_Options.CurrentMousePosition = xy;
            m_Options.Clear = false;
        }
        
        m_LastMousePosition = xy;
        
        m_MouseDown = true;
#if 0
        for (uint32 i = 0; i < 1600; i++)
        {
            for (uint32 j = 0; j < 900; j++)
            {
                float a = x - i;
                float b = y - j;
                if (a * a + b * b < 100.0f)
                {
                    uint32 base = i + m_Pixels.Size * j;

                    m_Pixels.Pixels[base + 0] = 1.0f;
                    m_Pixels.Pixels[base + 1] = 1.0f;
                    m_Pixels.Pixels[base + 2] = 1.0f;
                    m_Pixels.Pixels[base + 3] = 1.0f;
                }
            }
        }
#endif
    }
    else
    {
        m_MouseDown = false;
    }
    if (Input::IsKeyPressed(KeyCode::C))
    {
        m_Options.Clear = true;
        return;
        for (uint32 i = 0; i < 1600 * 900 * 4; i++)
        {
            m_Pixels.Pixels[i] = 0.0f;
        }
    }

}