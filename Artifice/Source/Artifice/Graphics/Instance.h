#pragma once

#include <vector>

#include <vulkan/vulkan.h>

#include "Artifice/Core/Core.h"


class Instance
{
private:
    std::vector<const char*> m_Extensions;
#ifdef AR_DEBUG
    std::vector<const char*> m_ValidationLayers;
    VkDebugUtilsMessengerEXT m_DebugMessenger;
#endif
    VkInstance m_Handle;
public:
    Instance();
    void Init();
    void CleanUp();

    inline VkInstance GetHandle() const { return m_Handle; }
private:
    void EnumerateExtensions();
#ifdef AR_DEBUG
    void EnumerateValidationLayers();
    void CreateDebugLayerCallback();
    void DestroyDebugLayerCallback();
#endif // #ifdef AR_DEBUG
};
