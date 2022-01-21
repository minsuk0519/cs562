#include "debug.hpp"

namespace debug
{
    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;
    VkDebugUtilsMessengerEXT debugUtilsMessenger;

    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation",
    };

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT /*messageSeverity*/,
        VkDebugUtilsMessageTypeFlagsEXT /*messageType*/,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* /*pUserData*/)
    {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }
}

void debug::init(VkInstance instance)
{
    if (!VALIDATION_LAYER_ENABLED)
    {
        return;
    }

    if (VALIDATION_LAYER_ENABLED && !queryValidationLayerSupport())
    {
        std::cout << "validation layers requested, but not available!" << std::endl;
        return;
    }

    vkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    vkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));

    VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo{};
    debugUtilsMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugUtilsMessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugUtilsMessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debugUtilsMessengerCreateInfo.pfnUserCallback = debugCallback;

    vkCreateDebugUtilsMessengerEXT(instance, &debugUtilsMessengerCreateInfo, VK_NULL_HANDLE, &debugUtilsMessenger);
}

bool debug::queryValidationLayerSupport()
{
    uint32_t layerCount;

    //get size of layerproperties
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers)
    {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound)
        {
            return false;
        }
    }

    return true;
}

void debug::InstanceCreateInfoLayerRegister(VkInstanceCreateInfo& instanceCreateInfo)
{
    if (VALIDATION_LAYER_ENABLED)
    {
        instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(debug::validationLayers.size());
        instanceCreateInfo.ppEnabledLayerNames = debug::validationLayers.data();

        return;
    }

    instanceCreateInfo.enabledLayerCount = 0;
}

void debug::DeviceCreateInfoLayerRegister(VkDeviceCreateInfo& deviceCreateInfo)
{
    if (VALIDATION_LAYER_ENABLED)
    {
        deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(debug::validationLayers.size());
        deviceCreateInfo.ppEnabledLayerNames = debug::validationLayers.data();

        return;
    }

    deviceCreateInfo.enabledLayerCount = 0;
}

void debug::closedebug(VkInstance& instance)
{
    vkDestroyDebugUtilsMessengerEXT(instance, debugUtilsMessenger, VK_NULL_HANDLE);
}
