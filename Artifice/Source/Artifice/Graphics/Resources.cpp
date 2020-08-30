#include "Resources.h"


// Buffers

bool BufferInfo::operator==(const BufferInfo& other) const
{
    if (BindFlags != other.BindFlags)
        return false;
    if (Access != other.Access)
        return false;
    if (Size != other.Size)
        return false;
    
    return true;
}
bool BufferInfo::operator!=(const BufferInfo& other) const
{
    return !(*this == other);
}
bool BufferInfo::operator<(const BufferInfo& other) const
{
    if (BindFlags != other.BindFlags)
        return BindFlags < other.BindFlags;
    if (Access != other.Access)
        return Access < other.Access;
    if (Size != other.Size)
        return Size < other.Size;
    
    return false;
}

// Textures

bool TextureInfo::operator==(const TextureInfo& other) const
{
    if (Type != other.Type)
        return false;
    if (BindFlags != other.BindFlags)
        return false;
    if (Width != other.Width)
        return false;
    if (Height != other.Height)
        return false;
    if (Depth != other.Depth)
        return false;
    if (Layers != other.Layers)
        return false;
    if (MipsEnabled != other.MipsEnabled)
        return false;
    if (Format != other.Format)
        return false;
    if (Samples != other.Samples)
        return false;

    return true;
}
bool TextureInfo::operator!=(const TextureInfo& other) const
{
    return !(*this == other);
}
bool TextureInfo::operator<(const TextureInfo& other) const
{
    if (Type != other.Type)
        return Type < other.Type;
    if (BindFlags != other.BindFlags)
        return BindFlags < other.BindFlags;
    if (Width != other.Width)
        return Width < other.Width;
    if (Height != other.Height)
        return Height < other.Height;
    if (Depth != other.Depth)
        return Depth < other.Depth;
    if (Layers != other.Layers)
        return Layers < other.Layers;
    if (MipsEnabled != other.MipsEnabled)
        return MipsEnabled < other.MipsEnabled;
    if (Format != other.Format)
        return Format < other.Format;
    if (Samples != other.Samples)
        return Samples < other.Samples;

    return false;
}

// Samplers

bool SamplerInfo::operator==(const SamplerInfo& other) const
{
    if (Filter != other.Filter)
        return false;
    if (AddressMode != other.AddressMode)
        return false;
    if (MipMode != other.MipMode)
        return false;
    if (Mips != other.Mips)
        return false;

    return true;
}
bool SamplerInfo::operator!=(const SamplerInfo& other) const
{
    return !(*this == other);
}
bool SamplerInfo::operator<(const SamplerInfo& other) const
{
    if (Filter != other.Filter)
        return Filter < other.Filter;
    if (AddressMode != other.AddressMode)
        return AddressMode < other.AddressMode;
    if (MipMode != other.MipMode)
        return MipMode < other.MipMode;
    if (Mips != other.Mips)
        return Mips < other.Mips;

    return false;
}

// Vertex layouts

VertexLayout::Binding::Binding(std::initializer_list<Format> attributes)
{
    for (const auto& attribute : attributes)
    {
        Attributes.push_back(attribute);
    }
}

bool VertexLayout::Binding::operator==(const Binding& other) const
{
    if (Attributes.size() != other.Attributes.size())
        return false;
    for (uint32 i = 0; i < Attributes.size(); i++)
    {
        if (Attributes[i] != other.Attributes[i])
            return false;
    }

    return true;
}
bool VertexLayout::Binding::operator!=(const Binding& other) const
{
    return !(*this == other);
}
bool VertexLayout::Binding::operator<(const Binding& other) const
{
    if (Attributes.size() != other.Attributes.size())
        return Attributes.size() < other.Attributes.size();
    for (uint32 i = 0; i < Attributes.size(); i++)
    {
        if (Attributes[i] != other.Attributes[i])
            return Attributes[i] < other.Attributes[i];
    }

    return false;
}

VertexLayout::VertexLayout(std::initializer_list<VertexLayout::Binding> bindings)
{
    for (const auto& binding : bindings)
    {
        Bindings.push_back(binding);
    }
}

bool VertexLayout::operator==(const VertexLayout& other) const
{
    if (Bindings.size() != other.Bindings.size())
        return false;
    for (uint32 i = 0; i < Bindings.size(); i++)
    {
        if (Bindings[i] != other.Bindings[i])
            return false;
    }

    return true;
}

bool VertexLayout::operator!=(const VertexLayout& other) const
{
    return !(*this == other);
}

bool VertexLayout::operator<(const VertexLayout& other) const
{
    if (Bindings.size() != other.Bindings.size())
        return Bindings.size() < other.Bindings.size();
    for (uint32 i = 0; i < Bindings.size(); i++)
    {
        if (Bindings[i] != other.Bindings[i])
            return Bindings[i] < other.Bindings[i];
    }

    return false;
}

uint32 VertexLayout::GetFormatLocationSize(Format format)
{
    switch (format)
    {
    case Format::R32Float:
    case Format::RG32Float:
    case Format::RGB32Float:
    case Format::RGBA32Float:
    case Format::RGBA8Unorm:
    case Format::RGBA32Int:
        return 1;
    default:
        AR_FATAL("");
    }
}

uint32 VertexLayout::GetFormatSize(Format format)
{
    switch (format)
    {
    case Format::R32Float:
        return 4;
    case Format::RG32Float:
        return 4 * 2;
    case Format::RGB32Float:
        return 4 * 3;
    case Format::RGBA32Float:
        return 4 * 4;
    case Format::RGBA32Int:
        return 4 * 4;
    case Format::RGBA8Unorm:
        return 1 * 4;
    case Format::BGRA8Unorm:
        return 1 * 4;
    default:
        AR_FATAL("");
    }
}

// Descriptor sets

bool DescriptorSetLayout::Binding::operator==(const DescriptorSetLayout::Binding& other) const
{
    if (Binding != other.Binding)
        return false;
    if (Type != other.Type)
        return false;
    if (Count != other.Count)
        return false;
    if (Stage != other.Stage)
        return false;

    return true;
}
bool DescriptorSetLayout::Binding::operator!=(const DescriptorSetLayout::Binding& other) const
{
    return !(*this == other);
}
bool DescriptorSetLayout::Binding::operator<(const DescriptorSetLayout::Binding& other) const
{
    if (Binding != other.Binding)
        return Binding < other.Binding;
    if (Type != other.Type)
        return Type < other.Type;
    if (Count != other.Count)
        return Count < other.Count;
    if (Stage != other.Stage)
        return Stage < other.Stage;

    return false;
}

DescriptorSetLayout::DescriptorSetLayout(std::initializer_list<DescriptorSetLayout::Binding> bindings)
{
    for (const auto& binding : bindings)
    {
        Bindings.push_back(binding);
    }
}

bool DescriptorSetLayout::operator==(const DescriptorSetLayout& other) const
{
    if (Bindings.size() != other.Bindings.size())
        return false;
    for (uint32 i = 0; i < Bindings.size(); i++)
    {
            if (Bindings[i] != other.Bindings[i])
                return false;
    }

    return true;
}
bool DescriptorSetLayout::operator!=(const DescriptorSetLayout& other) const
{
    return !(*this == other);
}
bool DescriptorSetLayout::operator<(const DescriptorSetLayout& other) const
{
    if (Bindings.size() != other.Bindings.size()) return Bindings.size() < other.Bindings.size();
    for (uint32 i = 0; i < Bindings.size(); i++)
    {
            if (Bindings[i] != other.Bindings[i]) return Bindings[i] < other.Bindings[i];
    }

    return false;
}

// Render passes

bool RenderPassLayout::Attachment::operator==(const Attachment& other) const
{
    if (Format != other.Format)
        return false;
    if (Samples != other.Samples)
        return false;

    return true;
}
bool RenderPassLayout::Attachment::operator!=(const Attachment& other) const
{
    return !(*this == other);
}
bool RenderPassLayout::Attachment::operator<(const Attachment& other) const
{
    if (Format != other.Format)
        return Format < other.Format;
    if (Samples != other.Samples)
        return Samples < other.Samples;

    return false;
}

bool RenderPassLayout::operator==(const RenderPassLayout& other) const
{
    if (ColorCount != other.ColorCount)
        return false;
    if (ResolveCount != other.ResolveCount)
        return false;
    if (DepthCount != other.DepthCount)
        return false;
    for (uint32 i = 0; i < ColorCount; i++)
    {
        if (Colors[i] != other.Colors[i])
            return false;
    }
    for (uint32 i = 0; i < DepthCount; i++)
    {
        if (Depth[i] != other.Depth[i])
            return false;
    }

    return true;
}
bool RenderPassLayout::operator!=(const RenderPassLayout& other) const
{
    return !(*this == other);
}
bool RenderPassLayout::operator<(const RenderPassLayout& other) const
{
    if (ColorCount != other.ColorCount)
        return ColorCount < other.ColorCount;
    if (ResolveCount != other.ResolveCount)
        return ResolveCount < other.ResolveCount;
    if (DepthCount != other.DepthCount)
        return DepthCount < other.DepthCount;
    for (uint32 i = 0; i < ColorCount; i++)
    {
        if (Colors[i] != other.Colors[i])
            return Colors[i] < other.Colors[i];
    }
    for (uint32 i = 0; i < DepthCount; i++)
    {
        if (Depth[i] != other.Depth[i])
            return Depth[i] < other.Depth[i];
    }

    return false;
}

bool RenderPassInfo::AttachmentOptions::operator==(const AttachmentOptions& other) const
{
    return LoadOp == other.LoadOp && StoreOp == other.StoreOp;
}
bool RenderPassInfo::AttachmentOptions::operator!=(const AttachmentOptions& other) const
{
    return !(*this == other);
}
bool RenderPassInfo::AttachmentOptions::operator<(const AttachmentOptions& other) const
{
    if (LoadOp != other.LoadOp)
        return LoadOp < other.LoadOp;
    if (StoreOp != other.StoreOp)
        return StoreOp < other.StoreOp;

    return false;
}

bool RenderPassInfo::operator==(const RenderPassInfo& other) const
{
    if (Layout != other.Layout)
        return false;
    for (uint32 i = 0; i < Layout.ColorCount; i++)
    {
        if (ColorOptions[i].LoadOp != other.ColorOptions[i].LoadOp)
            return false;
        if (ColorOptions[i].StoreOp != other.ColorOptions[i].StoreOp)
            return false;
    }
    for (uint32 i = 0; i < Layout.ResolveCount; i++)
    {
        if (ResolveOptions[i].LoadOp != other.ResolveOptions[i].LoadOp)
            return false;
        if (ResolveOptions[i].StoreOp != other.ResolveOptions[i].StoreOp)
            return false;
    }
    for (uint32 i = 0; i < Layout.DepthCount; i++)
    {
        if (DepthOptions[i].LoadOp != other.DepthOptions[i].LoadOp)
            return false;
        if (DepthOptions[i].StoreOp != other.DepthOptions[i].StoreOp)
            return false;
    }

    return true;
}
bool RenderPassInfo::operator!=(const RenderPassInfo& other) const
{
    return !(*this == other);
}
bool RenderPassInfo::operator<(const RenderPassInfo& other) const
{
    if (Layout != other.Layout)
        return Layout < other.Layout;

    for (uint32 i = 0; i < Layout.ColorCount; i++)
    {
        if (ColorOptions[i] != other.ColorOptions[i])
            return ColorOptions[i] < other.ColorOptions[i];
    }
    for (uint32 i = 0; i < Layout.ResolveCount; i++)
    {
        if (ResolveOptions[i] != other.ResolveOptions[i])
            return ResolveOptions[i] < other.ResolveOptions[i];
    }
    for (uint32 i = 0; i < Layout.DepthCount; i++)
    {
        if (DepthOptions[i] != other.DepthOptions[i])
            return DepthOptions[i] < other.DepthOptions[i];
    }

    return false;
}

bool FramebufferInfo::operator==(const FramebufferInfo& other) const
{
    if (Width != other.Width) return false;
    if (Height != other.Height) return false;
    if (RenderPassLayout != other.RenderPassLayout) return false;
    for (uint32 i = 0; i < RenderPassLayout.ColorCount; i++)
    {
        if (ColorTextures[i] != other.ColorTextures[i]) return false;
        if (Layers[i] != other.Layers[i]) return false;
    }
    for (uint32 i = 0; i < RenderPassLayout.ResolveCount; i++)
    {
        if (ResolveTextures[i] != other.ResolveTextures[i]) return false;
    }
    for (uint32 i = 0; i < RenderPassLayout.DepthCount; i++)
    {
        if (DepthTexture[i] != other.DepthTexture[i]) return false;
    }

    return true;
}
bool FramebufferInfo::operator!=(const FramebufferInfo& other) const
{
    return !(*this == other);
}
bool FramebufferInfo::operator<(const FramebufferInfo& other) const
{
    if (RenderPassLayout != other.RenderPassLayout)
        return RenderPassLayout < other.RenderPassLayout;

    for (uint32 i = 0; i < RenderPassLayout.ColorCount; i++)
    {
        if (ColorTextures[i] != other.ColorTextures[i])
            return ColorTextures[i] < other.ColorTextures[i];
        if (Layers[i] != other.Layers[i])
            return Layers[i] < other.Layers[i];
    }
    for (uint32 i = 0; i < RenderPassLayout.ResolveCount; i++)
    {
        if (ResolveTextures[i] != other.ResolveTextures[i])
            return ResolveTextures[i] < other.ResolveTextures[i];
    }
    for (uint32 i = 0; i < RenderPassLayout.DepthCount; i++)
    {
        if (DepthTexture[i] != other.DepthTexture[i])
            return DepthTexture[i] < other.DepthTexture[i];
    }

    return false;
}
