#pragma once

#include <vector>
#include <unordered_map>

#include "Artifice/Core/Core.h"
#include "Artifice/Core/Log.h"

#include "RenderGraphResourceCache.h"


class RenderGraphRegistry
{
private:
    RenderGraphTextureCache m_TextureCache;
    RenderGraphBufferCache m_BufferCache;
    
    std::unordered_map<std::string, uint32> m_ResourceMap;
    std::vector<TextureInfo> m_Textures;
    std::vector<BufferInfo> m_Buffers;

    std::vector<RenderHandle> m_TextureHandles;
    std::vector<RenderHandle> m_BufferHandles;

public:
    RenderGraphRegistry() = default;
    void Init(Device* device)
    {
        m_TextureCache.Init(device);
        m_BufferCache.Init(device);
    }
    void Advance()
    {
        m_TextureCache.ReleaseActive();
        m_BufferCache.ReleaseActive();
        
        m_TextureCache.Advance();
        m_BufferCache.Advance();
    }

    void Clear()
    {
        m_ResourceMap.clear();
        m_TextureHandles.clear();
        m_BufferHandles.clear();
        m_Textures.clear();
        m_Buffers.clear();
    }
    void CleanUp()
    {
        Clear();
        m_TextureCache.Reset();
        m_BufferCache.Reset();
    }

    bool Exists(const std::string& name) const
    {
        if (m_ResourceMap.find(name) != m_ResourceMap.end())
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    std::pair<uint32, uint32> GetAlive() const
    {
        return {m_TextureCache.GetAlive(), m_BufferCache.GetAlive()};
    }

    uint32 GetIndex(const std::string& name)
    {
        AR_CORE_ASSERT(m_ResourceMap.count(name), "Tried accessing resource that doesn't exists");

        return m_ResourceMap[name];
    }
    TextureInfo GetTextureInfo(const std::string& name)
    {
        AR_CORE_ASSERT(m_ResourceMap.count(name), "Tried accessing resource that doesn't exists");

        return m_Textures[GetIndex(name)];
    }

    uint32 GetTextureCount() const { return m_Textures.size(); }
    uint32 GetBufferCount() const { return m_Buffers.size(); }

    void ImportTexture(const std::string& name, RenderHandle texture, TextureInfo info)
    {
        AR_CORE_ASSERT(!m_ResourceMap.count(name), "Tried importing texture that already exists");

        uint32 index = m_Textures.size();
        m_ResourceMap[name] = index;

        m_Textures.push_back(info);
        m_TextureHandles.push_back(texture);
    }

    void CreateTexture(const std::string& name, TextureInfo info)
    {
        AR_CORE_ASSERT(!m_ResourceMap.count(name), "Tried creating texture that already exists");
        
        uint32 index = m_Textures.size();
        m_ResourceMap[name] = index;

        m_Textures.push_back(info);
        m_TextureHandles.push_back(RenderHandle());
    }
    void CreateBuffer(const std::string& name, BufferInfo info)
    {
        AR_CORE_ASSERT(!m_ResourceMap.count(name), "Tried creating buffer that already exists");

        uint32 index = m_Buffers.size();
        m_ResourceMap[name] = index;

        m_Buffers.push_back(info);
        m_BufferHandles.push_back(RenderHandle());
    }

    void AddTextureBindFlag(const std::string& name, RenderBindFlags bind_flag)
    {
        AR_CORE_ASSERT(m_ResourceMap.count(name), "Tried getting texture that does not exist");
        m_Textures[m_ResourceMap[name]].BindFlags |= bind_flag;

    }
    void AddBufferBindFlag(const std::string& name, RenderBindFlags bind_flag)
    {
        AR_CORE_ASSERT(m_ResourceMap.count(name), "Tried getting buffer that does not exist");
        m_Buffers[m_ResourceMap[name]].BindFlags |= bind_flag;

    }
    // Actual resource creation
    RenderHandle GetTexture(const std::string& name)
    {
        AR_CORE_ASSERT(m_ResourceMap.count(name), "Tried getting texture that does not exist");
        
        uint32 index = m_ResourceMap[name];

        if (!m_TextureHandles[index].IsNull())
        {
            return m_TextureHandles[index];
        }
        
        TextureInfo key = m_Textures[index];
        
        RenderHandle handle = m_TextureCache.Request(key);
        m_TextureHandles[index] = handle;

        return handle;
    }
    RenderHandle GetBuffer(const std::string& name)
    {
        AR_CORE_ASSERT(m_ResourceMap.count(name), "Tried getting buffer that does not exist");

        uint32 index = m_ResourceMap[name];

        if (!m_BufferHandles[index].IsNull())
        {
            return m_BufferHandles[index];
        }

        BufferInfo key = m_Buffers[index];

        RenderHandle handle = m_BufferCache.Request(key);
        m_BufferHandles[index] = handle;

        return handle;
    }
};