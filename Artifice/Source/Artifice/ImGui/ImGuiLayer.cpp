#include "ImGuiLayer.h"

#include <imgui.h>
#include <examples/imgui_impl_glfw.h>
#include <examples/imgui_impl_vulkan.h>

#include "Artifice/Core/Application.h"


#include "Artifice/math/vec2.h"


struct ImGuiViewportData
{
    RenderHandle Swapchain;
};

struct PushConstant
{
    vec2 Scale;
    vec2 Translate;
};

ImGuiLayer::ImGuiLayer()
{

}

static void ImGuiCreateWindow(ImGuiViewport* viewport)
{
    ImGuiViewportData* data = new ImGuiViewportData();
    viewport->RendererUserData = data;

    int32 width = (int32) viewport->Size.x;
    int32 height = (int32)viewport->Size.y;

    SwapchainInfo swap;
    swap.Window = viewport->PlatformHandle;
    swap.Width = width;
    swap.Height = height;

    Device* device = Application::Get()->GetRenderBackend()->GetDevice();
    data->Swapchain = device->CreateSwapchain(swap);
}

static void ImGuiDestroyWindow(ImGuiViewport* viewport)
{    
    Device* device = Application::Get()->GetRenderBackend()->GetDevice();
    device->WaitIdle();
    if (ImGuiViewportData* data = (ImGuiViewportData*)viewport->RendererUserData)
    {
        device->DestroySwapchain(data->Swapchain);
        delete data;
    }
    viewport->RendererUserData = NULL;
}

void ImGuiLayer::OnAttach()
{
    Device* device = Application::Get()->GetRenderBackend()->GetDevice();

    // Setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // Enable Multi-Viewport / Platform Windows
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    platform_io.Renderer_CreateWindow = ImGuiCreateWindow;
    platform_io.Renderer_DestroyWindow = ImGuiDestroyWindow;

    // Style
    ImGui::StyleColorsDark();
    
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui_ImplGlfw_InitForVulkan(Application::Get()->GetWindow()->GetHandle(), true);

    io.BackendRendererName = "Artifice";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset; // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
    io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports; // We can create multi-viewports on the Renderer side (optional)

    io.Fonts->AddFontFromFileTTF("Fonts/segoeui.ttf", 18.0f);
    io.Fonts->AddFontFromFileTTF("Fonts/SFProDisplay-Regular.ttf", 18.0f);
    io.FontDefault = io.Fonts->Fonts.back();
    
    uint8* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    TextureInfo texture;
    texture.Type = TextureType::Texture2D;
    texture.Width = width;
    texture.Height = height;
    texture.Format = VK_FORMAT_R8G8B8A8_UNORM;
    texture.BindFlags = RenderBindFlags::SampledTexture | RenderBindFlags::TransferDestination | RenderBindFlags::TransferSource;
    
    m_FontTexture = device->CreateTexture(texture);

    SamplerInfo sampler;
    sampler.Filter = VK_FILTER_LINEAR;
    sampler.AddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler.MipMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler.Mips = 1;

    m_Sampler = device->CreateSampler(sampler);

    CommandBuffer cmd = device->RequestCommandBuffer(QueueType::Universal);
    cmd.Begin();
    cmd.UploadTextureData(m_FontTexture, width * height * 4, pixels, false, {RenderBindFlags::SampledTexture, PipelineStageFlags::Fragment});
    cmd.End();
    device->CommitCommandBuffer(cmd);
    device->SubmitQueue(QueueType::Universal, {}, {}, true);

    //io.Fonts->TexID = (ImTextureID)device->GetTexture(m_FontTexture)->ImageView;
    m_ImGuiFontImage = ImGuiImage(m_FontTexture);
    io.Fonts->TexID = &m_ImGuiFontImage;

    Shader::Layout shader_layout;
    shader_layout.PushConstantRangeCount = 1;
    shader_layout.PushConstantRanges[0].Stage = ShaderStage::Vertex;
    shader_layout.PushConstantRanges[0].Offset = 0;
    shader_layout.PushConstantRanges[0].Size = sizeof(PushConstant); // Defined in ImGuiLayer.h.  Matches shader definition
    shader_layout.DescriptorSetLayoutCount = 1;
    shader_layout.DescriptorSetLayouts[0] = {{0, DescriptorType::CombinedImageSampler, 1, ShaderStage::Fragment}};

    m_Shader = new Shader(device, "Shaders/ImGui.vert.spv", "Shaders/ImGui.frag.spv", shader_layout);
    ShaderPipelineState state;
    state.VertexLayout = {
        {
            Format::RG32Float, // Position
            Format::RG32Float, // UV
            Format::RGBA8Unorm // Color
        }};
    state.CullMode = VK_CULL_MODE_NONE;
    state.FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    state.BlendEnabled = true;
    m_ShaderInstance = ShaderInstance(m_Shader, state);

    ShaderSystemLayout layout;
    layout.RegisterCombinedTextureSampler(0);

    m_TextureSystem = ShaderSystem(layout, m_Shader, 0);
    // TODO: push constants sytem?

}

void ImGuiLayer::OnDetach()
{
    Device* device = Application::Get()->GetRenderBackend()->GetDevice();
    m_Shader->CleanUp();
    delete m_Shader;
    m_ShaderInstance.CleanUp();
    device->DestroyTexture(m_FontTexture);
    device->DestroySampler(m_Sampler);

    ImGui_ImplGlfw_Shutdown();

    ImGui::DestroyContext();
}

bool show_demo_window = true;
bool show_another_window = false;
ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

void ImGuiLayer::Begin()
{
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiLayer::OnEvent(Event& e)
{
    if (m_BlockEvents)
    {
        ImGuiIO& io = ImGui::GetIO();
        e.Handled |= e.IsInCategory(EventCategoryMouse) & io.WantCaptureMouse;
        e.Handled |= e.IsInCategory(EventCategoryKeyboard) & io.WantCaptureKeyboard;
    }
}

void ImGuiLayer::AugmentRenderGraph(RenderGraph* graph)
{
    graph->AddPass("Primary Swapchain ImGui", [&](RenderGraphBuilder* builder) {
        builder->SetType(RenderGraphPassType::Render);
        builder->SetQueue(QueueType::Universal);

        builder->ReadWriteTexture(RENDER_GRAPH_PRIMARY_SWAPCHAIN_NAME, ResourceState::Color());

        // Register reads from calls to ImGui::Image()
        ImDrawData* draw_data = ImGui::GetDrawData();
        for (int n = 0; n < draw_data->CmdListsCount; n++)
        {
            const ImDrawList* cmd_list = draw_data->CmdLists[n];
            for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
            {
                const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
                ImGuiImage* image = (ImGuiImage*)pcmd->TextureId;
                if (image->Type == ImGuiImage::Type::RenderGraph)
                {
                    builder->ReadTexture(image->RenderGraphName, ResourceState::SampledFragment());
                }
            }
        }
        RenderPassLayout rpl = builder->GetRenderPassInfo().Layout;

        return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {
            End(rpl, registry, cmd);
        };
    });

    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    for (int i = 1; i < platform_io.Viewports.Size; i++)
    {
        ImGuiViewport* viewport = platform_io.Viewports[i];

        graph->AddPass("ImGui Viewport " + std::to_string(i), [&](RenderGraphBuilder* builder) {

            ImGuiViewportData* data = (ImGuiViewportData*)viewport->RendererUserData;

            Device* device = Application::Get()->GetRenderBackend()->GetDevice();
            std::string swap_name = "ImGui Swapchain " + std::to_string(i);
            builder->ImportTexture(swap_name, device->GetSwapchainTexture(data->Swapchain), ResourceState::None());
            builder->WriteTexture(swap_name, ResourceState::Color(), viewport->Flags & ImGuiViewportFlags_NoRendererClear ? false : true);

            // Register reads from calls to ImGui::Image()
            for (int n = 0; n < viewport->DrawData->CmdListsCount; n++)
            {
                const ImDrawList* cmd_list = viewport->DrawData->CmdLists[n];
                for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
                {
                    const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
                    ImGuiImage* image = (ImGuiImage*)pcmd->TextureId;
                    if (image->Type == ImGuiImage::Type::RenderGraph)
                    {
                        builder->ReadTexture(image->RenderGraphName, ResourceState::SampledFragment());
                    }
                }
            }

            RenderPassLayout rpl = builder->GetRenderPassInfo().Layout;

            return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {
                RenderImGuiDrawData(viewport->DrawData, rpl, registry, cmd);
            };    
        });

    }

    for (int i = 1; i < platform_io.Viewports.Size; i++)
    {
        graph->AddPass("ImGui Viewport Transition for Present " + std::to_string(i), [&](RenderGraphBuilder* builder) {
            builder->SetType(RenderGraphPassType::Other);
            builder->SetQueue(PRESENT_QUEUE);

            std::string swap_name = "ImGui Swapchain " + std::to_string(i);
            builder->ReadTexture(swap_name, ResourceState::Present());

            return [=](RenderGraphRegistry* registry, CommandBuffer* cmd) {
            };
        });
    }
}

std::vector<RenderHandle> ImGuiLayer::GetActiveSwapchains()
{
    std::vector<RenderHandle> swapchains;
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    for (int i = 1; i < platform_io.Viewports.Size; i++)
    {
        ImGuiViewport* viewport = platform_io.Viewports[i];

        ImGuiViewportData* data = (ImGuiViewportData*)viewport->RendererUserData;
        swapchains.push_back(data->Swapchain);
    }

    return swapchains;
}

void ImGuiLayer::RenderImGuiDrawData(ImDrawData* draw_data, RenderPassLayout rpl, RenderGraphRegistry* registry, CommandBuffer* cmd)
{
    Device* device = Application::Get()->GetRenderBackend()->GetDevice();

    uint64 vertex_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
    uint64 index_size = draw_data->TotalIdxCount * sizeof(ImDrawIdx);

    if (vertex_size == 0 || index_size == 0) return;

    ScratchBuffer vb = device->RequestVertexScratchBuffer(vertex_size);
    ScratchBuffer ib = device->RequestIndexScratchBuffer(index_size);

    ImDrawVert* vb_ptr = (ImDrawVert*)vb.Pointer;
    ImDrawIdx* ib_ptr = (ImDrawIdx*)ib.Pointer;

    for (uint32 n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        memcpy(vb_ptr, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy(ib_ptr, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
        vb_ptr += cmd_list->VtxBuffer.Size;
        ib_ptr += cmd_list->IdxBuffer.Size;
    }

    uint32 width = draw_data->DisplaySize.x * draw_data->FramebufferScale.x;
    uint32 height = draw_data->DisplaySize.y * draw_data->FramebufferScale.y;
    
    m_ShaderInstance.BindGraphicsPipeline(cmd, rpl);
    cmd->SetViewport(width, height);

    PushConstant pc;
    pc.Scale.x = 2.0f / draw_data->DisplaySize.x;
    pc.Scale.y = 2.0f / draw_data->DisplaySize.y;
    pc.Translate.x = -1.0f - draw_data->DisplayPos.x * pc.Scale.x;
    pc.Translate.y = -1.0f - draw_data->DisplayPos.y * pc.Scale.y;

    cmd->PushConstants(ShaderStage::Vertex, sizeof(pc), 0, &pc);
    cmd->BindVertexBuffers({vb.Buffer}, {vb.Offset});
    cmd->BindIndexBuffer(ib.Buffer, ib.Offset, VK_INDEX_TYPE_UINT16);

    ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
    ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

    int global_vtx_offset = 0;
    int global_idx_offset = 0;
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];

            // Project scissor/clipping rectangles into framebuffer space
            ImVec4 clip_rect;
            clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
            clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
            clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
            clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;

            if (clip_rect.x < width && clip_rect.y < height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f)
            {
                // Negative offsets are illegal for vkCmdSetScissor
                if (clip_rect.x < 0.0f)
                    clip_rect.x = 0.0f;
                if (clip_rect.y < 0.0f)
                    clip_rect.y = 0.0f;

                ImGuiImage* image = (ImGuiImage*)pcmd->TextureId;
                RenderHandle texture;
                if (image->Type == ImGuiImage::Type::Real)
                {
                    texture = image->Texture;
                }
                else if (image->Type == ImGuiImage::Type::RenderGraph)
                {
                    texture = registry->GetTexture(image->RenderGraphName);
                }


                // Assume not dealing with too many images, just allocate transient descriptor set rather than bothering with materials
                m_TextureSystem.SetCombinedTextureSampler(0, texture, m_Sampler);

                m_TextureSystem.Bind(cmd);
                

                // Apply scissor/clipping rectangle
                cmd->SetScissor((int)clip_rect.x, (int)clip_rect.y, (unsigned int)(clip_rect.z - clip_rect.x), (unsigned int)(clip_rect.w - clip_rect.y));

                // Draw
                cmd->DrawIndexed(pcmd->ElemCount, 1, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset, 0);
            }
        }
        global_idx_offset += cmd_list->IdxBuffer.Size;
        global_vtx_offset += cmd_list->VtxBuffer.Size;
    }

}

void ImGuiLayer::End(RenderPassLayout rpl, RenderGraphRegistry* registry, CommandBuffer* cmd)
{

    ImDrawData* draw_data = ImGui::GetDrawData();
    RenderImGuiDrawData(draw_data, rpl, registry, cmd);
}