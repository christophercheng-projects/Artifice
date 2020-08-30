#include "Instance.h"

#include <GLFW/glfw3.h>

#include "Artifice/Core/Log.h"


Instance::Instance()
{

}

void Instance::Init()
{
	VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Artifice";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Artifice";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);

    EnumerateExtensions();
#ifdef AR_DEBUG
    EnumerateValidationLayers();
#endif // #ifdef AR_DEBUG

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = (unsigned int)m_Extensions.size();
    createInfo.ppEnabledExtensionNames = m_Extensions.data();
    createInfo.enabledLayerCount = 0;
#ifdef AR_DEBUG
    createInfo.enabledLayerCount = (unsigned int)m_ValidationLayers.size();
    createInfo.ppEnabledLayerNames = m_ValidationLayers.data();
#endif // #ifdef AR_DEBUG

    VK_CALL(vkCreateInstance(&createInfo, nullptr, &m_Handle));

#ifdef AR_DEBUG
    CreateDebugLayerCallback();
#endif // #ifdef AR_DEBUG
}

void Instance::CleanUp()
{
#ifdef AR_DEBUG
    DestroyDebugLayerCallback();
#endif // #ifdef AR_DEBUG
    vkDestroyInstance(m_Handle, nullptr);
}

void Instance::EnumerateExtensions()
{
    unsigned int extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions;
    extensions.resize(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

    AR_CORE_INFO("Available instance extensions: ");
    for (unsigned int i = 0; i < extensions.size(); i++)
    {
        AR_CORE_INFO("\t%s", extensions[i].extensionName);
    }
    //check that all glfw required extensions are included
    unsigned int glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    for (unsigned int i = 0; i < glfwExtensionCount; i++)
    {
        bool found = false;
        for (unsigned int j = 0; j < extensions.size(); j++)
        {
            if (strcmp(extensions[j].extensionName, glfwExtensions[i]) == 0)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            AR_CORE_FATAL("Failed to find required instance extension %s required for GLFW.", glfwExtensions[i]);
        }
    }

#ifdef AR_DEBUG
    m_Extensions.reserve(glfwExtensionCount + 1);
    m_Extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#else // #ifdef AR_DEBUG
    m_Extensions.reserve(glfwExtensionCount);
#endif // #ifdef AR_DEBUG
    // add glfw required extensions
    for (unsigned int i = 0; i < glfwExtensionCount; i++)
    {
        m_Extensions.emplace_back(glfwExtensions[i]);
    }
    AR_CORE_INFO("Enabling instance extensions:");
    for (unsigned int i = 0; i < m_Extensions.size(); i++)
    {
        AR_CORE_INFO("\t%s", m_Extensions[i]);
    }
}

#ifdef AR_DEBUG

void Instance::EnumerateValidationLayers()
{
    //get available instance layers
    unsigned int layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> layers;
    layers.resize(layerCount); // needs to be resize NOT reserve because apprantly vkEnumerateInstanceLayerProperties requires initialized elements, not just space.
    vkEnumerateInstanceLayerProperties(&layerCount, layers.data());

    AR_CORE_INFO("Available layers:");
    for (unsigned int i = 0; i < layerCount; i++)
    {
        AR_CORE_INFO("\t%s", layers[i].layerName);
    }

    //add validation layers
    m_ValidationLayers.reserve(2);
    m_ValidationLayers.emplace_back("VK_LAYER_KHRONOS_validation");

    //check all added layers available
    for (unsigned int i = 0; i < m_ValidationLayers.size(); i++)
    {
        bool found = false;
        for (unsigned int j = 0; j < layers.size(); j++)
        {
            if (strcmp(layers[j].layerName, m_ValidationLayers[i]) == 0)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            AR_CORE_ERROR("Failed to find required layer %s", m_ValidationLayers[i]);
        }
    }

    AR_CORE_INFO("Enabling layers:");
    for (unsigned int i = 0; i < m_ValidationLayers.size(); i++)
    {
        AR_CORE_INFO("\t%s", m_ValidationLayers[i]);
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugUtilsCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData)
{
    AR_VK_VALIDATION("%s", callbackData->pMessage);

    return VK_FALSE;
}

void Instance::CreateDebugLayerCallback()
{
    PFN_vkCreateDebugUtilsMessengerEXT CreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Handle, "vkCreateDebugUtilsMessengerEXT");
    if (CreateDebugUtilsMessengerEXT)
    {
        VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = (PFN_vkDebugUtilsMessengerCallbackEXT)(void*)DebugUtilsCallback;

        VK_CALL((*CreateDebugUtilsMessengerEXT)(m_Handle, &createInfo, nullptr, &m_DebugMessenger));
    }
    else
    {
        AR_CORE_ERROR("Failed to create debug layer callback");
    }
}

void Instance::DestroyDebugLayerCallback()
{
    PFN_vkDestroyDebugUtilsMessengerEXT DestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Handle, "vkDestroyDebugUtilsMessengerEXT");
    if (DestroyDebugUtilsMessengerEXT)
    {
        (*DestroyDebugUtilsMessengerEXT)(m_Handle, m_DebugMessenger, nullptr);
    }
    else
    {
        AR_CORE_ERROR("Failed to destroy debug layer callback");
    }
}

#endif // #ifdef AR_DEBUG
