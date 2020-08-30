#pragma once

#include <vector>
#include <algorithm>
#include <unordered_map>
#include <map>

#include <vulkan/vulkan.h>

#include "Artifice/Core/Core.h"
#include "Artifice/Utils/Hash.h"
#include "Artifice/Core/Log.h"
#include "Artifice/Graphics/Resources.h"
#include "Artifice/Graphics/RenderHandle.h"




#if 0

struct RenderPassInfo
{
    struct AttachmentInfo
    {
        // TODO, own mapping
        VkFormat Format;
        VkSampleCountFlagBits Samples;
        VkAttachmentLoadOp Load;
        VkAttachmentStoreOp Store;

        ResourceState InitialState;
        ResourceState FinalState;
    };

    struct Subpass
    {
        std::vector<uint32> Colors;
        int32 DepthStencil;
        std::vector<uint32> Inputs;
        std::vector<uint32> Resolves;
        std::vector<uint32> Preserves;
    };

    std::vector<AttachmentInfo> Attachments;
    std::vector<Subpass> Subpasses;

};


void CreateRenderPass(VkDevice device, RenderPassInfo info)
{
    // Build raw attachments
    std::vector<VkAttachmentDescription> attachment_descriptions;
    for (auto& a : info.Attachments)
    {
        VkAttachmentDescription desc = {};
        desc.format = a.Format;
        desc.samples = a.Samples;
        desc.loadOp = a.Load;
        desc.storeOp = a.Store;
        // No stencil for now.
        desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        desc.initialLayout = RenderBindFlagToImageLayout(a.InitialState.Bind);
        desc.finalLayout = RenderBindFlagToImageLayout(a.FinalState.Bind);

        attachment_descriptions.push_back(desc);
    }

    // Build raw subpasses
    std::vector<VkSubpassDescription> subpass_descriptions;

    uint32 subpass_count = info.Subpasses.size();

    std::vector<VkAttachmentReference> color_refs[subpass_count];
    VkAttachmentReference depth_refs[subpass_count];
    std::vector<VkAttachmentReference> input_refs[subpass_count];
    std::vector<VkAttachmentReference> resolve_refs[subpass_count];
    std::vector<uint32> preserve_refs[subpass_count];

    for (uint32 i = 0; i < info.Subpasses.size(); i++)
    {
        RenderPassInfo::Subpass& s = info.Subpasses[i];
        for (auto at_idx : s.Colors)
        {
            color_refs[i].push_back({at_idx, RenderBindFlagToImageLayout(RenderBindFlags::ColorAttachment)});
        }
        if (s.DepthStencil != -1)
        {
            depth_refs[i] = {s.DepthStencil, RenderBindFlagToImageLayout(RenderBindFlags::DepthStencilAttachment)};
        }
        for (auto at_idx : s.Inputs)
        {
            input_refs[i].push_back({at_idx, RenderBindFlagToImageLayout(RenderBindFlags::InputAttachment)});
        }
        for (auto at_idx : s.Resolves)
        {
            resolve_refs[i].push_back({at_idx, RenderBindFlagToImageLayout(RenderBindFlags::ColorAttachment)});
        }
        for (auto at_idx : s.Preserves)
        {
            preserve_refs[i].push_back(at_idx);
        }

        VkSubpassDescription desc = {};
        desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        desc.inputAttachmentCount = input_refs[i].size();
        desc.pInputAttachments = input_refs[i].data();
        desc.colorAttachmentCount = color_refs[i].size();
        desc.pColorAttachments = color_refs[i].data();
        desc.pResolveAttachments = resolve_refs[i].size() ? resolve_refs[i].data() : nullptr;
        desc.pDepthStencilAttachment = s.DepthStencil != -1 ? &depth_refs[i] : nullptr;
        desc.preserveAttachmentCount = preserve_refs[i].size();
        desc.pPreserveAttachments = preserve_refs[i].data();

        subpass_descriptions.push_back(desc);
    }

    // Build subpass dependencies
    // map since pairs aren't automatically hashed.
    std::map<std::pair<uint32, uint32>, std::pair<ResourceState, ResourceState>> dependency_map;

    // We traverse subpasses in reverse order, storing the next use of each attachment as we go.
    std::unordered_map<uint32, std::pair<uint32, ResourceState>> NextUse;

    // Insert final 
    for (uint32 at_idx = 0; at_idx < info.Attachments.size(); at_idx++)
    {
        NextUse[at_idx] = {VK_SUBPASS_EXTERNAL, info.Attachments[at_idx].FinalState};
    }
    // Traverse subpasses in reverse order
    for (int32 i = subpass_count - 1; i >= 0; i++)
    {
        RenderPassInfo::Subpass& s = info.Subpasses[i];

        for (auto at_idx : s.Colors)
        {
            std::pair<uint32, ResourceState>& next_use = NextUse[at_idx];
            dependency_map[{i, next_use.first}] = {{RenderBindFlags::ColorAttachment, PipelineStageFlags::Color}, next_use.second};
            next_use = {i, {RenderBindFlags::ColorAttachment, PipelineStageFlags::Color}};
        }
        if (s.DepthStencil != -1)
        {
            std::pair<uint32, ResourceState> &next_use = NextUse[s.DepthStencil];
            dependency_map[{i, next_use.first}] = {{RenderBindFlags::DepthStencilAttachment, PipelineStageFlags::DepthStencil}, next_use.second};
            next_use = {i, {RenderBindFlags::DepthStencilAttachment, PipelineStageFlags::DepthStencil}};
        }
        for (auto at_idx : s.Inputs)
        {
            std::pair<uint32, ResourceState> &next_use = NextUse[at_idx];
            dependency_map[{i, next_use.first}] = {{RenderBindFlags::InputAttachment, PipelineStageFlags::Fragment}, next_use.second};
            next_use = {i, {RenderBindFlags::InputAttachment, PipelineStageFlags::Fragment}};
        }
        // TODO: verify I am doing the right thing here...
        for (auto at_idx : s.Resolves)
        {
            std::pair<uint32, ResourceState> &next_use = NextUse[at_idx];
            dependency_map[{i, next_use.first}] = {{RenderBindFlags::ColorAttachment, PipelineStageFlags::Color}, next_use.second};
            next_use = {i, {RenderBindFlags::ColorAttachment, PipelineStageFlags::Color}};
        }
    }
    // Insert initial
    for (uint32 at_idx = 0; at_idx < info.Attachments.size(); at_idx++)
    {
        std::pair<uint32, ResourceState>& next_use = NextUse[at_idx];
        dependency_map[{VK_SUBPASS_EXTERNAL, next_use.first}] = {info.Attachments[at_idx].InitialState, next_use.second};
    }

    // Now build raw dependencies from our dependency map
    std::vector<VkSubpassDependency> subpass_dependencies;
    for (auto& entry : dependency_map)
    {
        ResourceState src = entry.second.first;
        ResourceState dst = entry.second.second;

        VkSubpassDependency dep = {};
        dep.srcSubpass = entry.first.first;
        dep.dstSubpass = entry.first.second;
        dep.srcStageMask = PipelineStageFlagsToStages(src.Stage);
        dep.dstStageMask = PipelineStageFlagsToStages(dst.Stage);
        dep.srcAccessMask = RenderBindFlagsToAccess(src.Bind);
        dep.dstAccessMask = RenderBindFlagsToAccess(dst.Bind);
    }

    // Create render pass
    VkRenderPassCreateInfo ci = {};
    ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    ci.attachmentCount = attachment_descriptions.size();
    ci.pAttachments = attachment_descriptions.data();
    ci.subpassCount = subpass_descriptions.size();
    ci.pSubpasses = subpass_descriptions.data();
    ci.dependencyCount = subpass_dependencies.size();
    ci.pDependencies = subpass_dependencies.data();

    VkRenderPass render_pass_raw;

    VK_CALL(vkCreateRenderPass(device, &ci, nullptr, &render_pass_raw));
}

#endif