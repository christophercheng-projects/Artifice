#if 0
#include "RenderGraph.h"
#include <algorithm>

// Topological sort
std::vector<std::string> GetReversePassOrder(std::unordered_map<std::string, RenderGraphPass> passes)
{
    std::unordered_map<std::string, bool> visited;
    std::unordered_map<std::string, std::vector<std::string>> dependencies;

    // Get dependencies
    for (auto& pass : passes)
    {
        visited.emplace(pass.first, false);
        dependencies.emplace(pass.first, pass.second.Dependencies);
    }

    std::vector<std::string> order;

    std::function<void(std::string)> visit = [&](std::string node)
    {
        visited[node] = true;
        for (auto& d : dependencies[node])
        {
            if (!visited.count(d))
            {
                visit(d);
            }
        }
        order.push_back(node);
    };

    for (auto& pass : passes)
    {
        if (!visited.count(pass.first))
        {
            visit(pass.first);
        }
    }


    return order;
}


void RenderGraph::Builder::Build(VkDevice device)
{
    // no compute for now

    std::vector<VkRenderPass> render_passes_raw;

    std::vector<std::string> reverse_order = GetReversePassOrder(Passes);
    std::vector<std::string> order(reverse_order.size());
    std::reverse_copy(reverse_order.begin(), reverse_order.end(), order);

    struct Transition
    {
        std::string Resource;
        std::string SourcePass;
        std::string DestinationPass;
        RenderBindFlags SourceBindFlag;
        RenderBindFlags DestinationBindFlag;
    };

    std::vector<Transition> transitions;

    std::unordered_map<std::string, std::pair<std::string, RenderBindFlags>> NextUses;
    for (auto& pass_name : reverse_order)
    {
        RenderGraphPass pass = Passes[pass_name];
        for (auto &resource : pass.AccessInfos)
        {
            std::string name = resource.Name;
            RenderBindFlags use = resource.LastBindFlag;
            // Note: we are guaranteed that use is NOT RenderBindFlags::None
            // Note: we use the last bind flag, this is what we are transitioning FROM
            //       all transitions in subpasses are done internally (performed below)

            // Get next resource use
            // Last transition per resource is to {"", RenderBindFlags::None}
            std::string next_pass_name = "";
            RenderBindFlags next_use = RenderBindFlags::None;
            if (NextUses.count(name))
            {
                next_pass_name = NextUses[name].first;
                next_use = NextUses[name].second;
            }
            // Insert transition
            transitions.push_back({name, pass_name, next_pass_name, use, next_use});
            // Update next use
            // Note: we update to first bind flag, as we want to the FIRST next use the next time we see this resource.
            NextUses[name] = {pass_name, resource.FirstBindFlag};
        }
    }

    std::unordered_map<std::string, std::set<uint32>> transitions_by_resource;
    std::unordered_map<std::string, std::set<uint32>> transitions_by_source_pass;
    std::unordered_map<std::string, std::set<uint32>> transitions_by_destination_pass;
    std::unordered_map<RenderBindFlags, std::set<uint32>> transitions_by_source_use;
    std::unordered_map<RenderBindFlags, std::set<uint32>> transitions_by_destination_use;

    for (uint32 i = 0; i < transitions.size(); i++)
    {
        Transition& t = transitions[i];
        transitions_by_resource[t.Resource].emplace(i);
        transitions_by_source_pass[t.SourcePass].emplace(i);
        transitions_by_destination_pass[t.DestinationPass].emplace(i);
        transitions_by_source_use[t.SourceBindFlag].emplace(i);
        transitions_by_destination_use[t.DestinationBindFlag].emplace(i);
    }

    // render pass can sync:
    //     something -> attachment
    //     attachment -> something
    // initial layout is last use
    // final layout is next use


    const auto IsAttachment = [](RenderBindFlags flag) -> bool
    {
        return flag == RenderBindFlags::ColorAttachment || flag == RenderBindFlags::DepthStencilAttachment;
    };
    const auto IsTexture = [](RenderBindFlags flag) -> bool
    {
        return flag == RenderBindFlags::ColorAttachment || flag == RenderBindFlags::DepthStencilAttachment || flag == RenderBindFlags::SampledTexture || flag == RenderBindFlags::StorageTexture;
    };
    const auto IsBuffer = [](RenderBindFlags flag) -> bool {
        return flag == RenderBindFlags::UniformBuffer || flag == RenderBindFlags::StorageBuffer || flag == RenderBindFlags::VertexBuffer || flag == RenderBindFlags::IndexBuffer;
    };


    for (auto& transition_indices : transitions_by_source_pass)
    {
        std::string pass_name = transition_indices.first;
        for (auto& index : transition_indices.second)
        {
            Transition transition = transitions[index];
            bool from_attachment = IsAttachment(transition.SourceBindFlag);
            bool to_attachment = IsAttachment(transition.DestinationBindFlag);

            // Normal barrier
            // non-attachment -> non-attachment
            if (!from_attachment && !to_attachment)
            {
                if (IsTexture(transition.SourceBindFlag))
                {
                    VirtualTextureBarriers[pass_name][{transition.SourceBindFlag, transition.DestinationBindFlag}].push_back(transition.Resource);
                }
                if (IsBuffer(transition.SourceBindFlag))
                {
                    VirtualBufferBarriers[pass_name][{transition.SourceBindFlag, transition.DestinationBindFlag}].push_back(transition.Resource);
                }
            }

            // handled by render passes
            // non-attachment -> attachment
            if (!from_attachment && to_attachment)
            {

            }
            
            // handled by render passes
            // attachment -> non-attachment
            if (from_attachment && !to_attachment)
            {

            }

            // handled by render passes
            // attachment -> attachment
            if (from_attachment && to_attachment)
            {

            }
        }
    }


    const auto GetIntersection = [](std::set<uint32> v1, std::set<uint32> v2) -> uint32
    {
        for (auto& i : v1)
        {
            if (v2.count(i))
            {
                return i;
            }
        }

        AR_CORE_FATAL("");
    };

    for (auto& entry : Passes)
    {
        // no compute for now

        std::string pass_name = entry.first;
        RenderGraphPass pass = entry.second;

        std::set<uint32> by_dst_pass = transitions_by_destination_pass[pass_name];
        std::set<uint32> by_src_pass = transitions_by_source_pass[pass_name];

        VkSubpassDependency begin_external = {};
        begin_external.srcSubpass = VK_SUBPASS_EXTERNAL;
        begin_external.dstSubpass = 0;
        begin_external.srcAccessMask = 0;
        begin_external.srcStageMask = 0;
        begin_external.dstAccessMask = 0;
        begin_external.dstStageMask = 0;
        VkSubpassDependency end_external = {};
        end_external.srcSubpass = pass.Subpasses.size() - 1;
        end_external.dstSubpass = VK_SUBPASS_EXTERNAL;
        end_external.srcAccessMask = 0;
        end_external.srcStageMask = 0;
        end_external.dstAccessMask = 0;
        end_external.dstStageMask = 0;

        std::unordered_map<std::string, VkAttachmentDescription> attachments;
        for (auto& attachment_name : pass.Attachments)
        {
            uint32 to_attachment_idx = GetIntersection(by_dst_pass, transitions_by_resource[attachment_name]);
            uint32 from_attachment_idx = GetIntersection(by_src_pass, transitions_by_resource[attachment_name]);
            Transition to_attachment = transitions[to_attachment_idx];
            bool is_from_attachment = IsAttachment(to_attachment.SourceBindFlag);
            Transition from_attachment = transitions[from_attachment_idx];
            
            VkAttachmentDescription desc = {};
            //desc.format = 
            // ...
            // ...
            // if from attachment, we expect it to already by transitioned
            if (is_from_attachment)
            {
                RenderBindFlags bind_flag = to_attachment.DestinationBindFlag;
                desc.initialLayout = RenderBindFlagToImageLayout(bind_flag);
            }
            else
            {
                // otherwise, transition here and add external subpass dependency
                RenderBindFlags src_flag = to_attachment.SourceBindFlag;
                desc.initialLayout = RenderBindFlagToImageLayout(src_flag);
                begin_external.srcAccessMask |= RenderBindFlagsToAccess(src_flag);
                begin_external.srcStageMask |= RenderBindFlagsToStage(src_flag);

                RenderBindFlags dst_flag = to_attachment.DestinationBindFlag;
                begin_external.dstAccessMask |= RenderBindFlagsToAccess(dst_flag);
                begin_external.dstStageMask |= RenderBindFlagsToStage(dst_flag);
            }

            // whether to attachment or not, we transition for next use and add external dependency
            RenderBindFlags src_flag = from_attachment.SourceBindFlag;
            end_external.srcAccessMask |= RenderBindFlagsToAccess(src_flag);
            end_external.srcStageMask |= RenderBindFlagsToStage(src_flag);

            RenderBindFlags dst_flag = from_attachment.DestinationBindFlag;
            desc.finalLayout = RenderBindFlagToImageLayout(dst_flag);
            end_external.dstAccessMask |= RenderBindFlagsToAccess(dst_flag);
            end_external.dstStageMask |= RenderBindFlagsToStage(dst_flag);


            attachments[attachment_name] = desc;
        }

        std::vector<VkAttachmentDescription> attachment_descriptions; // NEED THIS
        std::unordered_map<std::string, uint32> indices;
        uint32 attachment_index = 0;
        for (auto& desc : attachments)
        {
            attachment_descriptions.push_back(desc.second);
            indices.emplace(attachment_index++, desc.first);
        }

        // build subpasses
        uint32 subpass_count = pass.Subpasses.size();

        std::vector<VkSubpassDescription> subpass_descriptions; // NEED THIS

        std::vector<std::vector<VkAttachmentReference>> Colors(subpass_count);
        std::vector<std::vector<VkAttachmentReference>> DepthStencils(subpass_count);
        std::vector<std::vector<VkAttachmentReference>> Inputs(subpass_count);
        std::vector<std::vector<VkAttachmentReference>> Resolves(subpass_count);
        std::vector<std::vector<uint32>> Preserves(subpass_count);
        for (uint32 i = 0; i < subpass_count; i++)
        {
            RenderGraphPass::Subpass subpass = pass.Subpasses[i];
            for (auto& color : subpass.Colors)
            {
                Colors[i].push_back({indices[color], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
            }
            for (auto& depth : subpass.DepthStencils)
            {
                DepthStencils[i].push_back({indices[depth], VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL});
            }
            for (auto& input : subpass.Inputs)
            {
                Inputs[i].push_back({indices[input], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
            }
            for (auto& resolve : subpass.Resolves)
            {
                Resolves[i].push_back({indices[resolve], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
            }
            for (auto& preserve : subpass.Preserves)
            {
                Preserves[i].push_back(indices[preserve]);
            }
            
            VkSubpassDescription desc = {};
            desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            desc.inputAttachmentCount = Inputs[i].size();
            desc.pInputAttachments = Inputs[i].data();
            desc.colorAttachmentCount = Colors[i].size();
            desc.pColorAttachments = Colors[i].data();
            desc.pDepthStencilAttachment = DepthStencils[i].size() ? DepthStencils[i].data() : nullptr;
            desc.preserveAttachmentCount = Preserves[i].size();
            desc.pPreserveAttachments = Preserves[i].data();
            desc.pResolveAttachments = Resolves[i].size() ? Resolves[i].data() : nullptr;

            subpass_descriptions.push_back(desc);
        }

        std::vector<VkSubpassDependency> subpass_dependencies; // NEED THIS
        subpass_dependencies.push_back(begin_external);
        subpass_dependencies.push_back(end_external);

        // build subpass dependency chain
        // not optimal, but don't anticipate subpasses being that large, so probably not an issue
        for (int32 i = 0; i < subpass_count - 1; i++)
        {
            RenderGraphPass::Subpass src = pass.Subpasses[i];
            RenderGraphPass::Subpass dst = pass.Subpasses[i + 1];

            VkSubpassDependency dependency = {};
            dependency.srcSubpass = i;
            dependency.dstSubpass = i + 1;
            dependency.srcStageMask = 0;
            dependency.srcAccessMask = 0;
            dependency.dstAccessMask = 0;
            dependency.dstStageMask = 0;

            if (src.Colors.size())
            {
                dependency.srcStageMask |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependency.srcAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            }
            if (src.DepthStencils.size())
            {
                dependency.srcStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                dependency.srcAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            }
            if (src.Inputs.size())
            {
                dependency.srcStageMask |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            }

            if (dst.Colors.size())
            {
                dependency.dstStageMask |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependency.dstAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            }
            if (dst.DepthStencils.size())
            {
                dependency.dstStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                dependency.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            }
            if (dst.Inputs.size())
            {
                dependency.dstStageMask |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                dependency.dstAccessMask |= VK_ACCESS_SHADER_READ_BIT;
            }

            subpass_dependencies.push_back(dependency);
        }

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
        PassesRaw[pass_name] = render_pass_raw;
    }
    
}

void Build2()
{
    // build actual resources
    // build actual barriers
    // build passinfos into vector for consumption
}

#endif