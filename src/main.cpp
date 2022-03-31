#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_vulkan.h>
#include <imgui/imgui_impl_glfw.h>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/quaternion.hpp>

#include <vector>
#include <iostream>
#include <array>

#include "uniforms.hpp"
#include "debug.hpp"
#include "window.hpp"
#include "device.hpp"
#include "buffer.hpp"
#include "gui.hpp"
#include "helper.hpp"
#include "render.hpp"
#include "object.hpp"
#include "shapes.hpp"

VkInstance vulkanInstance = VK_NULL_HANDLE;
VkSurfaceKHR vulkanSurface = VK_NULL_HANDLE;
VkSwapchainKHR vulkanSwapchain = VK_NULL_HANDLE;

VkQueryPool vulkanTimestampQuery;
VkQueryPool vulkanPipelineStatisticsQuery;

const uint32_t timestampQueryCount = TIMESTAMP::TIMESTAMP_MAX;
const uint32_t pipelinestatisticsQueryCount = 9;

std::array<float, TIMESTAMP::TIMESTAMP_MAX / 2> pipeline_timestamps;

VkQueue vulkanGraphicsQueue = VK_NULL_HANDLE;
VkQueue vulkanComputeQueue = VK_NULL_HANDLE;
VkPipelineCache vulkanPipelineCache = VK_NULL_HANDLE;

std::array<VkSampler, SAMPLE_INDEX_MAX> vulkanSamplers;

//formats
VkFormat vulkanColorFormat;
VkFormat vulkanDepthFormat;

// system values
window* windowPtr = new window();
device* devicePtr = new device();
memory* memPtr = new memory();
gui* guiPtr = new gui();

//buffer
std::array<Buffer, UNIFORM_INDEX_MAX> uniformbuffers;
std::array<VertexBuffer, VERTEX_INDEX_MAX> vertexbuffers;

std::array<Image*, IMAGE_INDEX_MAX> imagebuffers;

//draw swapchain
std::vector<Image> swapchainImages;
VkColorSpaceKHR vulkanColorSpace;
uint32_t swapchainImageCount;
std::vector<VkFence> vulkanWaitFences;

VkDescriptorPool vulkanDescriptorPool = VK_NULL_HANDLE;

std::vector<VkCommandBuffer> vulkanCommandBuffers;
std::vector<VkFramebuffer> vulkanFramebuffers;

std::array<VkCommandBuffer, render::COMPUTECMDBUFFER_MAX> vulkanComputeCommandBuffers;
std::array<VkDescriptorSetLayout, render::DESCRIPTOR_MAX> vulkanDescriptorSetLayouts;
std::array<VkDescriptorSet, render::DESCRIPTOR_MAX> vulkanDescriptorSets;
std::array<VkPipeline, render::PIPELINE_MAX> vulkanPipelines;
std::array<VkPipelineLayout, render::PIPELINE_MAX> vulkanPipelineLayouts;
std::array<VkSemaphore, render::SEMAPHORE_MAX> vulkanSemaphores;
std::array<VkRenderPass, render::RENDERPASS_MAX> vulkanRenderpasses;

//shadowmap
constexpr uint32_t shadowmapSize = 2048;
VkFormat shadowmapFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

std::vector<object*> objects;
glm::vec3 camerapos = glm::vec3(0.0f, 2.0f, 5.0f);
glm::quat rotation = glm::quat(glm::vec3(glm::radians(0.0f), 0, 0));

//glm::vec3 camerapos = glm::vec3(-0.00123167, 5.61389, 2.09556);
//glm::quat rotation = glm::quat(glm::vec3(3.08159, 0.031594, 3.14159));

light sun;
std::vector<light> local_light;
shadow shadowsetting;

lightSetting lightsetting;
ObjectProperties objproperties;

bool lightspacetoggle = false;
bool nolocallight = false;
helper::changeData<bool> blurshadow = true;

HammersleyBlock hammersley;
helper::changeData<int> hammersley_N = 30;

aoConstant AOConstant;

//constant value
constexpr glm::vec3 RIGHT = glm::vec3(1.0f, 0.0f, 0.0f);
constexpr glm::vec3 FORWARD = glm::vec3(0.0f, 0.0f, -1.0f);
constexpr glm::vec3 UP = glm::vec3(0.0f, 1.0f, 0.0f);

constexpr float LIGHT_SPHERE_SIZE = 0.05f;
constexpr float PI = 3.14159265358979f;
constexpr float PI_HALF = PI / 2.0f;

std::vector<const char*> getRequiredExtensions()
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (VALIDATION_LAYER_ENABLED)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

void createInstance()
{
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Minsuk API";
    appInfo.pEngineName = "Minsuk Engine";
    appInfo.apiVersion = VK_MAKE_API_VERSION(0, 1, 2, 0);

    VkInstanceCreateInfo instanceCreateInfo{};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;

    std::vector<const char*> extensions = getRequiredExtensions();
    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = extensions.data();

    debug::InstanceCreateInfoLayerRegister(instanceCreateInfo);

    instanceCreateInfo.pNext = nullptr;

    if (vkCreateInstance(&instanceCreateInfo, VK_NULL_HANDLE, &vulkanInstance) != VK_SUCCESS)
    {
        std::cout << "failed to create instance" << std::endl;
    }
}

void createswapchain()
{
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(devicePtr->vulkanPhysicalDevice, vulkanSurface, &formatCount, NULL);
    if (formatCount <= 0)
    {
        std::cout << "there is no supported format in surface" << std::endl;
    }

    std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(devicePtr->vulkanPhysicalDevice, vulkanSurface, &formatCount, surfaceFormats.data());

    if ((formatCount == 1) && (surfaceFormats[0].format == VK_FORMAT_UNDEFINED))
    {
        vulkanColorFormat = VK_FORMAT_B8G8R8A8_UNORM;
        vulkanColorSpace = surfaceFormats[0].colorSpace;
    }
    else
    {
        bool found_B8G8R8A8_UNORM = false;
        for (auto&& surfaceFormat : surfaceFormats)
        {
            if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM)
            {
                vulkanColorFormat = surfaceFormat.format;
                vulkanColorSpace = surfaceFormat.colorSpace;
                found_B8G8R8A8_UNORM = true;
                break;
            }
        }

        if (!found_B8G8R8A8_UNORM)
        {
            vulkanColorFormat = surfaceFormats[0].format;
            vulkanColorSpace = surfaceFormats[0].colorSpace;
        }
    }

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(devicePtr->vulkanPhysicalDevice, vulkanSurface, &surfaceCapabilities);

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(devicePtr->vulkanPhysicalDevice, vulkanSurface, &presentModeCount, VK_NULL_HANDLE);
    if (presentModeCount <= 0)
    {
        std::cout << "the number of present mode is incorrect!" << std::endl;
    }

    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(devicePtr->vulkanPhysicalDevice, vulkanSurface, &presentModeCount, presentModes.data());

    VkExtent2D swapchainExtent = {};
    if (surfaceCapabilities.currentExtent.width == (uint32_t)-1)
    {
        swapchainExtent.width = windowPtr->windowWidth;
        swapchainExtent.height = windowPtr->windowHeight;
    }
    else
    {
        swapchainExtent = surfaceCapabilities.currentExtent;
        windowPtr->windowWidth = surfaceCapabilities.currentExtent.width;
        windowPtr->windowHeight = surfaceCapabilities.currentExtent.height;
    }

    VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

    for (size_t i = 0; i < presentModeCount; i++)
    {
        if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
        if (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
        {
            swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        }
    }

    uint32_t desiredNumberOfSwapchainImages = surfaceCapabilities.minImageCount + 1;
    if ((surfaceCapabilities.maxImageCount > 0) && (desiredNumberOfSwapchainImages > surfaceCapabilities.maxImageCount))
    {
        desiredNumberOfSwapchainImages = surfaceCapabilities.maxImageCount;
    }

    VkSurfaceTransformFlagsKHR preTransform;
    if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
    {
        preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
    else
    {
        preTransform = surfaceCapabilities.currentTransform;
    }

    VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    std::vector<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags = {
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
    };
    for (auto& compositeAlphaFlag : compositeAlphaFlags)
    {
        if (surfaceCapabilities.supportedCompositeAlpha & compositeAlphaFlag)
        {
            compositeAlpha = compositeAlphaFlag;
            break;
        };
    }

    VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.surface = vulkanSurface;
    swapchainCreateInfo.minImageCount = desiredNumberOfSwapchainImages;
    swapchainCreateInfo.imageFormat = vulkanColorFormat;
    swapchainCreateInfo.imageColorSpace = vulkanColorSpace;
    swapchainCreateInfo.imageExtent = { swapchainExtent.width, swapchainExtent.height };
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainCreateInfo.preTransform = (VkSurfaceTransformFlagBitsKHR)preTransform;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCreateInfo.queueFamilyIndexCount = 0;
    swapchainCreateInfo.presentMode = swapchainPresentMode;
    swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.compositeAlpha = compositeAlpha;

    if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
    {
        swapchainCreateInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }

    if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
    {
        swapchainCreateInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }

    if (vkCreateSwapchainKHR(devicePtr->vulkanDevice, &swapchainCreateInfo, VK_NULL_HANDLE, &vulkanSwapchain) != VK_SUCCESS)
    {
        std::cout << "failed to create swapchain!" << std::endl;
    }

    std::vector<VkImage> vkswapchainimages;

    vkGetSwapchainImagesKHR(devicePtr->vulkanDevice, vulkanSwapchain, &swapchainImageCount, nullptr);

    vkswapchainimages.resize(swapchainImageCount);
    swapchainImages.resize(swapchainImageCount);
    vkGetSwapchainImagesKHR(devicePtr->vulkanDevice, vulkanSwapchain, &swapchainImageCount, vkswapchainimages.data());

    for (uint32_t i = 0; i < swapchainImageCount; i++)
    {
        VkImageViewCreateInfo colorAttachmentView = {};
        colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        colorAttachmentView.pNext = nullptr;
        colorAttachmentView.format = vulkanColorFormat;
        colorAttachmentView.components = {
            VK_COMPONENT_SWIZZLE_R,
            VK_COMPONENT_SWIZZLE_G,
            VK_COMPONENT_SWIZZLE_B,
            VK_COMPONENT_SWIZZLE_A
        };
        colorAttachmentView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        colorAttachmentView.subresourceRange.baseMipLevel = 0;
        colorAttachmentView.subresourceRange.levelCount = 1;
        colorAttachmentView.subresourceRange.baseArrayLayer = 0;
        colorAttachmentView.subresourceRange.layerCount = 1;
        colorAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;
        colorAttachmentView.flags = 0;

        colorAttachmentView.image = vkswapchainimages[i];
        swapchainImages[i].image = vkswapchainimages[i];
        swapchainImages[i].format = vulkanColorFormat;

        if (vkCreateImageView(devicePtr->vulkanDevice, &colorAttachmentView, nullptr, &swapchainImages[i].imageView) != VK_SUCCESS)
        {
            std::cout << "failed to create iamgeview for swapchain!" << std::endl;
        }
    }

    memPtr->create_sampler(devicePtr, vulkanSamplers[SAMPLE_INDEX_NORMAL], 1);
}

void createDepth()
{
    std::vector<VkFormat> depthFormats = {
    VK_FORMAT_D32_SFLOAT_S8_UINT,
    VK_FORMAT_D32_SFLOAT,
    VK_FORMAT_D24_UNORM_S8_UINT,
    VK_FORMAT_D16_UNORM_S8_UINT,
    VK_FORMAT_D16_UNORM
    };

    for (auto& format : depthFormats)
    {
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(devicePtr->vulkanPhysicalDevice, format, &formatProperties);

        if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        {
            vulkanDepthFormat = format;
            break;
        }
    }

    memPtr->create_depth_image(devicePtr, vulkanGraphicsQueue, vulkanDepthFormat, windowPtr->windowWidth, windowPtr->windowHeight, imagebuffers[IMAGE_INDEX_DEPTH]);
}

void compileshader()
{
    std::cout << "compiling shader..." << std::endl;
    system("data\\compile.bat");
    std::cout << "compiling shader finish" << std::endl;
}

void createquerypool()
{
    VkQueryPoolCreateInfo querypoolInfo{};
    querypoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    querypoolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
    querypoolInfo.queryCount = timestampQueryCount;
    if (vkCreateQueryPool(devicePtr->vulkanDevice, &querypoolInfo, VK_NULL_HANDLE, &vulkanTimestampQuery) != VK_SUCCESS)
    {
        std::cout << "failed to create querypool : timestamp" << std::endl;
    }

    querypoolInfo.queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS;
    querypoolInfo.pipelineStatistics =
        VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT |
        VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT |
        VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT |
        VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT |
        VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT |
        VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT |
        VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT |
        VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT |
        VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT;
    querypoolInfo.queryCount = pipelinestatisticsQueryCount;

    if (vkCreateQueryPool(devicePtr->vulkanDevice, &querypoolInfo, VK_NULL_HANDLE, &vulkanPipelineStatisticsQuery) != VK_SUCCESS)
    {
        std::cout << "failed to create querypool : pipelinestatistics" << std::endl;
    }
}

void createSemaphoreFence()
{
    VkSemaphoreCreateInfo createinfo;
    createinfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    createinfo.pNext = nullptr;
    createinfo.flags = 0;

    for (auto& semaphore : vulkanSemaphores)
    {
        vkCreateSemaphore(devicePtr->vulkanDevice, &createinfo, VK_NULL_HANDLE, &semaphore);
    }

    VkFenceCreateInfo fenceCreateInfo{};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    vulkanWaitFences.resize(swapchainImageCount);
    for (auto& fence : vulkanWaitFences)
    {
        if (vkCreateFence(devicePtr->vulkanDevice, &fenceCreateInfo, VK_NULL_HANDLE, &fence) != VK_SUCCESS)
        {
            std::cout << "failed to create fence!" << std::endl;
        }
    }

    VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
    pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    if (vkCreatePipelineCache(devicePtr->vulkanDevice, &pipelineCacheCreateInfo, nullptr, &vulkanPipelineCache) != VK_SUCCESS)
    {
        std::cout << "failed to create pipelinecache!" << std::endl;
    }
}

void createRenderpass()
{
    renderpass::create_renderpass(devicePtr->vulkanDevice, vulkanRenderpasses[render::RENDERPASS_SWAPCHAIN], {
        {vulkanColorFormat, 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, renderpass::ATTACHMENT_NONE},
        {vulkanDepthFormat, 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, renderpass::ATTACHMENT_DEPTH | renderpass::ATTACHMENT_NO_CLEAR_INITIAL},
    });

    vulkanFramebuffers.resize(render::RENDERPASS_SWAPCHAIN + swapchainImageCount);
    for (uint32_t i = 0; i < swapchainImageCount; i++)
    {
        renderpass::create_framebuffer(devicePtr->vulkanDevice, vulkanRenderpasses[render::RENDERPASS_SWAPCHAIN], vulkanFramebuffers[render::RENDERPASS_SWAPCHAIN + i], windowPtr->windowWidth, windowPtr->windowHeight, {
        swapchainImages[i].imageView, imagebuffers[IMAGE_INDEX_DEPTH]->imageView
            });
    }

    //gbuffer pass
    renderpass::create_renderpass(devicePtr->vulkanDevice, vulkanRenderpasses[render::RENDERPASS_GBUFFER], {
        {VK_FORMAT_R16G16B16A16_SFLOAT, 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, renderpass::ATTACHMENT_NONE},
        {VK_FORMAT_R16G16B16A16_SFLOAT, 1, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, renderpass::ATTACHMENT_NONE},
        {VK_FORMAT_R16G16B16A16_SFLOAT, 2, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, renderpass::ATTACHMENT_NONE},
        {VK_FORMAT_R8G8B8A8_SNORM, 3, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, renderpass::ATTACHMENT_NONE},
        {vulkanDepthFormat, 4, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, renderpass::ATTACHMENT_DEPTH},
    });

    renderpass::create_framebuffer(devicePtr->vulkanDevice, vulkanRenderpasses[render::RENDERPASS_GBUFFER], vulkanFramebuffers[render::RENDERPASS_GBUFFER], windowPtr->windowWidth, windowPtr->windowHeight, {
        imagebuffers[IMAGE_INDEX_GBUFFER_POS]->imageView, imagebuffers[IMAGE_INDEX_GBUFFER_NORM]->imageView, imagebuffers[IMAGE_INDEX_GBUFFER_TEX]->imageView, 
        imagebuffers[IMAGE_INDEX_GBUFFER_ALBEDO]->imageView, imagebuffers[IMAGE_INDEX_DEPTH]->imageView
    });

    //shadowmap pass
    renderpass::create_renderpass(devicePtr->vulkanDevice, vulkanRenderpasses[render::RENDERPASS_SHADOWMAP], {
        {shadowmapFormat, 0, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL, renderpass::ATTACHMENT_NONE},
        {vulkanDepthFormat, 1, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, renderpass::ATTACHMENT_DEPTH},
    });

    renderpass::create_framebuffer(devicePtr->vulkanDevice, vulkanRenderpasses[render::RENDERPASS_SHADOWMAP], vulkanFramebuffers[render::RENDERPASS_SHADOWMAP], shadowmapSize, shadowmapSize, {
        imagebuffers[IMAGE_INDEX_SHADOWMAP]->imageView, imagebuffers[IMAGE_INDEX_SHADOWMAP_DEPTH]->imageView
    });

    //ambientocculusion pass
    renderpass::create_renderpass(devicePtr->vulkanDevice, vulkanRenderpasses[render::RENDERPASS_AO], {
        {imagebuffers[IMAGE_INDEX_AO]->format, 0, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, renderpass::ATTACHMENT_NONE},
    });

    renderpass::create_framebuffer(devicePtr->vulkanDevice, vulkanRenderpasses[render::RENDERPASS_AO], vulkanFramebuffers[render::RENDERPASS_AO], windowPtr->windowWidth, windowPtr->windowHeight, {
        imagebuffers[IMAGE_INDEX_AO]->imageView,
        });
}

void updatelightingdescriptorset(bool blur)
{
    if (blur)
    {
        descriptor::write_descriptorset(devicePtr->vulkanDevice, vulkanDescriptorSets[render::DESCRIPTOR_LIGHT], {
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, {}, {vulkanSamplers[SAMPLE_INDEX_NORMAL], imagebuffers[IMAGE_INDEX_GBUFFER_POS]->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, {}, {vulkanSamplers[SAMPLE_INDEX_NORMAL], imagebuffers[IMAGE_INDEX_GBUFFER_NORM]->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, {}, {vulkanSamplers[SAMPLE_INDEX_NORMAL], imagebuffers[IMAGE_INDEX_GBUFFER_TEX]->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, {}, {vulkanSamplers[SAMPLE_INDEX_NORMAL], imagebuffers[IMAGE_INDEX_GBUFFER_ALBEDO]->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4, {}, {vulkanSamplers[SAMPLE_INDEX_NORMAL], imagebuffers[IMAGE_INDEX_SHADOWMAP_BLUR]->imageView, VK_IMAGE_LAYOUT_GENERAL}},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5, {}, {vulkanSamplers[SAMPLE_INDEX_NORMAL], imagebuffers[IMAGE_INDEX_SKYDOME_IRRADIANCE]->imageView, VK_IMAGE_LAYOUT_GENERAL}},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 6, {}, {vulkanSamplers[SAMPLE_INDEX_SKYDOME], imagebuffers[IMAGE_INDEX_SKYDOME]->imageView, VK_IMAGE_LAYOUT_GENERAL}},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 7, {}, {vulkanSamplers[SAMPLE_INDEX_NORMAL], imagebuffers[IMAGE_INDEX_AO_BLUR]->imageView, VK_IMAGE_LAYOUT_GENERAL}},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 8, {uniformbuffers[UNIFORM_INDEX_LIGHT_SETTING].buf, 0, uniformbuffers[UNIFORM_INDEX_LIGHT_SETTING].range}, {}},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 9, {uniformbuffers[UNIFORM_INDEX_CAMERA].buf, 0, uniformbuffers[UNIFORM_INDEX_CAMERA].range}, {}},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10, {uniformbuffers[UNIFORM_INDEX_LIGHT].buf, 0, uniformbuffers[UNIFORM_INDEX_LIGHT].range}, {}},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 11, {uniformbuffers[UNIFORM_INDEX_SHADOW_SETTING].buf, 0, uniformbuffers[UNIFORM_INDEX_SHADOW_SETTING].range}, {}},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 12, {uniformbuffers[UNIFORM_INDEX_HAMMERSLEYBLOCK].buf, 0, uniformbuffers[UNIFORM_INDEX_HAMMERSLEYBLOCK].range}, {}},
        });
    }
    else
    {
        descriptor::write_descriptorset(devicePtr->vulkanDevice, vulkanDescriptorSets[render::DESCRIPTOR_LIGHT], {
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, {}, {vulkanSamplers[SAMPLE_INDEX_NORMAL], imagebuffers[IMAGE_INDEX_GBUFFER_POS]->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, {}, {vulkanSamplers[SAMPLE_INDEX_NORMAL], imagebuffers[IMAGE_INDEX_GBUFFER_NORM]->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, {}, {vulkanSamplers[SAMPLE_INDEX_NORMAL], imagebuffers[IMAGE_INDEX_GBUFFER_TEX]->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, {}, {vulkanSamplers[SAMPLE_INDEX_NORMAL], imagebuffers[IMAGE_INDEX_GBUFFER_ALBEDO]->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4, {}, {vulkanSamplers[SAMPLE_INDEX_NORMAL], imagebuffers[IMAGE_INDEX_SHADOWMAP]->imageView, VK_IMAGE_LAYOUT_GENERAL}},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5, {}, {vulkanSamplers[SAMPLE_INDEX_NORMAL], imagebuffers[IMAGE_INDEX_SKYDOME_IRRADIANCE]->imageView, VK_IMAGE_LAYOUT_GENERAL}},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 6, {}, {vulkanSamplers[SAMPLE_INDEX_SKYDOME], imagebuffers[IMAGE_INDEX_SKYDOME]->imageView, VK_IMAGE_LAYOUT_GENERAL}},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 7, {}, {vulkanSamplers[IMAGE_INDEX_AO_BLUR], imagebuffers[IMAGE_INDEX_AO_BLUR]->imageView, VK_IMAGE_LAYOUT_GENERAL}},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 8, {uniformbuffers[UNIFORM_INDEX_LIGHT_SETTING].buf, 0, uniformbuffers[UNIFORM_INDEX_LIGHT_SETTING].range}, {}},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 9, {uniformbuffers[UNIFORM_INDEX_CAMERA].buf, 0, uniformbuffers[UNIFORM_INDEX_CAMERA].range}, {}},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10, {uniformbuffers[UNIFORM_INDEX_LIGHT].buf, 0, uniformbuffers[UNIFORM_INDEX_LIGHT].range}, {}},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 11, {uniformbuffers[UNIFORM_INDEX_SHADOW_SETTING].buf, 0, uniformbuffers[UNIFORM_INDEX_SHADOW_SETTING].range}, {}},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 12, {uniformbuffers[UNIFORM_INDEX_HAMMERSLEYBLOCK].buf, 0, uniformbuffers[UNIFORM_INDEX_HAMMERSLEYBLOCK].range}, {}},
        });
    }
}

void createdescriptorset()
{
    descriptor::create_descriptorpool(devicePtr->vulkanDevice, vulkanDescriptorPool, {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 12 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 5 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 15 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 4 },
        });

    //gbuffer pass
    descriptor::create_descriptorset_layout(devicePtr->vulkanDevice, vulkanDescriptorSetLayouts[render::DESCRIPTOR_GBUFFER], vulkanDescriptorSets[render::DESCRIPTOR_GBUFFER], vulkanDescriptorPool, {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT, 1},
        });

    descriptor::write_descriptorset(devicePtr->vulkanDevice, vulkanDescriptorSets[render::DESCRIPTOR_GBUFFER], {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, {uniformbuffers[UNIFORM_INDEX_PROJECTION].buf, 0, uniformbuffers[UNIFORM_INDEX_PROJECTION].range}, {}},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, {uniformbuffers[UNIFORM_INDEX_OBJECT].buf, 0, uniformbuffers[UNIFORM_INDEX_OBJECT].range}, {}},
        });

    //lighting pass
    descriptor::create_descriptorset_layout(devicePtr->vulkanDevice, vulkanDescriptorSetLayouts[render::DESCRIPTOR_LIGHT], vulkanDescriptorSets[render::DESCRIPTOR_LIGHT], vulkanDescriptorPool, {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 4},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 5},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 6},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 7},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 8},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 9},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_FRAGMENT_BIT, 10},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 11},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 12},
    });

    updatelightingdescriptorset(true);

    //local light pass
    descriptor::create_descriptorset_layout(devicePtr->vulkanDevice, vulkanDescriptorSetLayouts[render::DESCRIPTOR_LOCAL_LIGHT], vulkanDescriptorSets[render::DESCRIPTOR_LOCAL_LIGHT], vulkanDescriptorPool, {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 4},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 5},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 6},
        });

    descriptor::write_descriptorset(devicePtr->vulkanDevice, vulkanDescriptorSets[render::DESCRIPTOR_LOCAL_LIGHT], {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, {uniformbuffers[UNIFORM_INDEX_PROJECTION].buf, 0, uniformbuffers[UNIFORM_INDEX_PROJECTION].range}, {}},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, {}, {vulkanSamplers[SAMPLE_INDEX_NORMAL], imagebuffers[IMAGE_INDEX_GBUFFER_POS]->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, {}, {vulkanSamplers[SAMPLE_INDEX_NORMAL], imagebuffers[IMAGE_INDEX_GBUFFER_NORM]->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, {}, {vulkanSamplers[SAMPLE_INDEX_NORMAL], imagebuffers[IMAGE_INDEX_GBUFFER_TEX]->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4, {}, {vulkanSamplers[SAMPLE_INDEX_NORMAL], imagebuffers[IMAGE_INDEX_GBUFFER_ALBEDO]->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 5, {uniformbuffers[UNIFORM_INDEX_CAMERA].buf, 0, uniformbuffers[UNIFORM_INDEX_CAMERA].range}, {}},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 6, {uniformbuffers[UNIFORM_INDEX_LIGHT].buf, 0, uniformbuffers[UNIFORM_INDEX_LIGHT].range}, {}},
        });

    //diffuse obj
    descriptor::create_descriptorset_layout(devicePtr->vulkanDevice, vulkanDescriptorSetLayouts[render::DESCRIPTOR_DIFFUSE], vulkanDescriptorSets[render::DESCRIPTOR_DIFFUSE], vulkanDescriptorPool, {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT, 1},
        });

    descriptor::write_descriptorset(devicePtr->vulkanDevice, vulkanDescriptorSets[render::DESCRIPTOR_DIFFUSE], {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, {uniformbuffers[UNIFORM_INDEX_PROJECTION].buf, 0, uniformbuffers[UNIFORM_INDEX_PROJECTION].range}, {}},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, {uniformbuffers[UNIFORM_INDEX_OBJECT].buf, 0, uniformbuffers[UNIFORM_INDEX_OBJECT].range}, {}},
        });

    //shadowmapping
    descriptor::create_descriptorset_layout(devicePtr->vulkanDevice, vulkanDescriptorSetLayouts[render::DESCRIPTOR_SHADOWMAP], vulkanDescriptorSets[render::DESCRIPTOR_SHADOWMAP], vulkanDescriptorPool, {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT, 1},
        });

    descriptor::write_descriptorset(devicePtr->vulkanDevice, vulkanDescriptorSets[render::DESCRIPTOR_SHADOWMAP], {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, {uniformbuffers[UNIFORM_INDEX_SHADOW_SETTING].buf, 0, uniformbuffers[UNIFORM_INDEX_SHADOW_SETTING].range}, {}},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, {uniformbuffers[UNIFORM_INDEX_OBJECT].buf, 0, uniformbuffers[UNIFORM_INDEX_OBJECT].range}, {}},
        });

    //blurshadow
    descriptor::create_descriptorset_layout(devicePtr->vulkanDevice, vulkanDescriptorSetLayouts[render::DESCRIPTOR_SHADOWMAP_BLUR], vulkanDescriptorSets[render::DESCRIPTOR_SHADOWMAP_BLUR], vulkanDescriptorPool, {
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 0},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 1},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 2},
        });

    descriptor::write_descriptorset(devicePtr->vulkanDevice, vulkanDescriptorSets[render::DESCRIPTOR_SHADOWMAP_BLUR], {
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0, {}, {vulkanSamplers[SAMPLE_INDEX_NORMAL], imagebuffers[IMAGE_INDEX_SHADOWMAP]->imageView, VK_IMAGE_LAYOUT_GENERAL}},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, {}, {vulkanSamplers[SAMPLE_INDEX_NORMAL], imagebuffers[IMAGE_INDEX_SHADOWMAP_BLUR]->imageView, VK_IMAGE_LAYOUT_GENERAL}},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, {uniformbuffers[UNIFORM_INDEX_GAUSSIANWEIGHT].buf, 0, uniformbuffers[UNIFORM_INDEX_GAUSSIANWEIGHT].range}, {}},
        });

    //skydome
    descriptor::create_descriptorset_layout(devicePtr->vulkanDevice, vulkanDescriptorSetLayouts[render::DESCRIPTOR_SKYDOME], vulkanDescriptorSets[render::DESCRIPTOR_SKYDOME], vulkanDescriptorPool, {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 2},
        });

    descriptor::write_descriptorset(devicePtr->vulkanDevice, vulkanDescriptorSets[render::DESCRIPTOR_SKYDOME], {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, {uniformbuffers[UNIFORM_INDEX_PROJECTION].buf, 0, uniformbuffers[UNIFORM_INDEX_PROJECTION].range}, {}},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, {}, {vulkanSamplers[SAMPLE_INDEX_NORMAL], imagebuffers[IMAGE_INDEX_SKYDOME]->imageView, VK_IMAGE_LAYOUT_GENERAL}},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, {uniformbuffers[UNIFORM_INDEX_LIGHT_SETTING].buf, 0, uniformbuffers[UNIFORM_INDEX_LIGHT_SETTING].range}, {}},
        });

    //AO
    descriptor::create_descriptorset_layout(devicePtr->vulkanDevice, vulkanDescriptorSetLayouts[render::DESCRIPTOR_AO], vulkanDescriptorSets[render::DESCRIPTOR_AO], vulkanDescriptorPool, {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 2},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 3},
        });

    descriptor::write_descriptorset(devicePtr->vulkanDevice, vulkanDescriptorSets[render::DESCRIPTOR_AO], {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, {}, {vulkanSamplers[SAMPLE_INDEX_NORMAL], imagebuffers[IMAGE_INDEX_GBUFFER_POS]->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, {}, {vulkanSamplers[SAMPLE_INDEX_NORMAL], imagebuffers[IMAGE_INDEX_GBUFFER_NORM]->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, {uniformbuffers[UNIFORM_INDEX_CAMERA].buf, 0, uniformbuffers[UNIFORM_INDEX_CAMERA].range}, {}},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3, {uniformbuffers[UNIFORM_INDEX_AO_CONSTANT].buf, 0, uniformbuffers[UNIFORM_INDEX_AO_CONSTANT].range}, {}},
    });

    //blur AO
    descriptor::create_descriptorset_layout(devicePtr->vulkanDevice, vulkanDescriptorSetLayouts[render::DESCRIPTOR_AO_BLUR], vulkanDescriptorSets[render::DESCRIPTOR_AO_BLUR], vulkanDescriptorPool, {
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 0},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 1},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT, 2},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT, 3},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 4},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 5},
    });

    descriptor::write_descriptorset(devicePtr->vulkanDevice, vulkanDescriptorSets[render::DESCRIPTOR_AO_BLUR], {
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0, {}, {vulkanSamplers[SAMPLE_INDEX_NORMAL], imagebuffers[IMAGE_INDEX_AO]->imageView, VK_IMAGE_LAYOUT_GENERAL}},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, {}, {vulkanSamplers[SAMPLE_INDEX_NORMAL], imagebuffers[IMAGE_INDEX_AO_BLUR]->imageView, VK_IMAGE_LAYOUT_GENERAL}},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, {}, {vulkanSamplers[SAMPLE_INDEX_NORMAL], imagebuffers[IMAGE_INDEX_GBUFFER_POS]->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, {}, {vulkanSamplers[SAMPLE_INDEX_NORMAL], imagebuffers[IMAGE_INDEX_GBUFFER_NORM]->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4, {uniformbuffers[UNIFORM_INDEX_CAMERA].buf, 0, uniformbuffers[UNIFORM_INDEX_CAMERA].range}, {}},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 5, {uniformbuffers[UNIFORM_INDEX_GAUSSIANWEIGHT].buf, 0, uniformbuffers[UNIFORM_INDEX_GAUSSIANWEIGHT].range}, {}},
    });
}

void createPipeline()
{
    //gbuffer pass
    {
        pipeline::create_pipelinelayout(devicePtr->vulkanDevice, vulkanDescriptorSetLayouts[render::DESCRIPTOR_GBUFFER], vulkanPipelineLayouts[render::PIPELINE_GBUFFER]);

        VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.layout = vulkanPipelineLayouts[render::PIPELINE_GBUFFER];
        pipelineCreateInfo.renderPass = vulkanRenderpasses[render::RENDERPASS_GBUFFER];

        std::vector<VkVertexInputAttributeDescription> vertexinputs = {
            { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
            { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof(glm::vec3) },
            { 2, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(glm::vec3) * 2 },
        };

        VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo = pipeline::getInputAssemblyCreateInfo();
        VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo = pipeline::getRasterizationCreateInfo(VK_CULL_MODE_BACK_BIT);
        VkPipelineColorBlendAttachmentState pipelinecolorblendattachment = pipeline::getColorBlendAttachment(VK_FALSE);
        std::vector<VkPipelineColorBlendAttachmentState> pipelinecolorblendattachments = { pipelinecolorblendattachment, pipelinecolorblendattachment, pipelinecolorblendattachment, pipelinecolorblendattachment };
        VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo = pipeline::getColorBlendCreateInfo(static_cast<uint32_t>(pipelinecolorblendattachments.size()), pipelinecolorblendattachments.data());
        VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo = pipeline::getMultisampleCreateInfo(VK_SAMPLE_COUNT_1_BIT);
        VkViewport viewport = pipeline::getViewport(windowPtr->windowWidth, windowPtr->windowHeight);
        VkRect2D scissor = pipeline::getScissor(windowPtr->windowWidth, windowPtr->windowHeight);
        VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo = pipeline::getViewportCreateInfo(&viewport, &scissor);
        VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo = pipeline::getDepthStencilCreateInfo(VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
        VkVertexInputBindingDescription pipelinevertexinputbinding = pipeline::getVertexinputbindingDescription(sizeof(glm::vec3) * 2 + sizeof(glm::vec2));
        VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo = pipeline::getVertexinputAttributeDescription(&pipelinevertexinputbinding, vertexinputs);

        pipelineCreateInfo.pInputAssemblyState = &pipelineInputAssemblyStateCreateInfo;
        pipelineCreateInfo.pRasterizationState = &pipelineRasterizationStateCreateInfo;
        pipelineCreateInfo.pColorBlendState = &pipelineColorBlendStateCreateInfo;
        pipelineCreateInfo.pMultisampleState = &pipelineMultisampleStateCreateInfo;
        pipelineCreateInfo.pViewportState = &pipelineViewportStateCreateInfo;
        pipelineCreateInfo.pDepthStencilState = &pipelineDepthStencilStateCreateInfo;
        pipelineCreateInfo.pVertexInputState = &pipelineVertexInputStateCreateInfo;
        pipeline::create_pipieline(devicePtr->vulkanDevice, pipelineCreateInfo, vulkanPipelines[render::PIPELINE_GBUFFER], vulkanPipelineCache, {
                {"data/shaders/gbuffer.vert.spv", VK_SHADER_STAGE_VERTEX_BIT},
                {"data/shaders/gbuffer.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT},
            });
    }

    //lighting pass
    {
        pipeline::create_pipelinelayout(devicePtr->vulkanDevice, vulkanDescriptorSetLayouts[render::DESCRIPTOR_LIGHT], vulkanPipelineLayouts[render::PIPELINE_LIHGT]);

        VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.layout = vulkanPipelineLayouts[render::PIPELINE_LIHGT];
        pipelineCreateInfo.renderPass = vulkanRenderpasses[render::RENDERPASS_SWAPCHAIN];

        std::vector<VkVertexInputAttributeDescription> vertexinputs = {
            { 0, 0, VK_FORMAT_R32G32_SFLOAT, 0 },
            { 1, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(glm::vec2) },
        };

        VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo = pipeline::getInputAssemblyCreateInfo();
        VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo = pipeline::getRasterizationCreateInfo(VK_CULL_MODE_BACK_BIT);
        VkPipelineColorBlendAttachmentState pipelinecolorblendattachment = pipeline::getColorBlendAttachment(VK_FALSE);
        std::vector<VkPipelineColorBlendAttachmentState> pipelinecolorblendattachments = { pipelinecolorblendattachment };
        VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo = pipeline::getColorBlendCreateInfo(static_cast<uint32_t>(pipelinecolorblendattachments.size()), pipelinecolorblendattachments.data());
        VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo = pipeline::getMultisampleCreateInfo(VK_SAMPLE_COUNT_1_BIT);
        VkViewport viewport = pipeline::getViewport(windowPtr->windowWidth, windowPtr->windowHeight);
        VkRect2D scissor = pipeline::getScissor(windowPtr->windowWidth, windowPtr->windowHeight);
        VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo = pipeline::getViewportCreateInfo(&viewport, &scissor);
        VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo = pipeline::getDepthStencilCreateInfo(VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);
        VkVertexInputBindingDescription pipelinevertexinputbinding = pipeline::getVertexinputbindingDescription(sizeof(glm::vec2) * 2);
        VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo = pipeline::getVertexinputAttributeDescription(&pipelinevertexinputbinding, vertexinputs);

        pipelineCreateInfo.pInputAssemblyState = &pipelineInputAssemblyStateCreateInfo;
        pipelineCreateInfo.pRasterizationState = &pipelineRasterizationStateCreateInfo;
        pipelineCreateInfo.pColorBlendState = &pipelineColorBlendStateCreateInfo;
        pipelineCreateInfo.pMultisampleState = &pipelineMultisampleStateCreateInfo;
        pipelineCreateInfo.pViewportState = &pipelineViewportStateCreateInfo;
        pipelineCreateInfo.pDepthStencilState = &pipelineDepthStencilStateCreateInfo;
        pipelineCreateInfo.pVertexInputState = &pipelineVertexInputStateCreateInfo;
        pipeline::create_pipieline(devicePtr->vulkanDevice, pipelineCreateInfo, vulkanPipelines[render::PIPELINE_LIHGT], vulkanPipelineCache, {
                {"data/shaders/lighting.vert.spv", VK_SHADER_STAGE_VERTEX_BIT},
                {"data/shaders/lighting.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT},
            });
    }

    //local lighting pass
    {
        pipeline::create_pipelinelayout(devicePtr->vulkanDevice, vulkanDescriptorSetLayouts[render::DESCRIPTOR_LOCAL_LIGHT], vulkanPipelineLayouts[render::PIPELINE_LOCAL_LIGHT]);

        VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.layout = vulkanPipelineLayouts[render::PIPELINE_LOCAL_LIGHT];
        pipelineCreateInfo.renderPass = vulkanRenderpasses[render::RENDERPASS_SWAPCHAIN];

        std::vector<VkVertexInputAttributeDescription> vertexinputs = {
            { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
        };

        VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo = pipeline::getInputAssemblyCreateInfo();
        VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo = pipeline::getRasterizationCreateInfo(VK_CULL_MODE_BACK_BIT);
        VkPipelineColorBlendAttachmentState pipelinecolorblendattachment = pipeline::getColorBlendAttachment(VK_TRUE);
        std::vector<VkPipelineColorBlendAttachmentState> pipelinecolorblendattachments = { pipelinecolorblendattachment };
        VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo = pipeline::getColorBlendCreateInfo(static_cast<uint32_t>(pipelinecolorblendattachments.size()), pipelinecolorblendattachments.data());
        VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo = pipeline::getMultisampleCreateInfo(VK_SAMPLE_COUNT_1_BIT);
        VkViewport viewport = pipeline::getViewport(windowPtr->windowWidth, windowPtr->windowHeight);
        VkRect2D scissor = pipeline::getScissor(windowPtr->windowWidth, windowPtr->windowHeight);
        VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo = pipeline::getViewportCreateInfo(&viewport, &scissor);
        VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo = pipeline::getDepthStencilCreateInfo(VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);
        VkVertexInputBindingDescription pipelinevertexinputbinding = pipeline::getVertexinputbindingDescription(sizeof(glm::vec3));
        VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo = pipeline::getVertexinputAttributeDescription(&pipelinevertexinputbinding, vertexinputs);

        pipelineCreateInfo.pInputAssemblyState = &pipelineInputAssemblyStateCreateInfo;
        pipelineCreateInfo.pRasterizationState = &pipelineRasterizationStateCreateInfo;
        pipelineCreateInfo.pColorBlendState = &pipelineColorBlendStateCreateInfo;
        pipelineCreateInfo.pMultisampleState = &pipelineMultisampleStateCreateInfo;
        pipelineCreateInfo.pViewportState = &pipelineViewportStateCreateInfo;
        pipelineCreateInfo.pDepthStencilState = &pipelineDepthStencilStateCreateInfo;
        pipelineCreateInfo.pVertexInputState = &pipelineVertexInputStateCreateInfo;
        pipeline::create_pipieline(devicePtr->vulkanDevice, pipelineCreateInfo, vulkanPipelines[render::PIPELINE_LOCAL_LIGHT], vulkanPipelineCache, {
                {"data/shaders/locallights.vert.spv", VK_SHADER_STAGE_VERTEX_BIT},
                {"data/shaders/locallights.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT},
            });
    }

    //diffuse pass
    {
        pipeline::create_pipelinelayout(devicePtr->vulkanDevice, vulkanDescriptorSetLayouts[render::DESCRIPTOR_DIFFUSE], vulkanPipelineLayouts[render::PIPELINE_DIFFUSE]);

        VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.layout = vulkanPipelineLayouts[render::PIPELINE_DIFFUSE];
        pipelineCreateInfo.renderPass = vulkanRenderpasses[render::RENDERPASS_SWAPCHAIN];

        std::vector<VkVertexInputAttributeDescription> vertexinputs = {
            { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
        };

        VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo = pipeline::getInputAssemblyCreateInfo();
        VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo = pipeline::getRasterizationCreateInfo(VK_CULL_MODE_BACK_BIT);
        VkPipelineColorBlendAttachmentState pipelinecolorblendattachment = pipeline::getColorBlendAttachment(VK_FALSE);
        std::vector<VkPipelineColorBlendAttachmentState> pipelinecolorblendattachments = { pipelinecolorblendattachment };
        VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo = pipeline::getColorBlendCreateInfo(static_cast<uint32_t>(pipelinecolorblendattachments.size()), pipelinecolorblendattachments.data());
        VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo = pipeline::getMultisampleCreateInfo(VK_SAMPLE_COUNT_1_BIT);
        VkViewport viewport = pipeline::getViewport(windowPtr->windowWidth, windowPtr->windowHeight);
        VkRect2D scissor = pipeline::getScissor(windowPtr->windowWidth, windowPtr->windowHeight);
        VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo = pipeline::getViewportCreateInfo(&viewport, &scissor);
        VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo = pipeline::getDepthStencilCreateInfo(VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
        VkVertexInputBindingDescription pipelinevertexinputbinding = pipeline::getVertexinputbindingDescription(sizeof(glm::vec3));
        VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo = pipeline::getVertexinputAttributeDescription(&pipelinevertexinputbinding, vertexinputs);

        pipelineCreateInfo.pInputAssemblyState = &pipelineInputAssemblyStateCreateInfo;
        pipelineCreateInfo.pRasterizationState = &pipelineRasterizationStateCreateInfo;
        pipelineCreateInfo.pColorBlendState = &pipelineColorBlendStateCreateInfo;
        pipelineCreateInfo.pMultisampleState = &pipelineMultisampleStateCreateInfo;
        pipelineCreateInfo.pViewportState = &pipelineViewportStateCreateInfo;
        pipelineCreateInfo.pDepthStencilState = &pipelineDepthStencilStateCreateInfo;
        pipelineCreateInfo.pVertexInputState = &pipelineVertexInputStateCreateInfo;
        pipeline::create_pipieline(devicePtr->vulkanDevice, pipelineCreateInfo, vulkanPipelines[render::PIPELINE_DIFFUSE], vulkanPipelineCache, {
                {"data/shaders/diffuse.vert.spv", VK_SHADER_STAGE_VERTEX_BIT},
                {"data/shaders/diffuse.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT},
            });
    }

    //shadowmap pass
    {
        pipeline::create_pipelinelayout(devicePtr->vulkanDevice, vulkanDescriptorSetLayouts[render::DESCRIPTOR_SHADOWMAP], vulkanPipelineLayouts[render::PIPELINE_SHADOWMAP]);

        VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.layout = vulkanPipelineLayouts[render::PIPELINE_SHADOWMAP];
        pipelineCreateInfo.renderPass = vulkanRenderpasses[render::RENDERPASS_SHADOWMAP];

        std::vector<VkVertexInputAttributeDescription> vertexinputs = {
            { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
        };

        VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo = pipeline::getInputAssemblyCreateInfo();
        VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo = pipeline::getRasterizationCreateInfo(VK_CULL_MODE_BACK_BIT);
        VkPipelineColorBlendAttachmentState pipelinecolorblendattachment = pipeline::getColorBlendAttachment(VK_FALSE);
        std::vector<VkPipelineColorBlendAttachmentState> pipelinecolorblendattachments = { pipelinecolorblendattachment };
        VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo = pipeline::getColorBlendCreateInfo(static_cast<uint32_t>(pipelinecolorblendattachments.size()), pipelinecolorblendattachments.data());
        VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo = pipeline::getMultisampleCreateInfo(VK_SAMPLE_COUNT_1_BIT);
        VkViewport viewport = pipeline::getViewport(shadowmapSize, shadowmapSize);
        VkRect2D scissor = pipeline::getScissor(shadowmapSize, shadowmapSize);
        VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo = pipeline::getViewportCreateInfo(&viewport, &scissor);
        VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo = pipeline::getDepthStencilCreateInfo(VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
        VkVertexInputBindingDescription pipelinevertexinputbinding = pipeline::getVertexinputbindingDescription(sizeof(glm::vec3) * 2 + sizeof(glm::vec2));
        VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo = pipeline::getVertexinputAttributeDescription(&pipelinevertexinputbinding, vertexinputs);

        pipelineCreateInfo.pInputAssemblyState = &pipelineInputAssemblyStateCreateInfo;
        pipelineCreateInfo.pRasterizationState = &pipelineRasterizationStateCreateInfo;
        pipelineCreateInfo.pColorBlendState = &pipelineColorBlendStateCreateInfo;
        pipelineCreateInfo.pMultisampleState = &pipelineMultisampleStateCreateInfo;
        pipelineCreateInfo.pViewportState = &pipelineViewportStateCreateInfo;
        pipelineCreateInfo.pDepthStencilState = &pipelineDepthStencilStateCreateInfo;
        pipelineCreateInfo.pVertexInputState = &pipelineVertexInputStateCreateInfo;
        pipeline::create_pipieline(devicePtr->vulkanDevice, pipelineCreateInfo, vulkanPipelines[render::PIPELINE_SHADOWMAP], vulkanPipelineCache, {
                {"data/shaders/shadowmapping.vert.spv", VK_SHADER_STAGE_VERTEX_BIT},
                {"data/shaders/shadowmapping.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT},
            });
    }

    //blur shadow pass
    {
        pipeline::create_pipelinelayout(devicePtr->vulkanDevice, vulkanDescriptorSetLayouts[render::DESCRIPTOR_SHADOWMAP_BLUR], vulkanPipelineLayouts[render::PIPELINE_SHADOWMAP_BLUR_VERTICAL]);

        VkComputePipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.layout = vulkanPipelineLayouts[render::PIPELINE_SHADOWMAP_BLUR_VERTICAL];

        pipeline::create_compute_pipeline(devicePtr->vulkanDevice, pipelineCreateInfo, vulkanPipelines[render::PIPELINE_SHADOWMAP_BLUR_VERTICAL], vulkanPipelineCache,
            {"data/shaders/blurshadow_vertical.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT});

        pipeline::create_pipelinelayout(devicePtr->vulkanDevice, vulkanDescriptorSetLayouts[render::DESCRIPTOR_SHADOWMAP_BLUR], vulkanPipelineLayouts[render::PIPELINE_SHADOWMAP_BLUR_HORIZONTAL]);

        pipelineCreateInfo.layout = vulkanPipelineLayouts[render::PIPELINE_SHADOWMAP_BLUR_HORIZONTAL];

        pipeline::create_compute_pipeline(devicePtr->vulkanDevice, pipelineCreateInfo, vulkanPipelines[render::PIPELINE_SHADOWMAP_BLUR_HORIZONTAL], vulkanPipelineCache,
            { "data/shaders/blurshadow_horizontal.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT });
    }

    //skydome pass
    {
        pipeline::create_pipelinelayout(devicePtr->vulkanDevice, vulkanDescriptorSetLayouts[render::DESCRIPTOR_SKYDOME], vulkanPipelineLayouts[render::PIPELINE_SKYDOME]);

        VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.layout = vulkanPipelineLayouts[render::PIPELINE_SKYDOME];
        pipelineCreateInfo.renderPass = vulkanRenderpasses[render::RENDERPASS_SWAPCHAIN];

        std::vector<VkVertexInputAttributeDescription> vertexinputs = {
            { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
        };

        VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo = pipeline::getInputAssemblyCreateInfo();
        VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo = pipeline::getRasterizationCreateInfo(VK_CULL_MODE_BACK_BIT);
        VkPipelineColorBlendAttachmentState pipelinecolorblendattachment = pipeline::getColorBlendAttachment(VK_FALSE);
        std::vector<VkPipelineColorBlendAttachmentState> pipelinecolorblendattachments = { pipelinecolorblendattachment };
        VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo = pipeline::getColorBlendCreateInfo(static_cast<uint32_t>(pipelinecolorblendattachments.size()), pipelinecolorblendattachments.data());
        VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo = pipeline::getMultisampleCreateInfo(VK_SAMPLE_COUNT_1_BIT);
        VkViewport viewport = pipeline::getViewport(windowPtr->windowWidth, windowPtr->windowHeight);
        VkRect2D scissor = pipeline::getScissor(windowPtr->windowWidth, windowPtr->windowHeight);
        VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo = pipeline::getViewportCreateInfo(&viewport, &scissor);
        VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo = pipeline::getDepthStencilCreateInfo(VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
        VkVertexInputBindingDescription pipelinevertexinputbinding = pipeline::getVertexinputbindingDescription(sizeof(glm::vec3));
        VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo = pipeline::getVertexinputAttributeDescription(&pipelinevertexinputbinding, vertexinputs);

        pipelineCreateInfo.pInputAssemblyState = &pipelineInputAssemblyStateCreateInfo;
        pipelineCreateInfo.pRasterizationState = &pipelineRasterizationStateCreateInfo;
        pipelineCreateInfo.pColorBlendState = &pipelineColorBlendStateCreateInfo;
        pipelineCreateInfo.pMultisampleState = &pipelineMultisampleStateCreateInfo;
        pipelineCreateInfo.pViewportState = &pipelineViewportStateCreateInfo;
        pipelineCreateInfo.pDepthStencilState = &pipelineDepthStencilStateCreateInfo;
        pipelineCreateInfo.pVertexInputState = &pipelineVertexInputStateCreateInfo;
        pipeline::create_pipieline(devicePtr->vulkanDevice, pipelineCreateInfo, vulkanPipelines[render::PIPELINE_SKYDOME], vulkanPipelineCache, {
                {"data/shaders/skydome.vert.spv", VK_SHADER_STAGE_VERTEX_BIT},
                {"data/shaders/skydome.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT},
            });
    }

    //AO pass
    {
        pipeline::create_pipelinelayout(devicePtr->vulkanDevice, vulkanDescriptorSetLayouts[render::DESCRIPTOR_AO], vulkanPipelineLayouts[render::PIPELINE_AO]);

        VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.layout = vulkanPipelineLayouts[render::PIPELINE_AO];
        pipelineCreateInfo.renderPass = vulkanRenderpasses[render::RENDERPASS_AO];

        std::vector<VkVertexInputAttributeDescription> vertexinputs = {
            { 0, 0, VK_FORMAT_R32G32_SFLOAT, 0 },
        };

        VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo = pipeline::getInputAssemblyCreateInfo();
        VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo = pipeline::getRasterizationCreateInfo(VK_CULL_MODE_BACK_BIT);
        VkPipelineColorBlendAttachmentState pipelinecolorblendattachment = pipeline::getColorBlendAttachment(VK_FALSE);
        std::vector<VkPipelineColorBlendAttachmentState> pipelinecolorblendattachments = { pipelinecolorblendattachment };
        VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo = pipeline::getColorBlendCreateInfo(static_cast<uint32_t>(pipelinecolorblendattachments.size()), pipelinecolorblendattachments.data());
        VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo = pipeline::getMultisampleCreateInfo(VK_SAMPLE_COUNT_1_BIT);
        VkViewport viewport = pipeline::getViewport(windowPtr->windowWidth, windowPtr->windowHeight);
        VkRect2D scissor = pipeline::getScissor(windowPtr->windowWidth, windowPtr->windowHeight);
        VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo = pipeline::getViewportCreateInfo(&viewport, &scissor);
        VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo = pipeline::getDepthStencilCreateInfo(VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
        VkVertexInputBindingDescription pipelinevertexinputbinding = pipeline::getVertexinputbindingDescription(sizeof(glm::vec2));
        VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo = pipeline::getVertexinputAttributeDescription(&pipelinevertexinputbinding, vertexinputs);

        pipelineCreateInfo.pInputAssemblyState = &pipelineInputAssemblyStateCreateInfo;
        pipelineCreateInfo.pRasterizationState = &pipelineRasterizationStateCreateInfo;
        pipelineCreateInfo.pColorBlendState = &pipelineColorBlendStateCreateInfo;
        pipelineCreateInfo.pMultisampleState = &pipelineMultisampleStateCreateInfo;
        pipelineCreateInfo.pViewportState = &pipelineViewportStateCreateInfo;
        pipelineCreateInfo.pDepthStencilState = &pipelineDepthStencilStateCreateInfo;
        pipelineCreateInfo.pVertexInputState = &pipelineVertexInputStateCreateInfo;
        pipeline::create_pipieline(devicePtr->vulkanDevice, pipelineCreateInfo, vulkanPipelines[render::PIPELINE_AO], vulkanPipelineCache, {
                {"data/shaders/ambientocculusion.vert.spv", VK_SHADER_STAGE_VERTEX_BIT},
                {"data/shaders/ambientocculusion.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT},
            });
    }

    //blur AO pass
    {
        pipeline::create_pipelinelayout(devicePtr->vulkanDevice, vulkanDescriptorSetLayouts[render::DESCRIPTOR_AO_BLUR], vulkanPipelineLayouts[render::PIPELINE_AO_BLUR_VERTICAL]);

        VkComputePipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.layout = vulkanPipelineLayouts[render::PIPELINE_AO_BLUR_VERTICAL];

        pipeline::create_compute_pipeline(devicePtr->vulkanDevice, pipelineCreateInfo, vulkanPipelines[render::PIPELINE_AO_BLUR_VERTICAL], vulkanPipelineCache,
            { "data/shaders/blurAO_vertical.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT });

        pipeline::create_pipelinelayout(devicePtr->vulkanDevice, vulkanDescriptorSetLayouts[render::DESCRIPTOR_AO_BLUR], vulkanPipelineLayouts[render::PIPELINE_AO_BLUR_HORIZONTAL]);

        pipelineCreateInfo.layout = vulkanPipelineLayouts[render::PIPELINE_AO_BLUR_HORIZONTAL];

        pipeline::create_compute_pipeline(devicePtr->vulkanDevice, pipelineCreateInfo, vulkanPipelines[render::PIPELINE_AO_BLUR_HORIZONTAL], vulkanPipelineCache,
            { "data/shaders/blurAO_horizontal.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT });
    }
}

void createcommandbuffer()
{
    vulkanCommandBuffers.resize(swapchainImageCount + render::COMMANDBUFFER_SWAPCHAIN);

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = devicePtr->commandbuffer_allocateinfo(static_cast<uint32_t>(vulkanCommandBuffers.size()), false);

    if (vkAllocateCommandBuffers(devicePtr->vulkanDevice, &commandBufferAllocateInfo, vulkanCommandBuffers.data()) != VK_SUCCESS)
    {
        std::cout << "failed to allocate commandbuffers!" << std::endl;
    }

    commandBufferAllocateInfo = devicePtr->commandbuffer_allocateinfo(static_cast<uint32_t>(render::COMPUTECMDBUFFER_MAX), true);

    if (vkAllocateCommandBuffers(devicePtr->vulkanDevice, &commandBufferAllocateInfo, vulkanComputeCommandBuffers.data()) != VK_SUCCESS)
    {
        std::cout << "failed to allocate commandbuffers!" << std::endl;
    }
}

void updatebuffer()
{
    float fov = static_cast<float>(windowPtr->windowWidth) / static_cast<float>(windowPtr->windowHeight);

    Projection proj;

    proj.projMat = glm::perspective(glm::radians(45.0f), fov, 0.1f, 1000.0f);
    //force it to right handed
    proj.projMat[1][1] *= -1.0f;

    proj.viewMat = glm::toMat4(glm::conjugate(rotation)) * glm::translate(glm::mat4(1.0f), -camerapos);

    glm::vec3 lightpos = glm::vec3(0.0f, 10.0f, 0.0f);

    void* data0;
    vkMapMemory(devicePtr->vulkanDevice, uniformbuffers[UNIFORM_INDEX_PROJECTION].memory, 0, uniformbuffers[UNIFORM_INDEX_PROJECTION].size, 0, &data0);
    memcpy(data0, &proj, uniformbuffers[UNIFORM_INDEX_PROJECTION].size);
    vkUnmapMemory(devicePtr->vulkanDevice, uniformbuffers[UNIFORM_INDEX_PROJECTION].memory);

    shadowsetting.proj.projMat = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, shadowsetting.far_plane);
    shadowsetting.proj.projMat[1][1] *= -1.0f;

    glm::vec3 look = normalize(sun.direction);// rotation * FORWARD;

    glm::vec3 U = (std::abs(dot(look, UP)) == 1) ? glm::cross(look, FORWARD) : glm::cross(look, UP);
    U = glm::cross(U, look);

    glm::vec3 V = normalize(look);
    glm::vec3 A = normalize(glm::cross(V, U));
    glm::vec3 B = glm::cross(A, V);

    glm::mat4 R = glm::mat4(glm::vec4(A, 0.0f), glm::vec4(B, 0.0f), glm::vec4(-V, 0.0f), glm::vec4(0, 0, 0, 1));
    R = glm::transpose(R);
    glm::mat4 T = glm::translate(R, -glm::vec3(sun.position));

    shadowsetting.proj.viewMat = T;

    vkMapMemory(devicePtr->vulkanDevice, uniformbuffers[UNIFORM_INDEX_SHADOW_SETTING].memory, 0, uniformbuffers[UNIFORM_INDEX_SHADOW_SETTING].size, 0, &data0);
    memcpy(data0, &shadowsetting, uniformbuffers[UNIFORM_INDEX_SHADOW_SETTING].size);
    vkUnmapMemory(devicePtr->vulkanDevice, uniformbuffers[UNIFORM_INDEX_SHADOW_SETTING].memory);

    static float a = 0.0f;
    //a += 0.001f;

    for (auto obj : objects)
    {
        obj->prop->albedoColor = objproperties.albedoColor;
        obj->prop->metallic = objproperties.metallic;
        obj->prop->roughness = objproperties.roughness;
        obj->prop->refractiveindex = objproperties.refractiveindex;
    }

    objects[0]->prop->modelMat = glm::translate(glm::mat4(1.0f), glm::vec3(3.75f, -0.56f, -10.0f)) * glm::rotate(glm::mat4(1.0f), a, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(20.0f));
    objects[6]->prop->modelMat = glm::translate(glm::mat4(1.0f), glm::vec3(-3.75f, 1.7f, -10.0f)) * glm::rotate(glm::mat4(1.0f), PI - a, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.03f));

    //sphere
    objects[1]->prop->modelMat = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 3.5f, -12.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));
    objects[2]->prop->albedoColor = glm::vec3(0.4f, 0.4f, 0.4f);
    objects[2]->prop->modelMat = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -5.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(15.0f, 0.2f, 15.0f));

    objects[3]->prop->albedoColor = glm::vec3(0.4f, 0.1f, 0.4f);
    objects[3]->prop->modelMat = glm::translate(glm::mat4(1.0f), glm::vec3(-7.5f, 4.0f, -5.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.2f, 8.0f, 15.0f));
    //backwall
    objects[4]->prop->albedoColor = glm::vec3(0.4f, 0.1f, 0.4f);
    objects[4]->prop->modelMat = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 4.0f, -12.5f)) * glm::scale(glm::mat4(1.0f), glm::vec3(15.0f, 8.0f, 0.2f));

    //screen
    objects[5]->prop->albedoColor = glm::vec3(0.2f, 0.8f, 0.5f);
    objects[5]->prop->modelMat = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.5f, -9.5f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.2f, 3.0f, 6.0f));

    objects[7]->prop->albedoColor = glm::vec3(1.0f, 0.5f, 0.2f);
    objects[7]->prop->modelMat = glm::translate(glm::mat4(1.0f), glm::vec3(-3.75f, 0.1f, -9.45f)) * glm::rotate(glm::mat4(1.0f), -PI_HALF, glm::vec3(1.0f, 0.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.36f, 0.295f, 0.3f));
    objects[8]->prop->albedoColor = glm::vec3(1.0f, 0.5f, 0.2f);
    objects[8]->prop->modelMat = glm::translate(glm::mat4(1.0f), glm::vec3(3.75f, 0.1f, -9.45f)) * glm::rotate(glm::mat4(1.0f), -PI_HALF, glm::vec3(1.0f, 0.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.36f, 0.295f, 0.3f));
    
    objects[9]->prop->modelMat = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 3.5f, -0.5f)) * glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 2.0f, 2.0f));

    objects[10]->prop->modelMat = glm::translate(glm::mat4(1.0f), glm::vec3(-3.75f, 0.25f, -5.5f)) * glm::rotate(glm::mat4(1.0f), PI, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.003f));
    objects[11]->prop->modelMat = glm::translate(glm::mat4(1.0f), glm::vec3(-3.25f, 0.25f, -6.0f)) * glm::rotate(glm::mat4(1.0f), PI, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.003f));
    objects[12]->prop->modelMat = glm::translate(glm::mat4(1.0f), glm::vec3(-4.25f, 0.25f, -6.0f)) * glm::rotate(glm::mat4(1.0f), PI, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.003f));

    objects[13]->prop->modelMat = glm::translate(glm::mat4(1.0f), glm::vec3(3.75f, 0.025f, -5.5f)) * glm::rotate(glm::mat4(1.0f), PI_HALF, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(2.0f));
    objects[14]->prop->modelMat = glm::translate(glm::mat4(1.0f), glm::vec3(3.25f, 0.025f, -6.0f)) * glm::rotate(glm::mat4(1.0f), PI_HALF, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(2.0f));
    objects[15]->prop->modelMat = glm::translate(glm::mat4(1.0f), glm::vec3(4.25f, 0.025f, -6.0f)) * glm::rotate(glm::mat4(1.0f), PI_HALF, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(2.0f));

    objects[16]->prop->modelMat = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 8.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(1.0f));

    for (int i = 0; i < 5; ++i)
    {
        for (int j = 0; j < 5; ++j)
        {
            objects[16 + i * 5 + j]->prop->modelMat = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f + 1.2f * (2.0f - i), 5.0f + 1.2f * (2.0f - j), 10.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));
            objects[16 + i * 5 + j]->prop->metallic = i / 5.0f;
            objects[16 + i * 5 + j]->prop->roughness = (j / 5.0f) * (j / 5.0f);
        }
    }

    VkDeviceSize offset = 0;
    for (auto obj : objects)
    {
        void* data;
        vkMapMemory(devicePtr->vulkanDevice, uniformbuffers[UNIFORM_INDEX_OBJECT].memory, offset, uniformbuffers[UNIFORM_INDEX_OBJECT].range, 0, &data);
        memcpy(data, obj->prop, uniformbuffers[UNIFORM_INDEX_OBJECT].range);
        vkUnmapMemory(devicePtr->vulkanDevice, uniformbuffers[UNIFORM_INDEX_OBJECT].memory);
        offset += 128;
    }

    for (auto lit : local_light)
    {
        ObjectProperties prop;

        prop.albedoColor = glm::normalize(lit.color);
        prop.modelMat = glm::translate(glm::mat4(1.0f), lit.position) * glm::scale(glm::mat4(1.0f), glm::vec3(LIGHT_SPHERE_SIZE));

        void* data;
        vkMapMemory(devicePtr->vulkanDevice, uniformbuffers[UNIFORM_INDEX_OBJECT].memory, offset, uniformbuffers[UNIFORM_INDEX_OBJECT].range, 0, &data);
        memcpy(data, &prop, uniformbuffers[UNIFORM_INDEX_OBJECT].range);
        vkUnmapMemory(devicePtr->vulkanDevice, uniformbuffers[UNIFORM_INDEX_OBJECT].memory);
        offset += 128;
    }

    void* data2;
    vkMapMemory(devicePtr->vulkanDevice, uniformbuffers[UNIFORM_INDEX_LIGHT_SETTING].memory, 0, uniformbuffers[UNIFORM_INDEX_LIGHT_SETTING].size, 0, &data2);
    memcpy(data2, &lightsetting, uniformbuffers[UNIFORM_INDEX_LIGHT_SETTING].size);
    vkUnmapMemory(devicePtr->vulkanDevice, uniformbuffers[UNIFORM_INDEX_LIGHT_SETTING].memory);

    camera cam;
    cam.position = camerapos;
    cam.width = windowPtr->windowWidth;
    cam.height = windowPtr->windowHeight;

    void* data3;
    vkMapMemory(devicePtr->vulkanDevice, uniformbuffers[UNIFORM_INDEX_CAMERA].memory, 0, uniformbuffers[UNIFORM_INDEX_CAMERA].size, 0, &data3);
    memcpy(data3, &cam, uniformbuffers[UNIFORM_INDEX_CAMERA].size);
    vkUnmapMemory(devicePtr->vulkanDevice, uniformbuffers[UNIFORM_INDEX_CAMERA].memory);

    void* data4;
    vkMapMemory(devicePtr->vulkanDevice, uniformbuffers[UNIFORM_INDEX_LIGHT].memory, 0, uniformbuffers[UNIFORM_INDEX_LIGHT].range, 0, &data4);
    memcpy(data4, &sun, uniformbuffers[UNIFORM_INDEX_LIGHT].size);
    vkUnmapMemory(devicePtr->vulkanDevice, uniformbuffers[UNIFORM_INDEX_LIGHT].memory);

    offset = 0;
    for (const auto& lit : local_light)
    {
        offset += 64;
        void* data5;
        vkMapMemory(devicePtr->vulkanDevice, uniformbuffers[UNIFORM_INDEX_LIGHT].memory, offset, uniformbuffers[UNIFORM_INDEX_LIGHT].range, 0, &data5);
        memcpy(data5, &lit, uniformbuffers[UNIFORM_INDEX_LIGHT].range);
        vkUnmapMemory(devicePtr->vulkanDevice, uniformbuffers[UNIFORM_INDEX_LIGHT].memory);
    }

    {
        if (hammersley_N.isChanged())
        {
            hammersley.build(hammersley_N.value);

            void* data;
            vkMapMemory(devicePtr->vulkanDevice, uniformbuffers[UNIFORM_INDEX_HAMMERSLEYBLOCK].memory, 0, uniformbuffers[UNIFORM_INDEX_HAMMERSLEYBLOCK].range, 0, &data);
            memcpy(data, &hammersley.N, sizeof(int));
            data = static_cast<int*>(data) + 4;
            for (int i = 0; i < hammersley.N; ++i)
            {
                memcpy(data, &hammersley.hammersley[i], sizeof(float) * 2);
                data = static_cast<int*>(data) + 4;
            }
            vkUnmapMemory(devicePtr->vulkanDevice, uniformbuffers[UNIFORM_INDEX_HAMMERSLEYBLOCK].memory);
        }
    }

    void* data5;
    vkMapMemory(devicePtr->vulkanDevice, uniformbuffers[UNIFORM_INDEX_AO_CONSTANT].memory, 0, uniformbuffers[UNIFORM_INDEX_AO_CONSTANT].range, 0, &data5);
    memcpy(data5, &AOConstant, uniformbuffers[UNIFORM_INDEX_AO_CONSTANT].size);
    vkUnmapMemory(devicePtr->vulkanDevice, uniformbuffers[UNIFORM_INDEX_AO_CONSTANT].memory);
}

void setupbuffer()
{
    sun.direction = glm::vec3(-3.0f, -2.0f, 0.0);
    sun.color = glm::vec3(8.0f, 8.0f, 10.0f);

    sun.position = glm::vec3(15.0, 10.0f, -5.0f);
    sun.radius = 10.1f;

    local_light.push_back(light{ glm::vec3(-4.0f, 6.0f, -10.0f), 10.0f, glm::vec3(0.0f, 0.0f, 0.0f), 0, glm::vec3(20.0f, 0.0f, 0.0f) });
    local_light.push_back(light{ glm::vec3(4.0f, 6.0f, -10.0f),  5.0f, glm::vec3(0.0f, 0.0f, 0.0f), 0, glm::vec3(0.0f, 0.0f, 15.0f) });

    local_light.push_back(light{ glm::vec3(-3.75f, 0.6f, -5.5f), 0.5f, glm::vec3(0.0f, 0.0f, 0.0f), 0, glm::vec3(0.0f, 10.0f, 0.0f) });
    local_light.push_back(light{ glm::vec3(-3.25f, 0.6f, -6.0f), 0.5f, glm::vec3(0.0f, 0.0f, 0.0f), 0, glm::vec3(0.0f, 10.0f, 0.0f) });
    local_light.push_back(light{ glm::vec3(-4.25f, 0.6f, -6.0f), 0.5f, glm::vec3(0.0f, 0.0f, 0.0f), 0, glm::vec3(0.0f, 10.0f, 0.0f) });
    local_light.push_back(light{ glm::vec3(3.75f,  0.6f, -5.5f), 0.5f, glm::vec3(0.0f, 0.0f, 0.0f), 0, glm::vec3(0.0f, 10.0f, 0.0f) });
    local_light.push_back(light{ glm::vec3(3.25f,  0.6f, -6.0f), 0.5f, glm::vec3(0.0f, 0.0f, 0.0f), 0, glm::vec3(0.0f, 10.0f, 0.0f) });
    local_light.push_back(light{ glm::vec3(4.25f,  0.6f, -6.0f), 0.5f, glm::vec3(0.0f, 0.0f, 0.0f), 0, glm::vec3(0.0f, 10.0f, 0.0f) });

    memPtr->create_fb_image(devicePtr->vulkanDevice, VK_FORMAT_R16G16B16A16_SFLOAT, windowPtr->windowWidth, windowPtr->windowHeight, imagebuffers[IMAGE_INDEX_GBUFFER_POS]);
    memPtr->create_fb_image(devicePtr->vulkanDevice, VK_FORMAT_R16G16B16A16_SFLOAT, windowPtr->windowWidth, windowPtr->windowHeight, imagebuffers[IMAGE_INDEX_GBUFFER_NORM]);
    memPtr->create_fb_image(devicePtr->vulkanDevice, VK_FORMAT_R16G16B16A16_SFLOAT, windowPtr->windowWidth, windowPtr->windowHeight, imagebuffers[IMAGE_INDEX_GBUFFER_TEX]);
    memPtr->create_fb_image(devicePtr->vulkanDevice, VK_FORMAT_R8G8B8A8_SNORM, windowPtr->windowWidth, windowPtr->windowHeight, imagebuffers[IMAGE_INDEX_GBUFFER_ALBEDO]);

    memPtr->create_fb_image(devicePtr->vulkanDevice, shadowmapFormat, shadowmapSize, shadowmapSize, imagebuffers[IMAGE_INDEX_SHADOWMAP]);
    memPtr->transitionImage(devicePtr, vulkanGraphicsQueue, imagebuffers[IMAGE_INDEX_SHADOWMAP]->image, 1, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    memPtr->create_fb_image(devicePtr->vulkanDevice, shadowmapFormat, shadowmapSize, shadowmapSize, imagebuffers[IMAGE_INDEX_SHADOWMAP_BLUR]);
    memPtr->transitionImage(devicePtr, vulkanGraphicsQueue, imagebuffers[IMAGE_INDEX_SHADOWMAP_BLUR]->image, 1, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    memPtr->create_depth_image(devicePtr, vulkanGraphicsQueue, vulkanDepthFormat, shadowmapSize, shadowmapSize, imagebuffers[IMAGE_INDEX_SHADOWMAP_DEPTH]);

    memPtr->create_fb_image(devicePtr->vulkanDevice, VK_FORMAT_R32_SFLOAT, windowPtr->windowWidth, windowPtr->windowHeight, imagebuffers[IMAGE_INDEX_AO]);
    memPtr->create_fb_image(devicePtr->vulkanDevice, VK_FORMAT_R32_SFLOAT, windowPtr->windowWidth, windowPtr->windowHeight, imagebuffers[IMAGE_INDEX_AO_BLUR]);
    memPtr->transitionImage(devicePtr, vulkanGraphicsQueue, imagebuffers[IMAGE_INDEX_AO_BLUR]->image, 1, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    uint32_t miplevel;
    //memPtr->load_texture_image(devicePtr, vulkanGraphicsQueue, "skys/Hamarikyu_Bridge_B/14-Hamarikyu_Bridge_B_3k.hdr", imagebuffers[IMAGE_INDEX_SKYDOME], miplevel, VK_IMAGE_LAYOUT_GENERAL);
    //memPtr->load_texture_image(devicePtr, vulkanGraphicsQueue, "skys/Arches_E_PineTree/Arches_E_PineTree_3k.hdr", imagebuffers[IMAGE_INDEX_SKYDOME], miplevel, VK_IMAGE_LAYOUT_GENERAL);
    //memPtr->load_texture_image(devicePtr, vulkanGraphicsQueue, "skys/Alexs_Apartment/Alexs_Apt_2k.hdr", imagebuffers[IMAGE_INDEX_SKYDOME], miplevel, VK_IMAGE_LAYOUT_GENERAL);
    //memPtr->load_texture_image(devicePtr, vulkanGraphicsQueue, "skys/HS-Cave-Room/Mt-Washington-Cave-Room_Ref.hdr", imagebuffers[IMAGE_INDEX_SKYDOME], miplevel, VK_IMAGE_LAYOUT_GENERAL);
    //memPtr->load_texture_image(devicePtr, vulkanGraphicsQueue, "skys/Chelsea_Stairs/Chelsea_Stairs_3k.hdr", imagebuffers[IMAGE_INDEX_SKYDOME], miplevel, VK_IMAGE_LAYOUT_GENERAL);
    //memPtr->load_texture_image(devicePtr, vulkanGraphicsQueue, "skys/snow_machine/test8_Ref.hdr", imagebuffers[IMAGE_INDEX_SKYDOME], miplevel, VK_IMAGE_LAYOUT_GENERAL);
    //memPtr->load_texture_image(devicePtr, vulkanGraphicsQueue, "skys/Popcorn_Lobby/Lobby-Center_2k.hdr", imagebuffers[IMAGE_INDEX_SKYDOME], miplevel, VK_IMAGE_LAYOUT_GENERAL);
    //memPtr->load_texture_image(devicePtr, vulkanGraphicsQueue, "skys/QueenMary_Chimney/QueenMary_Chimney_Ref.hdr", imagebuffers[IMAGE_INDEX_SKYDOME], miplevel, VK_IMAGE_LAYOUT_GENERAL);
    //memPtr->load_texture_image(devicePtr, vulkanGraphicsQueue, "skys/Theatre_Seating/Theatre-Side_2k.hdr", imagebuffers[IMAGE_INDEX_SKYDOME], miplevel, VK_IMAGE_LAYOUT_GENERAL);
    //memPtr->load_texture_image(devicePtr, vulkanGraphicsQueue, "skys/NatureLab/NatureLabFront_IBL_Ref.hdr", imagebuffers[IMAGE_INDEX_SKYDOME], miplevel, VK_IMAGE_LAYOUT_GENERAL);
    //memPtr->load_texture_image(devicePtr, vulkanGraphicsQueue, "skys/Milkyway/Milkyway_small.hdr", imagebuffers[IMAGE_INDEX_SKYDOME], miplevel, VK_IMAGE_LAYOUT_GENERAL);
    //memPtr->load_texture_image(devicePtr, vulkanGraphicsQueue, "skys/Winter_Forest/WinterForest_Ref.hdr", imagebuffers[IMAGE_INDEX_SKYDOME], miplevel, VK_IMAGE_LAYOUT_GENERAL);
    //memPtr->load_texture_image(devicePtr, vulkanGraphicsQueue, "skys/Wooden_Door/WoodenDoor_Ref.hdr", imagebuffers[IMAGE_INDEX_SKYDOME], miplevel, VK_IMAGE_LAYOUT_GENERAL);
    memPtr->load_texture_image(devicePtr, vulkanGraphicsQueue, "skys/Footprint_Court/Footprint_Court_2k.hdr", imagebuffers[IMAGE_INDEX_SKYDOME], miplevel, VK_IMAGE_LAYOUT_GENERAL);
    
    memPtr->generate_filteredtex(devicePtr, vulkanGraphicsQueue, vulkanComputeQueue, imagebuffers[IMAGE_INDEX_SKYDOME], imagebuffers[IMAGE_INDEX_SKYDOME_IRRADIANCE], vulkanSamplers[SAMPLE_INDEX_NORMAL]);
    memPtr->create_sampler(devicePtr, vulkanSamplers[SAMPLE_INDEX_SKYDOME], miplevel);

    {
        std::vector<float> vertices = {
            1.0f, 1.0f,   1.0f, 1.0f,
            1.0f,-1.0f,   1.0f, 0.0f,
           -1.0f, 1.0f,   0.0f, 1.0f,
           -1.0f,-1.0f,   0.0f, 0.0f,
        };

        std::vector<uint32_t> indices = {
            0, 1, 2,
            2, 1, 3,
        };

        memPtr->create_vertex_index_buffer(devicePtr->vulkanDevice, vulkanGraphicsQueue, devicePtr, vertices, indices, vertexbuffers[VERTEX_INDEX_FULLSCREENQUAD]);
    }

    {
        std::vector<float> vertices = {
            1.0f, 1.0f,
            1.0f,-1.0f,
           -1.0f, 1.0f,
           -1.0f,-1.0f,
        };

        std::vector<uint32_t> indices = {
            0, 1, 2,
            2, 1, 3,
        };

        memPtr->create_vertex_index_buffer(devicePtr->vulkanDevice, vulkanGraphicsQueue, devicePtr, vertices, indices, vertexbuffers[VERTEX_INDEX_QUAD_POSONLY]);
    }

    {
        std::vector<helper::vertindex> bunnydata = helper::readassimp("data/model/bunny.ply");
        memPtr->create_vertex_index_buffer(devicePtr->vulkanDevice, vulkanGraphicsQueue, devicePtr, bunnydata[0].vertices, bunnydata[0].indices, vertexbuffers[VERTEX_INDEX_BUNNY]);
        object* newobject = new object();
        newobject->create_object(static_cast<unsigned int>(bunnydata[0].indices.size()), vertexbuffers[VERTEX_INDEX_BUNNY]);
        objects.push_back(newobject);

        helper::vertindex vertexdata = generateSphere();
        memPtr->create_vertex_index_buffer(devicePtr->vulkanDevice, vulkanGraphicsQueue, devicePtr, vertexdata.vertices, vertexdata.indices, vertexbuffers[VERTEX_INDEX_SPHERE]);
        newobject = new object();
        newobject->create_object(static_cast<unsigned int>(vertexdata.indices.size()), vertexbuffers[VERTEX_INDEX_SPHERE]);
        objects.push_back(newobject);

        vertexdata = generateSphereposonly();
        memPtr->create_vertex_index_buffer(devicePtr->vulkanDevice, vulkanGraphicsQueue, devicePtr, vertexdata.vertices, vertexdata.indices, vertexbuffers[VERTEX_INDEX_SPHERE_POSONLY]);

        vertexdata = generateBoxposonly();
        memPtr->create_vertex_index_buffer(devicePtr->vulkanDevice, vulkanGraphicsQueue, devicePtr, vertexdata.vertices, vertexdata.indices, vertexbuffers[VERTEX_INDEX_BOX_POSONLY]);

        vertexdata = generateBox();
        memPtr->create_vertex_index_buffer(devicePtr->vulkanDevice, vulkanGraphicsQueue, devicePtr, vertexdata.vertices, vertexdata.indices, vertexbuffers[VERTEX_INDEX_BOX]);
        newobject = new object();
        newobject->create_object(static_cast<unsigned int>(vertexdata.indices.size()), vertexbuffers[VERTEX_INDEX_BOX]);
        objects.push_back(newobject);
        newobject = new object();
        newobject->create_object(static_cast<unsigned int>(vertexdata.indices.size()), vertexbuffers[VERTEX_INDEX_BOX]);
        objects.push_back(newobject);
        newobject = new object();
        newobject->create_object(static_cast<unsigned int>(vertexdata.indices.size()), vertexbuffers[VERTEX_INDEX_BOX]);
        objects.push_back(newobject);
        newobject = new object();
        newobject->create_object(static_cast<unsigned int>(vertexdata.indices.size()), vertexbuffers[VERTEX_INDEX_BOX]);
        objects.push_back(newobject);

        std::vector<helper::vertindex> armadillodata = helper::readassimp("data/model/armadillo.ply");
        memPtr->create_vertex_index_buffer(devicePtr->vulkanDevice, vulkanGraphicsQueue, devicePtr, armadillodata[0].vertices, armadillodata[0].indices, vertexbuffers[VERTEX_INDEX_ARMADILLO]);
        newobject = new object();
        newobject->create_object(static_cast<unsigned int>(armadillodata[0].indices.size()), vertexbuffers[VERTEX_INDEX_ARMADILLO]);
        objects.push_back(newobject);

        std::vector<helper::vertindex> roomdata = helper::readassimp("data/model/room.ply");
        memPtr->create_vertex_index_buffer(devicePtr->vulkanDevice, vulkanGraphicsQueue, devicePtr, roomdata[0].vertices, roomdata[0].indices, vertexbuffers[VERTEX_INDEX_ROOM]);
        newobject = new object();
        newobject->create_object(static_cast<unsigned int>(roomdata[0].indices.size()), vertexbuffers[VERTEX_INDEX_ROOM]);
        objects.push_back(newobject);
        newobject = new object();
        newobject->create_object(static_cast<unsigned int>(roomdata[0].indices.size()), vertexbuffers[VERTEX_INDEX_ROOM]);
        objects.push_back(newobject);

        newobject = new object();
        newobject->create_object(static_cast<unsigned int>(vertexdata.indices.size()), vertexbuffers[VERTEX_INDEX_BOX]);
        objects.push_back(newobject);

        newobject = new object();
        newobject->create_object(static_cast<unsigned int>(armadillodata[0].indices.size()), vertexbuffers[VERTEX_INDEX_ARMADILLO]);
        objects.push_back(newobject);
        newobject = new object();
        newobject->create_object(static_cast<unsigned int>(armadillodata[0].indices.size()), vertexbuffers[VERTEX_INDEX_ARMADILLO]);
        objects.push_back(newobject);
        newobject = new object();
        newobject->create_object(static_cast<unsigned int>(armadillodata[0].indices.size()), vertexbuffers[VERTEX_INDEX_ARMADILLO]);
        objects.push_back(newobject);

        newobject = new object();
        newobject->create_object(static_cast<unsigned int>(bunnydata[0].indices.size()), vertexbuffers[VERTEX_INDEX_BUNNY]);
        objects.push_back(newobject);
        newobject = new object();
        newobject->create_object(static_cast<unsigned int>(bunnydata[0].indices.size()), vertexbuffers[VERTEX_INDEX_BUNNY]);
        objects.push_back(newobject);
        newobject = new object();
        newobject->create_object(static_cast<unsigned int>(bunnydata[0].indices.size()), vertexbuffers[VERTEX_INDEX_BUNNY]);
        objects.push_back(newobject);

        vertexdata = generateSphere();
        newobject = new object();
        newobject->create_object(static_cast<unsigned int>(vertexdata.indices.size()), vertexbuffers[VERTEX_INDEX_SPHERE]);
        objects.push_back(newobject);

        for (int i = 0; i < 25; ++i)
        {
            newobject = new object();
            newobject->create_object(static_cast<unsigned int>(vertexdata.indices.size()), vertexbuffers[VERTEX_INDEX_SPHERE]);
            objects.push_back(newobject);
        }
    }

    memPtr->create_buffer(devicePtr->vulkanDevice, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sizeof(Projection), 1, uniformbuffers[UNIFORM_INDEX_PROJECTION]);
    memPtr->create_buffer(devicePtr->vulkanDevice, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, /*sizeof(ObjectProperties)*/128, 128, uniformbuffers[UNIFORM_INDEX_OBJECT]);
    memPtr->create_buffer(devicePtr->vulkanDevice, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sizeof(lightSetting), 1, uniformbuffers[UNIFORM_INDEX_LIGHT_SETTING]);
    memPtr->create_buffer(devicePtr->vulkanDevice, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sizeof(camera), 1, uniformbuffers[UNIFORM_INDEX_CAMERA]);
    memPtr->create_buffer(devicePtr->vulkanDevice, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, /*sizeof(light)*/64, 64, uniformbuffers[UNIFORM_INDEX_LIGHT]);
    memPtr->create_buffer(devicePtr->vulkanDevice, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sizeof(shadow), 1, uniformbuffers[UNIFORM_INDEX_SHADOW_SETTING]);
    memPtr->create_buffer(devicePtr->vulkanDevice, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, /*sizeof(HammersleyBlock)*/1616, 1, uniformbuffers[UNIFORM_INDEX_HAMMERSLEYBLOCK]);
    memPtr->create_buffer(devicePtr->vulkanDevice, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, /*sizeof(HammersleyBlock)*/1616, 1, uniformbuffers[UNIFORM_INDEX_GAUSSIANWEIGHT]);
    memPtr->create_buffer(devicePtr->vulkanDevice, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sizeof(aoConstant), 1, uniformbuffers[UNIFORM_INDEX_AO_CONSTANT]);

    {
        GaussianWeight weight;

        weight.build(9);

        void* data;
        vkMapMemory(devicePtr->vulkanDevice, uniformbuffers[UNIFORM_INDEX_GAUSSIANWEIGHT].memory, 0, uniformbuffers[UNIFORM_INDEX_GAUSSIANWEIGHT].range, 0, &data);
        memcpy(data, &weight.w, sizeof(int));
        data = static_cast<int*>(data) + 4;
        for (int i = 0; i < weight.w * 2 + 1; ++i)
        {
            memcpy(data, &weight.weight[i], sizeof(float) * 2);
            data = static_cast<int*>(data) + 4;
        }
        vkUnmapMemory(devicePtr->vulkanDevice, uniformbuffers[UNIFORM_INDEX_GAUSSIANWEIGHT].memory);
    }

    updatebuffer();
}

void freebuffer()
{
    for (auto& vertexbuffer : vertexbuffers)
    {
        memPtr->free_buffer(devicePtr->vulkanDevice, vertexbuffer.vertexbuffer);
        memPtr->free_buffer(devicePtr->vulkanDevice, vertexbuffer.indexbuffer);
    }

    for (auto& uniformbuffer : uniformbuffers)
    {
        memPtr->free_buffer(devicePtr->vulkanDevice, uniformbuffer);
    }

    for (auto& image : imagebuffers)
    {
        memPtr->free_image(devicePtr->vulkanDevice, image);
        delete image;
    }
}

void init()
{
    compileshader();
    windowPtr->create_window();
    createInstance();
    debug::init(vulkanInstance);
    devicePtr->select_physical_device(vulkanInstance);
    memPtr->init(devicePtr->vulkandeviceMemoryProperties);
    windowPtr->create_surface(vulkanInstance, vulkanSurface);
    devicePtr->create_logical_device(vulkanInstance, vulkanSurface, SampleRate_Shading | Geometry_Shader | Pipeline_Statistics_Query);
    devicePtr->request_queue(vulkanGraphicsQueue, vulkanComputeQueue);
    devicePtr->create_command_pool();

    createswapchain();
    createDepth();
    createSemaphoreFence();

    createcommandbuffer();
    setupbuffer();
    createRenderpass();
    createquerypool();
    createdescriptorset();
    createPipeline();
}

void update(float dt)
{
    glm::vec3 look = rotation * FORWARD;
    glm::vec3 right = rotation * RIGHT;

    if (windowPtr->triggered[GLFW_KEY_C])
    {
        std::cout << "Camera status captured!" << std::endl;
        std::cout << "position : " << camerapos << std::endl;
        std::cout << "rotation : " << glm::eulerAngles(rotation) << std::endl;
    }

    float cam_speed = 5.0f;
    if (windowPtr->pressed[GLFW_KEY_LEFT_SHIFT])
    {
        cam_speed *= 3.0f;
    }

    if (windowPtr->pressed[GLFW_KEY_W])
    {
        camerapos += look * cam_speed * dt;
    }
    if (windowPtr->pressed[GLFW_KEY_S])
    {
        camerapos -= look * cam_speed * dt;
    }
    if (windowPtr->pressed[GLFW_KEY_A])
    {
        camerapos -= right * cam_speed * dt;
    }
    if (windowPtr->pressed[GLFW_KEY_D])
    {
        camerapos += right * cam_speed * dt;
    }

    static float pitch = 0.0f;
    static float yaw = 0.0f;

    if (windowPtr->mousepressed[GLFW_MOUSE_BUTTON_RIGHT])
    {
        if (windowPtr->mousemovex != 0.0f)
        {
            yaw -= 0.01f * windowPtr->mousemovex;

            rotation = glm::quat(glm::vec3(pitch, yaw, 0.0f));
        }

        if (windowPtr->mousemovey != 0.0f)
        {
            pitch -= 0.01f * windowPtr->mousemovey;

            pitch = (pitch > -PI_HALF) ? pitch : -PI_HALF;
            pitch = (pitch < PI_HALF) ? pitch : PI_HALF;

            rotation = glm::quat(glm::vec3(pitch, yaw, 0.0f));
        }
    }

    if (windowPtr->pressed[GLFW_KEY_ESCAPE])
    {
        windowPtr->shouldclose = true;
    }
}

void close()
{
    vkDeviceWaitIdle(devicePtr->vulkanDevice);

    for (auto& obj : objects)
    {
        delete obj;
    }
    objects.clear();

    freebuffer();

    for (uint32_t i = 0; i < render::PIPELINE_MAX; ++i)
    {
        pipeline::closepipeline(devicePtr->vulkanDevice, vulkanPipelines[i]);
        pipeline::close_pipelinelayout(devicePtr->vulkanDevice, vulkanPipelineLayouts[i]);
    }

    for (auto& descriptorsetlayout : vulkanDescriptorSetLayouts)
    {
        descriptor::close_descriptorset_layout(devicePtr->vulkanDevice, descriptorsetlayout);
    }

    descriptor::close_descriptorpool(devicePtr->vulkanDevice, vulkanDescriptorPool);

    for (auto& framebuffer : vulkanFramebuffers)
    {
        renderpass::close_framebuffer(devicePtr->vulkanDevice, framebuffer);
    }

    for (auto& renderpass : vulkanRenderpasses)
    {
        renderpass::close_renderpass(devicePtr->vulkanDevice, renderpass);
    }

    devicePtr->free_command_buffer(static_cast<uint32_t>(vulkanCommandBuffers.size()), vulkanCommandBuffers.data(), static_cast<uint32_t>(vulkanComputeCommandBuffers.size()), vulkanComputeCommandBuffers.data());

    for (auto sampler : vulkanSamplers)
    {
        vkDestroySampler(devicePtr->vulkanDevice, sampler, VK_NULL_HANDLE);
    }

    guiPtr->close(devicePtr->vulkanDevice);
    windowPtr->close_window();
    vkDestroyPipelineCache(devicePtr->vulkanDevice, vulkanPipelineCache, VK_NULL_HANDLE);

    for (auto& semaphore : vulkanSemaphores)
    {
        vkDestroySemaphore(devicePtr->vulkanDevice, semaphore, VK_NULL_HANDLE);
    }

    for (auto fence : vulkanWaitFences)
    {
        vkDestroyFence(devicePtr->vulkanDevice, fence, VK_NULL_HANDLE);
    }
    vkDestroyQueryPool(devicePtr->vulkanDevice, vulkanPipelineStatisticsQuery, VK_NULL_HANDLE);
    vkDestroyQueryPool(devicePtr->vulkanDevice, vulkanTimestampQuery, VK_NULL_HANDLE);
    for (auto swapchainimage : swapchainImages)
    {
        vkDestroyImageView(devicePtr->vulkanDevice, swapchainimage.imageView, VK_NULL_HANDLE);
    }
    vkDestroySwapchainKHR(devicePtr->vulkanDevice, vulkanSwapchain, VK_NULL_HANDLE);

    devicePtr->close();
    vkDestroySurfaceKHR(vulkanInstance, vulkanSurface, VK_NULL_HANDLE);
    debug::closedebug(vulkanInstance);
    vkDestroyInstance(vulkanInstance, VK_NULL_HANDLE);

    delete guiPtr;
    delete memPtr;
    delete devicePtr;
    delete windowPtr;
}

int main(void)
{
    init();

    guiPtr->init(vulkanInstance, vulkanGraphicsQueue, swapchainImageCount, windowPtr->glfwWindow, vulkanRenderpasses[render::RENDERPASS_SWAPCHAIN], devicePtr);

    uint32_t imageIndex = 0;
    while (windowPtr->update_window_frame())
    {
        update(windowPtr->dt);
        updatebuffer();

        vkAcquireNextImageKHR(devicePtr->vulkanDevice, vulkanSwapchain, UINT64_MAX, vulkanSemaphores[render::SEMAPHORE_PRESENT], 0, &imageIndex);
        uint32_t framebufferindex = render::RENDERPASS_SWAPCHAIN + imageIndex;
        uint32_t commandbufferindex = render::COMMANDBUFFER_SWAPCHAIN + imageIndex;

        guiPtr->pre_upate();

        if (ImGui::BeginMainMenuBar()) 
        {
            if (ImGui::BeginMenu("Objects")) 
            {
                if (ImGui::BeginMenu("Properties##Object"))
                {
                    ImGui::ColorPicker3("Albedo", &objproperties.albedoColor.x);
                    ImGui::DragFloat("Roughness", &objproperties.roughness, 0.001f, 0.0f, 1.0f);
                    ImGui::DragFloat("Metallic", &objproperties.metallic, 0.001f, 0.0f, 1.0f);
                    ImGui::DragFloat("RefractiveIndex", &objproperties.refractiveindex, 0.005f, 1.0f, 5.0f);

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("SUN##Object"))
                {
                    ImGui::DragFloat3("color##SUN", &sun.color.x);
                    ImGui::DragFloat3("position##SUN", &sun.position.x, 0.005f);
                    ImGui::DragFloat3("direction##SUN", &sun.direction.x, 0.01f);
                    ImGui::DragFloat("rad##SUN", &sun.radius, 0.0001f, 0.0f, 1.00f);

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("LOCAL1LIGHT##Object"))
                {
                    ImGui::DragFloat3("color##LOCAL1", &local_light[0].color.x);
                    ImGui::DragFloat3("position##LOCAL1", &local_light[0].position.x, 0.005f);
                    ImGui::DragFloat("radius##LOCAL1", &local_light[0].radius, 0.01f, 0.0f, 50.0f);

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("LOCAL2LIGHT##Object"))
                {
                    ImGui::DragFloat3("color##LOCAL12", &local_light[1].color.x);
                    ImGui::DragFloat3("position##LOCAL12", &local_light[1].position.x, 0.005f);
                    ImGui::DragFloat("radius##LOCAL12", &local_light[1].radius, 0.01f, 0.0f, 50.0f);

                    ImGui::EndMenu();
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Menu")) 
            {
                if (ImGui::BeginMenu("Performance"))
                {
                    ImGui::Text("Gbuffer pass : %fms", pipeline_timestamps[TIMESTAMP::TIMESTAMP_GBUFFER_START / 2]);
                    ImGui::Text("Blur pass : %fms", pipeline_timestamps[TIMESTAMP::TIMESTAMP_BLUR_VERTICAL_START / 2] + pipeline_timestamps[TIMESTAMP::TIMESTAMP_BLUR_VERTICAL_END / 2]);
                    ImGui::Text("Lighting pass : %fms", pipeline_timestamps[TIMESTAMP::TIMESTAMP_LIGHTING_START / 2]);

                    ImGui::EndMenu();
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Setting"))
            {
                ImGui::Checkbox("ShadowEnable", &lightsetting.shadowenable);

                if (ImGui::BeginMenu("Shadow"))
                {
                    ImGui::Checkbox("Blur Shadow", &blurshadow);

                    ImGui::DragFloat("Depth Bias", &shadowsetting.depthBias, 0.00001f, 0.0f, 0.1f, "%.5f");
                    ImGui::DragFloat("Shadow Bias", &shadowsetting.shadowBias, 0.00001f, 0.0f, 0.1f, "%.5f");
                    ImGui::DragFloat("Shadow Far_plane", &shadowsetting.far_plane, 1.0f, 0.0f, 100.0f);

                    const char* items[] = { "None", "Monument Shader Mapping", "No Shadow" };

                    if (ImGui::BeginCombo("Shadow Type", items[shadowsetting.type]))
                    {
                        for (int n = 0; n < IM_ARRAYSIZE(items); n++)
                        {
                            bool is_selected = (shadowsetting.type == n);
                            if (ImGui::Selectable(items[n], is_selected))
                            {
                                shadowsetting.type = n;
                                if (is_selected)
                                {
                                    ImGui::SetItemDefaultFocus();
                                }
                            }
                        }
                        ImGui::EndCombo();
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Ambient Occulusion"))
                {
                    ImGui::DragFloat("s##AOCONSTANT", &AOConstant.s, 0.001f, 0.0f, 1.0f);
                    ImGui::DragFloat("k##AOCONSTANT", &AOConstant.k, 0.01f, 0.0f, 10.0f);
                    ImGui::DragFloat("R##AOCONSTANT", &AOConstant.R, 0.1f, 0.1f, 100.0f);
                    ImGui::DragInt("N##AOCONSTANT", &AOConstant.n, 0.1f, 1, 100);

                    ImGui::EndMenu();
                }

                {
                    ImGui::DragInt("Hammersley block size", &hammersley_N, 1, 1, 100);
                }

                if (ImGui::BeginMenu("Deferred Target"))
                {
                    bool deferred[outputMode::OUTPUTMODE_MAX] = { false };
                    deferred[lightsetting.outputTex] = true;

                    if (ImGui::MenuItem("Position##Target", "", deferred[outputMode::POSITION])) lightsetting.outputTex = outputMode::POSITION;
                    else if (ImGui::MenuItem("Normal##Target", "", deferred[outputMode::NORMAL])) lightsetting.outputTex = outputMode::NORMAL;
                    else if (ImGui::MenuItem("TextureCoordinate##Target", "", deferred[outputMode::TEXTURECOORDINATE])) lightsetting.outputTex = outputMode::TEXTURECOORDINATE;
                    else if (ImGui::MenuItem("ObjectColor##Target", "", deferred[outputMode::ALBEDO])) lightsetting.outputTex = outputMode::ALBEDO;
                    else if (ImGui::MenuItem("Lighting##Target", "", deferred[outputMode::LIGHTING])) lightsetting.outputTex = outputMode::LIGHTING;
                    else if (ImGui::MenuItem("Shadow Map##Target", "", deferred[outputMode::SHADOW_MAP])) lightsetting.outputTex = outputMode::SHADOW_MAP;
                    else if (ImGui::MenuItem("Only Shadow##Target", "", deferred[outputMode::ONLY_SHADOW])) lightsetting.outputTex = outputMode::ONLY_SHADOW;

                    ImGui::EndMenu();
                }

                ImGui::Checkbox("HighDynamicRange", &lightsetting.highdynamicrange);
                ImGui::DragFloat("Gamma", &lightsetting.gamma, 0.01f, 0.0f, 10.0f);
                ImGui::DragFloat("Exposure", &lightsetting.exposure, 1.0f, 0.1f, 10000.0f);

                {
                    bool lightspacevisible = lightspacetoggle;
                    ImGui::MenuItem("VisibleLightSpace", 0, &lightspacevisible);

                    if (lightspacevisible != lightspacetoggle)
                    {
                        lightspacetoggle = lightspacevisible;
                        if (lightspacetoggle)
                        {
                            for (auto& lit : local_light)
                            {
                                lit.type = LIGHT_TYPE::LIGHT_SPACE_DRAW;
                            }
                        }
                        else
                        {
                            for (auto& lit : local_light)
                            {
                                lit.type = LIGHT_TYPE::LIGHT_NONE;
                            }
                        }
                    }
                }

                ImGui::MenuItem("LocalLightDisabled", 0, &nolocallight);
                
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }
        ImGui::Render();

        //gbuffer pass
        {
            VkCommandBufferBeginInfo commandBufferBeginInfo{};
            commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

            std::array<VkClearValue, 5> gbufferclearValues;
            gbufferclearValues[0].color = { { 0.25f, 0.25f, 0.25f, 1.0f } };
            gbufferclearValues[1].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
            gbufferclearValues[2].color = { { 0.25f, 0.25f, 0.25f, 1.0f } };
            gbufferclearValues[3].color = { { 0.25f, 0.25f, 0.25f, 1.0f } };
            gbufferclearValues[4].depthStencil = { 1.0f, 0 };

            VkRenderPassBeginInfo renderPassBeginInfo{};
            renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassBeginInfo.renderPass = vulkanRenderpasses[render::RENDERPASS_GBUFFER];
            renderPassBeginInfo.renderArea.offset.x = 0;
            renderPassBeginInfo.renderArea.offset.y = 0;
            renderPassBeginInfo.renderArea.extent.width = windowPtr->windowWidth;
            renderPassBeginInfo.renderArea.extent.height = windowPtr->windowHeight;
            renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(gbufferclearValues.size());
            renderPassBeginInfo.pClearValues = gbufferclearValues.data();
            renderPassBeginInfo.framebuffer = vulkanFramebuffers[render::RENDERPASS_GBUFFER];

            if (vkBeginCommandBuffer(vulkanCommandBuffers[render::COMMANDBUFFER_GBUFFER], &commandBufferBeginInfo) != VK_SUCCESS)
            {
                std::cout << "failed to begin command buffer!" << std::endl;
                return -1;
            }
            
            vkCmdResetQueryPool(vulkanCommandBuffers[render::COMMANDBUFFER_GBUFFER], vulkanTimestampQuery, TIMESTAMP::TIMESTAMP_GBUFFER_START, 2);
            vkCmdWriteTimestamp(vulkanCommandBuffers[render::COMMANDBUFFER_GBUFFER], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, vulkanTimestampQuery, TIMESTAMP::TIMESTAMP_GBUFFER_START);

            vkCmdBeginRenderPass(vulkanCommandBuffers[render::COMMANDBUFFER_GBUFFER], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(vulkanCommandBuffers[render::COMMANDBUFFER_GBUFFER], VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipelines[render::PIPELINE_GBUFFER]);

            std::array<uint32_t, 1> dynamicoffsets = { 0 };

            for (auto obj : objects)
            {
                vkCmdBindDescriptorSets(vulkanCommandBuffers[render::COMMANDBUFFER_GBUFFER], VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipelineLayouts[render::PIPELINE_GBUFFER], 0, 1, 
                    &vulkanDescriptorSets[render::PIPELINE_GBUFFER], static_cast<uint32_t>(dynamicoffsets.size()), dynamicoffsets.data());

                obj->draw_object(vulkanCommandBuffers[render::COMMANDBUFFER_GBUFFER]);
                dynamicoffsets[0] += 128;
            }

            vkCmdEndRenderPass(vulkanCommandBuffers[render::COMMANDBUFFER_GBUFFER]);

            vkCmdWriteTimestamp(vulkanCommandBuffers[render::COMMANDBUFFER_GBUFFER], VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, vulkanTimestampQuery, TIMESTAMP::TIMESTAMP_GBUFFER_END);

            if (vkEndCommandBuffer(vulkanCommandBuffers[render::COMMANDBUFFER_GBUFFER]) != VK_SUCCESS)
            {
                std::cout << "failed to end commnad buffer!" << std::endl;
                return -1;
            }
        }

        //shadowmap pass
        {
            VkCommandBufferBeginInfo commandBufferBeginInfo{};
            commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

            std::array<VkClearValue, 2> shadowmapclearValues;
            //shadowmapclearValues[0].color = { { 0.999999989201f, 0.9975599302f, 0.8934375093f, 0.0000000003f } };
            shadowmapclearValues[0].color = { { 1.0f, 1.0f, 1.0f, 1.0f } };
            shadowmapclearValues[1].depthStencil = { 1.0f, 0 };

            VkRenderPassBeginInfo renderPassBeginInfo{};
            renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassBeginInfo.renderPass = vulkanRenderpasses[render::RENDERPASS_SHADOWMAP];
            renderPassBeginInfo.renderArea.offset.x = 0;
            renderPassBeginInfo.renderArea.offset.y = 0;
            renderPassBeginInfo.renderArea.extent.width = shadowmapSize;
            renderPassBeginInfo.renderArea.extent.height = shadowmapSize;
            renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(shadowmapclearValues.size());
            renderPassBeginInfo.pClearValues = shadowmapclearValues.data();
            renderPassBeginInfo.framebuffer = vulkanFramebuffers[render::RENDERPASS_SHADOWMAP];

            if (vkBeginCommandBuffer(vulkanCommandBuffers[render::COMMANDBUFFER_SHADOWMAP], &commandBufferBeginInfo) != VK_SUCCESS)
            {
                std::cout << "failed to begin command buffer!" << std::endl;
                return -1;
            }

            vkCmdBeginRenderPass(vulkanCommandBuffers[render::COMMANDBUFFER_SHADOWMAP], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(vulkanCommandBuffers[render::COMMANDBUFFER_SHADOWMAP], VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipelines[render::PIPELINE_SHADOWMAP]);

            std::array<uint32_t, 1> dynamicoffsets = { 0 };

            for (auto obj : objects)
            {
                vkCmdBindDescriptorSets(vulkanCommandBuffers[render::COMMANDBUFFER_SHADOWMAP], VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipelineLayouts[render::PIPELINE_SHADOWMAP], 0, 1, 
                    &vulkanDescriptorSets[render::DESCRIPTOR_SHADOWMAP], static_cast<uint32_t>(dynamicoffsets.size()), dynamicoffsets.data());

                obj->draw_object(vulkanCommandBuffers[render::COMMANDBUFFER_SHADOWMAP]);
                dynamicoffsets[0] += 128;
            }

            vkCmdEndRenderPass(vulkanCommandBuffers[render::COMMANDBUFFER_SHADOWMAP]);

            if (vkEndCommandBuffer(vulkanCommandBuffers[render::COMMANDBUFFER_SHADOWMAP]) != VK_SUCCESS)
            {
                std::cout << "failed to end commnad buffer!" << std::endl;
                return -1;
            }
        }

        //submit all commandbuffers
        {
            std::array<VkCommandBuffer, 2> submitcommandbuffers = { vulkanCommandBuffers[render::COMMANDBUFFER_GBUFFER], vulkanCommandBuffers[render::COMMANDBUFFER_SHADOWMAP] };

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            VkPipelineStageFlags pipelinestageFlag = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            std::array<VkPipelineStageFlags, 1> pipelinestageFlags = { pipelinestageFlag };
            submitInfo.pWaitDstStageMask = &pipelinestageFlag;
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = &vulkanSemaphores[render::SEMAPHORE_PRESENT];
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = (blurshadow.value) ? &vulkanSemaphores[render::SEMAPHORE_GBUFFER] : &vulkanSemaphores[render::SEMAPHORE_SHADOWMAP_BLUR_HORIZONTAL];
            submitInfo.commandBufferCount = static_cast<uint32_t>(submitcommandbuffers.size());
            submitInfo.pCommandBuffers = submitcommandbuffers.data();
            if (vkQueueSubmit(vulkanGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
            {
                std::cout << "failed to submit queue" << std::endl;
                return -1;
            }
        }

        if (blurshadow.isChanged())
        {
            updatelightingdescriptorset(blurshadow.value);
        }

        if (blurshadow.value)
        {
            descriptor::write_descriptorset(devicePtr->vulkanDevice, vulkanDescriptorSets[render::DESCRIPTOR_SHADOWMAP_BLUR], {
                {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0, {}, {vulkanSamplers[SAMPLE_INDEX_NORMAL], imagebuffers[IMAGE_INDEX_SHADOWMAP]->imageView, VK_IMAGE_LAYOUT_GENERAL}},
                {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, {}, {vulkanSamplers[SAMPLE_INDEX_NORMAL], imagebuffers[IMAGE_INDEX_SHADOWMAP_BLUR]->imageView, VK_IMAGE_LAYOUT_GENERAL}},
                {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, {uniformbuffers[UNIFORM_INDEX_GAUSSIANWEIGHT].buf, 0, uniformbuffers[UNIFORM_INDEX_GAUSSIANWEIGHT].range}, {}},
            });

            {
                VkCommandBufferBeginInfo commandBufferBeginInfo{};
                commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

                if (vkBeginCommandBuffer(vulkanComputeCommandBuffers[render::COMPUTECMDBUFFER_BLUR_VERTICAL], &commandBufferBeginInfo) != VK_SUCCESS)
                {
                    std::cout << "failed to begin command buffer!" << std::endl;
                    return -1;
                }

                // Reset timestamp query pool
                vkCmdResetQueryPool(vulkanComputeCommandBuffers[render::COMPUTECMDBUFFER_BLUR_VERTICAL], vulkanTimestampQuery, TIMESTAMP::TIMESTAMP_BLUR_VERTICAL_START, 2);
                vkCmdWriteTimestamp(vulkanComputeCommandBuffers[render::COMPUTECMDBUFFER_BLUR_VERTICAL], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, vulkanTimestampQuery, TIMESTAMP::TIMESTAMP_BLUR_VERTICAL_START);

                vkCmdBindPipeline(vulkanComputeCommandBuffers[render::COMPUTECMDBUFFER_BLUR_VERTICAL], VK_PIPELINE_BIND_POINT_COMPUTE, vulkanPipelines[render::PIPELINE_SHADOWMAP_BLUR_VERTICAL]);
                vkCmdBindDescriptorSets(vulkanComputeCommandBuffers[render::COMPUTECMDBUFFER_BLUR_VERTICAL], VK_PIPELINE_BIND_POINT_COMPUTE, vulkanPipelineLayouts[render::PIPELINE_SHADOWMAP_BLUR_VERTICAL], 0, 1,
                    &vulkanDescriptorSets[render::DESCRIPTOR_SHADOWMAP_BLUR], 0, 0);

                vkCmdDispatch(vulkanComputeCommandBuffers[render::COMPUTECMDBUFFER_BLUR_VERTICAL], shadowmapSize / 128, shadowmapSize, 1);

                vkCmdWriteTimestamp(vulkanComputeCommandBuffers[render::COMPUTECMDBUFFER_BLUR_VERTICAL], VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, vulkanTimestampQuery, TIMESTAMP::TIMESTAMP_BLUR_VERTICAL_END);

                vkEndCommandBuffer(vulkanComputeCommandBuffers[render::COMPUTECMDBUFFER_BLUR_VERTICAL]);

                VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

                // Submit compute commands
                VkSubmitInfo submitInfo{};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = &vulkanComputeCommandBuffers[render::COMPUTECMDBUFFER_BLUR_VERTICAL];
                submitInfo.waitSemaphoreCount = 1;
                submitInfo.pWaitSemaphores = &vulkanSemaphores[render::SEMAPHORE_GBUFFER];
                submitInfo.pWaitDstStageMask = &waitStageMask;
                submitInfo.signalSemaphoreCount = 1;
                submitInfo.pSignalSemaphores = &vulkanSemaphores[render::SEMAPHORE_SHADOWMAP_BLUR_VERTICAL];
                if (vkQueueSubmit(vulkanComputeQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
                {
                    std::cout << "failed to submit queue" << std::endl;
                    return -1;
                }
            }

            vkQueueWaitIdle(vulkanComputeQueue);

            descriptor::write_descriptorset(devicePtr->vulkanDevice, vulkanDescriptorSets[render::DESCRIPTOR_SHADOWMAP_BLUR], {
                {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0, {}, {vulkanSamplers[SAMPLE_INDEX_NORMAL], imagebuffers[IMAGE_INDEX_SHADOWMAP_BLUR]->imageView, VK_IMAGE_LAYOUT_GENERAL}},
                {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, {}, {vulkanSamplers[SAMPLE_INDEX_NORMAL], imagebuffers[IMAGE_INDEX_SHADOWMAP_BLUR]->imageView, VK_IMAGE_LAYOUT_GENERAL}},
                {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, {uniformbuffers[UNIFORM_INDEX_GAUSSIANWEIGHT].buf, 0, uniformbuffers[UNIFORM_INDEX_GAUSSIANWEIGHT].range}, {}},
            });

            {
                VkCommandBufferBeginInfo commandBufferBeginInfo{};
                commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

                if (vkBeginCommandBuffer(vulkanComputeCommandBuffers[render::COMPUTECMDBUFFER_BLUR_HORIZONTAL], &commandBufferBeginInfo) != VK_SUCCESS)
                {
                    std::cout << "failed to begin command buffer!" << std::endl;
                    return -1;
                }

                // Reset timestamp query pool
                vkCmdResetQueryPool(vulkanComputeCommandBuffers[render::COMPUTECMDBUFFER_BLUR_HORIZONTAL], vulkanTimestampQuery, TIMESTAMP::TIMESTAMP_BLUR_HORIZONTAL_START, 2);
                vkCmdWriteTimestamp(vulkanComputeCommandBuffers[render::COMPUTECMDBUFFER_BLUR_HORIZONTAL], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, vulkanTimestampQuery, TIMESTAMP::TIMESTAMP_BLUR_HORIZONTAL_START);

                vkCmdBindPipeline(vulkanComputeCommandBuffers[render::COMPUTECMDBUFFER_BLUR_HORIZONTAL], VK_PIPELINE_BIND_POINT_COMPUTE, vulkanPipelines[render::PIPELINE_SHADOWMAP_BLUR_HORIZONTAL]);
                vkCmdBindDescriptorSets(vulkanComputeCommandBuffers[render::COMPUTECMDBUFFER_BLUR_HORIZONTAL], VK_PIPELINE_BIND_POINT_COMPUTE, vulkanPipelineLayouts[render::PIPELINE_SHADOWMAP_BLUR_HORIZONTAL], 0, 1,
                    &vulkanDescriptorSets[render::DESCRIPTOR_SHADOWMAP_BLUR], 0, 0);

                vkCmdDispatch(vulkanComputeCommandBuffers[render::COMPUTECMDBUFFER_BLUR_HORIZONTAL], shadowmapSize, shadowmapSize / 128, 1);

                vkCmdWriteTimestamp(vulkanComputeCommandBuffers[render::COMPUTECMDBUFFER_BLUR_HORIZONTAL], VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, vulkanTimestampQuery, TIMESTAMP::TIMESTAMP_BLUR_HORIZONTAL_END);

                vkEndCommandBuffer(vulkanComputeCommandBuffers[render::COMPUTECMDBUFFER_BLUR_HORIZONTAL]);

                VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

                // Submit compute commands
                VkSubmitInfo submitInfo{};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = &vulkanComputeCommandBuffers[render::COMPUTECMDBUFFER_BLUR_HORIZONTAL];
                submitInfo.waitSemaphoreCount = 1;
                submitInfo.pWaitSemaphores = &vulkanSemaphores[render::SEMAPHORE_SHADOWMAP_BLUR_VERTICAL];
                submitInfo.pWaitDstStageMask = &waitStageMask;
                submitInfo.signalSemaphoreCount = 1;
                submitInfo.pSignalSemaphores = &vulkanSemaphores[render::SEMAPHORE_SHADOWMAP_BLUR_HORIZONTAL];
                if (vkQueueSubmit(vulkanComputeQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
                {
                    std::cout << "failed to submit queue" << std::endl;
                    return -1;
                }
            }
        }

        //AOpass
        {
            VkCommandBufferBeginInfo commandBufferBeginInfo{};
            commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

            std::array<VkClearValue, 1> clearValues;
            clearValues[0].color = { { 1.0f, 0.0f, 0.0f, 1.0f } };

            VkRenderPassBeginInfo renderPassBeginInfo{};
            renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassBeginInfo.renderArea.offset.x = 0;
            renderPassBeginInfo.renderArea.offset.y = 0;
            renderPassBeginInfo.renderArea.extent.width = windowPtr->windowWidth;
            renderPassBeginInfo.renderArea.extent.height = windowPtr->windowHeight;
            renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());;
            renderPassBeginInfo.pClearValues = clearValues.data();
            renderPassBeginInfo.renderPass = vulkanRenderpasses[render::RENDERPASS_AO];
            renderPassBeginInfo.framebuffer = vulkanFramebuffers[render::RENDERPASS_AO];

            if (vkBeginCommandBuffer(vulkanCommandBuffers[render::COMMANDBUFFER_AO], &commandBufferBeginInfo) != VK_SUCCESS)
            {
                std::cout << "failed to end commnad buffer!" << std::endl;
                return -1;
            }

            // Reset timestamp query pool
            //vkCmdResetQueryPool(vulkanCommandBuffers[COMMANDBUFFER_AO], vulkanTimestampQuery, TIMESTAMP::TIMESTAMP_LIGHTING_START, 2);
            //vkCmdWriteTimestamp(vulkanCommandBuffers[COMMANDBUFFER_AO], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, vulkanTimestampQuery, TIMESTAMP::TIMESTAMP_LIGHTING_START);
            //
            //vkCmdResetQueryPool(vulkanCommandBuffers[COMMANDBUFFER_AO], vulkanPipelineStatisticsQuery, 0, pipelinestatisticsQueryCount);
            //vkCmdBeginQuery(vulkanCommandBuffers[COMMANDBUFFER_AO], vulkanPipelineStatisticsQuery, 0, 0);

            vkCmdBeginRenderPass(vulkanCommandBuffers[render::COMMANDBUFFER_AO], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(vulkanCommandBuffers[render::COMMANDBUFFER_AO], VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipelines[render::PIPELINE_AO]);

            //draw AO
            {
                vkCmdBindDescriptorSets(vulkanCommandBuffers[render::COMMANDBUFFER_AO], VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipelineLayouts[render::PIPELINE_AO], 0, 1,
                    &vulkanDescriptorSets[render::DESCRIPTOR_AO], 0, 0);

                render::draw(vulkanCommandBuffers[render::COMMANDBUFFER_AO], vertexbuffers[VERTEX_INDEX_QUAD_POSONLY]);
            }

            vkCmdEndRenderPass(vulkanCommandBuffers[render::COMMANDBUFFER_AO]);

            //vkCmdEndQuery(vulkanCommandBuffers[render::COMMANDBUFFER_AO], vulkanPipelineStatisticsQuery, 0);
            //vkCmdWriteTimestamp(vulkanCommandBuffers[render::COMMANDBUFFER_AO], VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, vulkanTimestampQuery, TIMESTAMP::TIMESTAMP_LIGHTING_END);

            if (vkEndCommandBuffer(vulkanCommandBuffers[render::COMMANDBUFFER_AO]) != VK_SUCCESS)
            {
                std::cout << "failed to end commnad buffer!" << std::endl;
                return -1;
            }

            std::array<VkCommandBuffer, 1> submitcommandbuffers = { vulkanCommandBuffers[render::COMMANDBUFFER_AO] };

            std::array<VkSemaphore, 1> waitsemaphores = { vulkanSemaphores[render::SEMAPHORE_SHADOWMAP_BLUR_HORIZONTAL] };

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            VkPipelineStageFlags pipelinestageFlag = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            std::array<VkPipelineStageFlags, 1> pipelinestageFlags = { pipelinestageFlag };
            submitInfo.pWaitDstStageMask = pipelinestageFlags.data();
            submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitsemaphores.size());
            submitInfo.pWaitSemaphores = waitsemaphores.data();
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = &vulkanSemaphores[render::SEMAPHORE_AO];
            submitInfo.commandBufferCount = static_cast<uint32_t>(submitcommandbuffers.size());
            submitInfo.pCommandBuffers = submitcommandbuffers.data();
            if (vkQueueSubmit(vulkanGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
            {
                std::cout << "failed to submit queue" << std::endl;
                return -1;
            }

            //blur
            {
                descriptor::write_descriptorset(devicePtr->vulkanDevice, vulkanDescriptorSets[render::DESCRIPTOR_AO_BLUR], {
                    {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0, {}, {vulkanSamplers[SAMPLE_INDEX_NORMAL], imagebuffers[IMAGE_INDEX_AO]->imageView, VK_IMAGE_LAYOUT_GENERAL}},
                    {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, {}, {vulkanSamplers[SAMPLE_INDEX_NORMAL], imagebuffers[IMAGE_INDEX_AO_BLUR]->imageView, VK_IMAGE_LAYOUT_GENERAL}},
                    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, {}, {vulkanSamplers[SAMPLE_INDEX_NORMAL], imagebuffers[IMAGE_INDEX_GBUFFER_POS]->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}},
                    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, {}, {vulkanSamplers[SAMPLE_INDEX_NORMAL], imagebuffers[IMAGE_INDEX_GBUFFER_NORM]->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}},
                    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4, {uniformbuffers[UNIFORM_INDEX_CAMERA].buf, 0, uniformbuffers[UNIFORM_INDEX_CAMERA].range}, {}},
                    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 5, {uniformbuffers[UNIFORM_INDEX_GAUSSIANWEIGHT].buf, 0, uniformbuffers[UNIFORM_INDEX_GAUSSIANWEIGHT].range}, {}},
                });

                VkCommandBufferBeginInfo commandBufferBeginInfo{};
                commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

                if (vkBeginCommandBuffer(vulkanComputeCommandBuffers[render::COMPUTECMDBUFFER_AO_BLUR_VERTICAL], &commandBufferBeginInfo) != VK_SUCCESS)
                {
                    std::cout << "failed to begin command buffer!" << std::endl;
                    return -1;
                }

                vkCmdBindPipeline(vulkanComputeCommandBuffers[render::COMPUTECMDBUFFER_AO_BLUR_VERTICAL], VK_PIPELINE_BIND_POINT_COMPUTE, vulkanPipelines[render::PIPELINE_AO_BLUR_VERTICAL]);
                vkCmdBindDescriptorSets(vulkanComputeCommandBuffers[render::COMPUTECMDBUFFER_AO_BLUR_VERTICAL], VK_PIPELINE_BIND_POINT_COMPUTE, vulkanPipelineLayouts[render::PIPELINE_AO_BLUR_VERTICAL], 0, 1,
                    &vulkanDescriptorSets[render::DESCRIPTOR_AO_BLUR], 0, 0);

                vkCmdDispatch(vulkanComputeCommandBuffers[render::COMPUTECMDBUFFER_AO_BLUR_VERTICAL], windowPtr->windowWidth, windowPtr->windowHeight, 1);

                vkEndCommandBuffer(vulkanComputeCommandBuffers[render::COMPUTECMDBUFFER_AO_BLUR_VERTICAL]);

                VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

                // Submit compute commands
                VkSubmitInfo submitInfo{};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = &vulkanComputeCommandBuffers[render::COMPUTECMDBUFFER_AO_BLUR_VERTICAL];
                submitInfo.waitSemaphoreCount = 1;
                submitInfo.pWaitSemaphores = &vulkanSemaphores[render::SEMAPHORE_AO];
                submitInfo.pWaitDstStageMask = &waitStageMask;
                submitInfo.signalSemaphoreCount = 1;
                submitInfo.pSignalSemaphores = &vulkanSemaphores[render::COMPUTECMDBUFFER_AO_BLUR_VERTICAL];
                if (vkQueueSubmit(vulkanComputeQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
                {
                    std::cout << "failed to submit queue" << std::endl;
                    return -1;
                }

                vkQueueWaitIdle(vulkanComputeQueue);

                descriptor::write_descriptorset(devicePtr->vulkanDevice, vulkanDescriptorSets[render::DESCRIPTOR_AO_BLUR], {
                    {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0, {}, {vulkanSamplers[SAMPLE_INDEX_NORMAL], imagebuffers[IMAGE_INDEX_AO_BLUR]->imageView, VK_IMAGE_LAYOUT_GENERAL}},
                    {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, {}, {vulkanSamplers[SAMPLE_INDEX_NORMAL], imagebuffers[IMAGE_INDEX_AO_BLUR]->imageView, VK_IMAGE_LAYOUT_GENERAL}},
                    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, {}, {vulkanSamplers[SAMPLE_INDEX_NORMAL], imagebuffers[IMAGE_INDEX_GBUFFER_POS]->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}},
                    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, {}, {vulkanSamplers[SAMPLE_INDEX_NORMAL], imagebuffers[IMAGE_INDEX_GBUFFER_NORM]->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}},
                    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4, {uniformbuffers[UNIFORM_INDEX_CAMERA].buf, 0, uniformbuffers[UNIFORM_INDEX_CAMERA].range}, {}},
                    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 5, {uniformbuffers[UNIFORM_INDEX_GAUSSIANWEIGHT].buf, 0, uniformbuffers[UNIFORM_INDEX_GAUSSIANWEIGHT].range}, {}},
                });

                if (vkBeginCommandBuffer(vulkanComputeCommandBuffers[render::COMPUTECMDBUFFER_AO_BLUR_HORIZONTAL], &commandBufferBeginInfo) != VK_SUCCESS)
                {
                    std::cout << "failed to begin command buffer!" << std::endl;
                    return -1;
                }

                vkCmdBindPipeline(vulkanComputeCommandBuffers[render::COMPUTECMDBUFFER_AO_BLUR_HORIZONTAL], VK_PIPELINE_BIND_POINT_COMPUTE, vulkanPipelines[render::PIPELINE_AO_BLUR_HORIZONTAL]);
                vkCmdBindDescriptorSets(vulkanComputeCommandBuffers[render::COMPUTECMDBUFFER_AO_BLUR_HORIZONTAL], VK_PIPELINE_BIND_POINT_COMPUTE, vulkanPipelineLayouts[render::PIPELINE_AO_BLUR_HORIZONTAL], 0, 1,
                    &vulkanDescriptorSets[render::DESCRIPTOR_AO_BLUR], 0, 0);

                vkCmdDispatch(vulkanComputeCommandBuffers[render::COMPUTECMDBUFFER_AO_BLUR_HORIZONTAL], windowPtr->windowWidth, windowPtr->windowHeight, 1);

                vkEndCommandBuffer(vulkanComputeCommandBuffers[render::COMPUTECMDBUFFER_AO_BLUR_HORIZONTAL]);

                // Submit compute commands
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = &vulkanComputeCommandBuffers[render::COMPUTECMDBUFFER_AO_BLUR_HORIZONTAL];
                submitInfo.waitSemaphoreCount = 1;
                submitInfo.pWaitSemaphores = &vulkanSemaphores[render::COMPUTECMDBUFFER_AO_BLUR_VERTICAL];
                submitInfo.pWaitDstStageMask = &waitStageMask;
                submitInfo.signalSemaphoreCount = 1;
                submitInfo.pSignalSemaphores = &vulkanSemaphores[render::COMPUTECMDBUFFER_AO_BLUR_HORIZONTAL];
                if (vkQueueSubmit(vulkanComputeQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
                {
                    std::cout << "failed to submit queue" << std::endl;
                    return -1;
                }
            }
        }


        //lightingpass
        {
            VkCommandBufferBeginInfo commandBufferBeginInfo{};
            commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

            std::array<VkClearValue, 2> clearValues;
            clearValues[0].color = { { 0.25f, 0.25f, 0.25f, 1.0f } };
            clearValues[1].depthStencil = { 1.0f, 0 };

            VkRenderPassBeginInfo renderPassBeginInfo{};
            renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassBeginInfo.renderArea.offset.x = 0;
            renderPassBeginInfo.renderArea.offset.y = 0;
            renderPassBeginInfo.renderArea.extent.width = windowPtr->windowWidth;
            renderPassBeginInfo.renderArea.extent.height = windowPtr->windowHeight;
            renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());;
            renderPassBeginInfo.pClearValues = clearValues.data();
            renderPassBeginInfo.renderPass = vulkanRenderpasses[render::RENDERPASS_SWAPCHAIN];
            renderPassBeginInfo.framebuffer = vulkanFramebuffers[framebufferindex];

            if (vkBeginCommandBuffer(vulkanCommandBuffers[commandbufferindex], &commandBufferBeginInfo) != VK_SUCCESS)
            {
                std::cout << "failed to end commnad buffer!" << std::endl;
                return -1;
            }

            // Reset timestamp query pool
            vkCmdResetQueryPool(vulkanCommandBuffers[commandbufferindex], vulkanTimestampQuery, TIMESTAMP::TIMESTAMP_LIGHTING_START, 2);
            vkCmdWriteTimestamp(vulkanCommandBuffers[commandbufferindex], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, vulkanTimestampQuery, TIMESTAMP::TIMESTAMP_LIGHTING_START);

            vkCmdResetQueryPool(vulkanCommandBuffers[commandbufferindex], vulkanPipelineStatisticsQuery, 0, pipelinestatisticsQueryCount);
            vkCmdBeginQuery(vulkanCommandBuffers[commandbufferindex], vulkanPipelineStatisticsQuery, 0, 0);
            
            vkCmdBeginRenderPass(vulkanCommandBuffers[commandbufferindex], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(vulkanCommandBuffers[commandbufferindex], VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipelines[render::PIPELINE_LIHGT]);

            //sun light pass
            {
                std::array<uint32_t, 1> dynamicoffsets = { 0 };

                vkCmdBindDescriptorSets(vulkanCommandBuffers[commandbufferindex], VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipelineLayouts[render::PIPELINE_LIHGT], 0, 1, 
                    &vulkanDescriptorSets[render::DESCRIPTOR_LIGHT], static_cast<uint32_t>(dynamicoffsets.size()), dynamicoffsets.data());

                render::draw(vulkanCommandBuffers[commandbufferindex], vertexbuffers[VERTEX_INDEX_FULLSCREENQUAD]);
            }


            if (!nolocallight)
            {
                vkCmdBindPipeline(vulkanCommandBuffers[commandbufferindex], VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipelines[render::PIPELINE_LOCAL_LIGHT]);

                //local light pass
                {
                    std::array<uint32_t, 1> dynamicoffsets = { 0 };
                    for (auto lit : local_light)
                    {
                        dynamicoffsets[0] += 64;
                        vkCmdBindDescriptorSets(vulkanCommandBuffers[commandbufferindex], VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipelineLayouts[render::PIPELINE_LOCAL_LIGHT], 0, 1, 
                            &vulkanDescriptorSets[render::DESCRIPTOR_LOCAL_LIGHT], static_cast<uint32_t>(dynamicoffsets.size()), dynamicoffsets.data());

                        render::draw(vulkanCommandBuffers[commandbufferindex], vertexbuffers[VERTEX_INDEX_SPHERE_POSONLY]);
                    }
                }

                vkCmdBindPipeline(vulkanCommandBuffers[commandbufferindex], VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipelines[render::PIPELINE_DIFFUSE]);
                //draw local_light
                {
                    std::array<uint32_t, 1> dynamicoffsets = { 128 * static_cast<uint32_t>(objects.size()) };
                    for (auto lit : local_light)
                    {
                        vkCmdBindDescriptorSets(vulkanCommandBuffers[commandbufferindex], VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipelineLayouts[render::PIPELINE_DIFFUSE], 0, 1, 
                            &vulkanDescriptorSets[render::DESCRIPTOR_DIFFUSE], static_cast<uint32_t>(dynamicoffsets.size()), dynamicoffsets.data());

                        render::draw(vulkanCommandBuffers[commandbufferindex], vertexbuffers[VERTEX_INDEX_BOX_POSONLY]);

                        dynamicoffsets[0] += 128;
                    }
                }
            }

            {
                vkCmdBindPipeline(vulkanCommandBuffers[commandbufferindex], VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipelines[render::PIPELINE_SKYDOME]);
                std::array<uint32_t, 1> dynamicoffsets = { 0 };

                vkCmdBindDescriptorSets(vulkanCommandBuffers[commandbufferindex], VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipelineLayouts[render::PIPELINE_SKYDOME], 0, 1,
                    &vulkanDescriptorSets[render::DESCRIPTOR_SKYDOME], 0, 0);

                render::draw(vulkanCommandBuffers[commandbufferindex], vertexbuffers[VERTEX_INDEX_SPHERE_POSONLY]);
            }

            guiPtr->render(vulkanCommandBuffers[commandbufferindex]);

            vkCmdEndRenderPass(vulkanCommandBuffers[commandbufferindex]);

            vkCmdEndQuery(vulkanCommandBuffers[commandbufferindex], vulkanPipelineStatisticsQuery, 0);
            vkCmdWriteTimestamp(vulkanCommandBuffers[commandbufferindex], VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, vulkanTimestampQuery, TIMESTAMP::TIMESTAMP_LIGHTING_END);

            if (vkEndCommandBuffer(vulkanCommandBuffers[commandbufferindex]) != VK_SUCCESS)
            {
                std::cout << "failed to end commnad buffer!" << std::endl;
                return -1;
            }

            std::array<VkCommandBuffer, 1> submitcommandbuffers = { vulkanCommandBuffers[commandbufferindex] };

            std::array<VkSemaphore, 1> waitsemaphores = { vulkanSemaphores[render::COMPUTECMDBUFFER_AO_BLUR_HORIZONTAL] };

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            VkPipelineStageFlags pipelinestageFlag = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            std::array<VkPipelineStageFlags, 1> pipelinestageFlags = { pipelinestageFlag };
            submitInfo.pWaitDstStageMask = pipelinestageFlags.data();
            submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitsemaphores.size());
            submitInfo.pWaitSemaphores = waitsemaphores.data();
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = &vulkanSemaphores[render::SEMAPHORE_RENDER];
            submitInfo.commandBufferCount = static_cast<uint32_t>(submitcommandbuffers.size());
            submitInfo.pCommandBuffers = submitcommandbuffers.data();
            if (vkQueueSubmit(vulkanGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
            {
                std::cout << "failed to submit queue" << std::endl;
                return -1;
            }
        }

        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.pNext = VK_NULL_HANDLE;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &vulkanSwapchain;
        presentInfo.pImageIndices = &imageIndex;

        if (vulkanSemaphores[render::SEMAPHORE_RENDER] != VK_NULL_HANDLE)
        {
            presentInfo.pWaitSemaphores = &vulkanSemaphores[render::SEMAPHORE_RENDER];
            presentInfo.waitSemaphoreCount = 1;
        }

        vkQueuePresentKHR(vulkanGraphicsQueue, &presentInfo);

        if (vkQueueWaitIdle(vulkanGraphicsQueue) != VK_SUCCESS)
        {
            std::cout << "cannot wait queue" << std::endl;
            return -1;
        }

        std::array<uint64_t, pipelinestatisticsQueryCount + 1> queryresult;
        vkGetQueryPoolResults(devicePtr->vulkanDevice, vulkanPipelineStatisticsQuery, 0, 1, (pipelinestatisticsQueryCount + 1) * sizeof(uint64_t), queryresult.data(), sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);

        std::array<uint64_t, timestampQueryCount + 1> timequeryresult;
        vkGetQueryPoolResults(devicePtr->vulkanDevice, vulkanTimestampQuery, 0, timestampQueryCount, (timestampQueryCount + 1) * sizeof(uint64_t), timequeryresult.data(), sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);

        for (int i = 0; i < timestampQueryCount / 2; ++i)
        {
            pipeline_timestamps[i] = (timequeryresult[i * 2 + 1] - timequeryresult[i * 2]) / 1000000.0f;
        }
    }

    close();

	return 1;
}