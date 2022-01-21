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

Image depthImage;

std::vector<Image> swapchainImages;
std::vector<VkFence> vulkanWaitFences;
std::vector<VkFramebuffer> vulkanFramebuffers;

VkQueue vulkanGraphicsQueue = VK_NULL_HANDLE;
VkQueue vulkanComputeQueue = VK_NULL_HANDLE;
VkSemaphore vulkanPresentSemaphore = VK_NULL_HANDLE;
VkSemaphore vulkanRenderSemaphore = VK_NULL_HANDLE;
VkPipelineCache vulkanPipelineCache = VK_NULL_HANDLE;

VkSampler vulkanSampler = VK_NULL_HANDLE;

VkFormat vulkanColorFormat;
VkFormat vulkanDepthFormat;
VkColorSpaceKHR vulkanColorSpace;

uint32_t swapchainImageCount;

uint64_t gbufferpasstime;
uint64_t lightingpasstime;

const uint32_t timestampQueryCount = 4;
const uint32_t pipelinestatisticsQueryCount = 9;

window* windowPtr = new window();
device* devicePtr = new device();
memory* memPtr = new memory();
gui* guiPtr = new gui();

std::vector<VkCommandBuffer> vulkanCommandBuffers;
VkPipeline vulkanPipeline = VK_NULL_HANDLE;
VkRenderPass vulkanRenderPass = VK_NULL_HANDLE;
VkDescriptorSetLayout vulkanDescriptorSetLayout = VK_NULL_HANDLE;
VkDescriptorPool vulkanDescriptorPool = VK_NULL_HANDLE;
VkDescriptorSet vulkanDescriptorSet = VK_NULL_HANDLE;
VkPipelineLayout vulkanPipelineLayout = VK_NULL_HANDLE;

Buffer objvertexBuffer{};
Buffer objindexBuffer{};

Buffer objvertexBuffer2{};
Buffer objindexBuffer2{};

Buffer fsqvertexBuffer{};
Buffer fsqindexBuffer{};

Buffer uniformbuffer0{};
Buffer uniformbuffer1{};
Buffer uniformbuffer2{};
Buffer uniformbuffer3{};
Buffer uniformbuffer4{};

Image posframebufferimage;
Image normframebufferimage;
Image texframebufferimage;
Image albedoframebufferimage;

VkPipeline gbufferPipeline = VK_NULL_HANDLE;
VkRenderPass gbufferRenderPass = VK_NULL_HANDLE;
VkFramebuffer gbufferFramebuffer = VK_NULL_HANDLE;
VkSemaphore gbuffersemaphore = VK_NULL_HANDLE;
VkCommandBuffer gbufferCommandbuffer = VK_NULL_HANDLE;
VkDescriptorSetLayout gbufferDescriptorSetLayout = VK_NULL_HANDLE;
VkDescriptorSet gbufferDescriptorSet = VK_NULL_HANDLE;
VkPipelineLayout gbufferPipelineLayout = VK_NULL_HANDLE;

std::vector<object*> objects;

glm::vec3 camerapos = glm::vec3(0.0f, 2.0f, 5.0f);
glm::quat rotation = glm::quat(glm::vec3(glm::radians(0.0f), 0, 0));

lightdata lights;

const glm::vec3 RIGHT = glm::vec3(1.0f, 0.0f, 0.0f);
const glm::vec3 FORWARD = glm::vec3(0.0f, 0.0f, -1.0f);

lightSetting lightsetting;
ObjectProperties objproperties;

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

    memPtr->create_depth_image(devicePtr->vulkanDevice, vulkanDepthFormat, windowPtr->windowWidth, windowPtr->windowHeight, depthImage);

    VkSamplerCreateInfo samplerCreateInfo{};
    samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCreateInfo.maxAnisotropy = 1.0f;    
    samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
    samplerCreateInfo.minFilter = VK_FILTER_NEAREST;
    samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCreateInfo.mipLodBias = 0.0f;
    samplerCreateInfo.maxAnisotropy = 1.0f;
    samplerCreateInfo.minLod = 0.0f;
    samplerCreateInfo.maxLod = 1.0f;
    samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    
    if (vkCreateSampler(devicePtr->vulkanDevice, &samplerCreateInfo, VK_NULL_HANDLE, &vulkanSampler))
    {
        std::cout << "failed to create sampler!" << std::endl;
    }
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

    vkCreateSemaphore(devicePtr->vulkanDevice, &createinfo, VK_NULL_HANDLE, &vulkanRenderSemaphore);
    vkCreateSemaphore(devicePtr->vulkanDevice, &createinfo, VK_NULL_HANDLE, &vulkanPresentSemaphore);
    vkCreateSemaphore(devicePtr->vulkanDevice, &createinfo, VK_NULL_HANDLE, &gbuffersemaphore);

    VkFenceCreateInfo fenceCreateInfo{};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    vulkanWaitFences.resize(vulkanCommandBuffers.size());
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
    renderpass::create_renderpass(devicePtr->vulkanDevice, vulkanRenderPass, {
    {vulkanColorFormat, 0, true, false},
    {vulkanDepthFormat, 1, false, true},
        });

    vulkanFramebuffers.resize(swapchainImageCount);
    for (uint32_t i = 0; i < swapchainImageCount; i++)
    {
        renderpass::create_framebuffer(devicePtr->vulkanDevice, vulkanRenderPass, vulkanFramebuffers[i], windowPtr->windowWidth, windowPtr->windowHeight, {
        swapchainImages[i].imageView, depthImage.imageView
            });
    }

    //gbuffer pass
    renderpass::create_renderpass(devicePtr->vulkanDevice, gbufferRenderPass, {
        {VK_FORMAT_R16G16B16A16_SFLOAT, 0, false, false},
        {VK_FORMAT_R16G16B16A16_SFLOAT, 1, false, false},
        {VK_FORMAT_R16G16B16A16_SFLOAT, 2, false, false},
        {VK_FORMAT_R16G16B16A16_SFLOAT, 3, false, false},
        {vulkanDepthFormat, 4, false, true},
        });

    renderpass::create_framebuffer(devicePtr->vulkanDevice, gbufferRenderPass, gbufferFramebuffer, windowPtr->windowWidth, windowPtr->windowHeight, {
        posframebufferimage.imageView, normframebufferimage.imageView, texframebufferimage.imageView, albedoframebufferimage.imageView, depthImage.imageView
        });
}

void createdescriptorset()
{
    descriptor::create_descriptorpool(devicePtr->vulkanDevice, vulkanDescriptorPool, {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 },
        });

    descriptor::create_descriptorset_layout(devicePtr->vulkanDevice, gbufferDescriptorSetLayout, gbufferDescriptorSet, vulkanDescriptorPool, {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT, 1},
        });

    descriptor::write_descriptorset(devicePtr->vulkanDevice, gbufferDescriptorSet, {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, {uniformbuffer0.buf, 0, uniformbuffer0.range}, {}},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, {uniformbuffer1.buf, 0, uniformbuffer1.range}, {}},
        });

    descriptor::create_descriptorset_layout(devicePtr->vulkanDevice, vulkanDescriptorSetLayout, vulkanDescriptorSet, vulkanDescriptorPool, {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 2},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 3},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 4},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 5},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 6},
    });

    descriptor::write_descriptorset(devicePtr->vulkanDevice, vulkanDescriptorSet, {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, {}, {vulkanSampler, posframebufferimage.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, {}, {vulkanSampler, normframebufferimage.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, {}, {vulkanSampler, texframebufferimage.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3, {}, {vulkanSampler, albedoframebufferimage.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4, {uniformbuffer2.buf, 0, uniformbuffer2.range}, {}},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 5, {uniformbuffer3.buf, 0, uniformbuffer3.range}, {}},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 6, {uniformbuffer4.buf, 0, uniformbuffer4.range}, {}},
        });
}

void createPipeline()
{
    {
        pipeline::create_pipelinelayout(devicePtr->vulkanDevice, vulkanDescriptorSetLayout, vulkanPipelineLayout);

        VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.layout = vulkanPipelineLayout;
        pipelineCreateInfo.renderPass = vulkanRenderPass;

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
        pipeline::create_pipieline(devicePtr->vulkanDevice, pipelineCreateInfo, vulkanPipeline, vulkanPipelineCache, {
                {"data/shaders/lighting.vert.spv", VK_SHADER_STAGE_VERTEX_BIT},
                {"data/shaders/lighting.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT},
            });
    }

    {
        pipeline::create_pipelinelayout(devicePtr->vulkanDevice, gbufferDescriptorSetLayout, gbufferPipelineLayout);

        VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
        pipelineCreateInfo.layout = gbufferPipelineLayout;
        pipelineCreateInfo.renderPass = gbufferRenderPass;

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
        pipeline::create_pipieline(devicePtr->vulkanDevice, pipelineCreateInfo, gbufferPipeline, vulkanPipelineCache, {
                {"data/shaders/gbuffer.vert.spv", VK_SHADER_STAGE_VERTEX_BIT},
                {"data/shaders/gbuffer.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT},
            });
    }
}

void createcommandbuffer()
{
    vulkanCommandBuffers.resize(swapchainImageCount);

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = devicePtr->commandbuffer_allocateinfo(static_cast<uint32_t>(vulkanCommandBuffers.size()));

    if (vkAllocateCommandBuffers(devicePtr->vulkanDevice, &commandBufferAllocateInfo, vulkanCommandBuffers.data()) != VK_SUCCESS)
    {
        std::cout << "failed to allocate commandbuffers!" << std::endl;
    }

    commandBufferAllocateInfo = devicePtr->commandbuffer_allocateinfo(static_cast<uint32_t>(1));
    if (vkAllocateCommandBuffers(devicePtr->vulkanDevice, &commandBufferAllocateInfo, &gbufferCommandbuffer) != VK_SUCCESS)
    {
        std::cout << "failed to allocate commandbuffers!" << std::endl;
    }
}

void updatebuffer()
{
    float fov = static_cast<float>(windowPtr->windowWidth) / static_cast<float>(windowPtr->windowHeight);

    Projection proj;

    proj.projMat = glm::perspective(glm::radians(45.0f), fov, 0.1f, 100.0f);
    //force it to right handed
    proj.projMat[1][1] *= -1.0f;

    proj.viewMat = glm::toMat4(glm::conjugate(rotation)) * glm::translate(glm::mat4(1.0f), -camerapos);

    glm::vec3 lightpos = glm::vec3(0.0f, 10.0f, 0.0f);

    void* data0;
    vkMapMemory(devicePtr->vulkanDevice, uniformbuffer0.memory, 0, uniformbuffer0.size, 0, &data0);
    memcpy(data0, &proj, uniformbuffer0.size);
    vkUnmapMemory(devicePtr->vulkanDevice, uniformbuffer0.memory);

    static float a = 0.0f;
    a += 0.001f;
    objproperties.modelMat = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)) * glm::rotate(glm::mat4(1.0f), a, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(10.0f));

    ObjectProperties props[2];
    props[0].albedoColor = objproperties.albedoColor;
    props[0].metallic = objproperties.metallic;
    props[0].roughness = objproperties.roughness;
    props[0].modelMat = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)) * glm::rotate(glm::mat4(1.0f), a, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(10.0f));

    props[1].albedoColor = objproperties.albedoColor;
    props[1].metallic = objproperties.metallic;
    props[1].roughness = objproperties.roughness;
    props[1].modelMat = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 2.0f, 0.0f)) * glm::rotate(glm::mat4(1.0f), -a, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.01f));

    void* data1;
    vkMapMemory(devicePtr->vulkanDevice, uniformbuffer1.memory, 0, uniformbuffer1.range, 0, &data1);
    memcpy(data1, props, uniformbuffer1.range);
    vkUnmapMemory(devicePtr->vulkanDevice, uniformbuffer1.memory);
    vkMapMemory(devicePtr->vulkanDevice, uniformbuffer1.memory, 128, uniformbuffer1.range, 0, &data1);
    memcpy(data1, &props[1], uniformbuffer1.range);
    vkUnmapMemory(devicePtr->vulkanDevice, uniformbuffer1.memory);

    void* data2;
    vkMapMemory(devicePtr->vulkanDevice, uniformbuffer2.memory, 0, uniformbuffer2.size, 0, &data2);
    memcpy(data2, &lightsetting, uniformbuffer2.size);
    vkUnmapMemory(devicePtr->vulkanDevice, uniformbuffer2.memory);

    camera cam;
    cam.position = camerapos;

    void* data3;
    vkMapMemory(devicePtr->vulkanDevice, uniformbuffer3.memory, 0, uniformbuffer3.size, 0, &data3);
    memcpy(data3, &cam, uniformbuffer3.size);
    vkUnmapMemory(devicePtr->vulkanDevice, uniformbuffer3.memory);

    void* data4;
    vkMapMemory(devicePtr->vulkanDevice, uniformbuffer4.memory, 0, uniformbuffer4.size, 0, &data4);
    memcpy(data4, &lights, uniformbuffer4.size);
    vkUnmapMemory(devicePtr->vulkanDevice, uniformbuffer4.memory);
}

void setupbuffer()
{
    lights.lights[0].position = glm::vec3(0, 5.0, -2.5);
    lights.lights[1].position = glm::vec3(-2.5, 5.0, 0);
    lights.lightnum = 2;

    memPtr->create_fb_image(devicePtr->vulkanDevice, VK_FORMAT_R16G16B16A16_SFLOAT, windowPtr->windowWidth, windowPtr->windowHeight, posframebufferimage);
    memPtr->create_fb_image(devicePtr->vulkanDevice, VK_FORMAT_R16G16B16A16_SFLOAT, windowPtr->windowWidth, windowPtr->windowHeight, normframebufferimage);
    memPtr->create_fb_image(devicePtr->vulkanDevice, VK_FORMAT_R16G16B16A16_SFLOAT, windowPtr->windowWidth, windowPtr->windowHeight, texframebufferimage);
    memPtr->create_fb_image(devicePtr->vulkanDevice, VK_FORMAT_R16G16B16A16_SFLOAT, windowPtr->windowWidth, windowPtr->windowHeight, albedoframebufferimage);

    //std::vector<float> vertices = {
    //    0.5f, 0.5f, 0.5f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,
    //    0.5f, 0.5f,-0.5f,   1.0f, 0.0f, 0.0f,   1.0f, 0.0f,
    //    0.5f,-0.5f, 0.5f,   1.0f, 0.0f, 0.0f,   0.0f, 1.0f,
    //    0.5f,-0.5f,-0.5f,   1.0f, 0.0f, 0.0f,   0.0f, 0.0f,

    //   -0.5f, 0.5f,-0.5f,  -1.0f, 0.0f, 0.0f,   1.0f, 0.0f,
    //   -0.5f, 0.5f, 0.5f,  -1.0f, 0.0f, 0.0f,   1.0f, 1.0f,
    //   -0.5f,-0.5f,-0.5f,  -1.0f, 0.0f, 0.0f,   0.0f, 0.0f,
    //   -0.5f,-0.5f, 0.5f,  -1.0f, 0.0f, 0.0f,   0.0f, 1.0f,

    //    0.5f, 0.5f, 0.5f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
    //   -0.5f, 0.5f, 0.5f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
    //    0.5f, 0.5f,-0.5f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
    //   -0.5f, 0.5f,-0.5f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,

    //   -0.5f,-0.5f, 0.5f,   0.0f,-1.0f, 0.0f,   0.0f, 1.0f,
    //    0.5f,-0.5f, 0.5f,   0.0f,-1.0f, 0.0f,   1.0f, 1.0f,
    //   -0.5f,-0.5f,-0.5f,   0.0f,-1.0f, 0.0f,   0.0f, 0.0f,
    //    0.5f,-0.5f,-0.5f,   0.0f,-1.0f, 0.0f,   1.0f, 0.0f,

    //    0.5f, 0.5f, 0.5f,   0.0f, 0.0f, 1.0f,   1.0f, 1.0f,
    //    0.5f,-0.5f, 0.5f,   0.0f, 0.0f, 1.0f,   1.0f, 0.0f,
    //   -0.5f, 0.5f, 0.5f,   0.0f, 0.0f, 1.0f,   0.0f, 1.0f,
    //   -0.5f,-0.5f, 0.5f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,

    //   -0.5f, 0.5f,-0.5f,   0.0f, 0.0f,-1.0f,   0.0f, 1.0f,
    //   -0.5f,-0.5f,-0.5f,   0.0f, 0.0f,-1.0f,   0.0f, 0.0f,
    //    0.5f, 0.5f,-0.5f,   0.0f, 0.0f,-1.0f,   1.0f, 1.0f,
    //    0.5f,-0.5f,-0.5f,   0.0f, 0.0f,-1.0f,   1.0f, 0.0f,
    //};

    //std::vector<uint32_t> indices = {
    //    0, 1, 2,
    //    2, 1, 3,

    //    4, 5, 6,
    //    6, 5, 7,

    //    8, 9,10,
    //   10, 9,11,

    //   12,13,14,
    //   14,13,15,

    //   16,17,18,
    //   18,17,19,

    //   20,21,22,
    //   22,21,23,
    //};

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

        memPtr->create_vertex_index_buffer(devicePtr->vulkanDevice, vulkanGraphicsQueue, devicePtr, vertices, indices, fsqvertexBuffer, fsqindexBuffer);
    }

    {
        std::vector<helper::vertindex> data = helper::readassimp("data/model/bunny.ply");

        memPtr->create_vertex_index_buffer(devicePtr->vulkanDevice, vulkanGraphicsQueue, devicePtr, data[0].vertices, data[0].indices, objvertexBuffer, objindexBuffer);

        object* newobject = new object();
        newobject->create_object(static_cast<unsigned int>(data[0].indices.size()), objvertexBuffer.buf, objindexBuffer.buf);
        objects.push_back(newobject);

        //data = helper::readassimp("data/model/armadillo.ply");
        //memPtr->create_vertex_index_buffer(devicePtr->vulkanDevice, vulkanGraphicsQueue, devicePtr, data[0].vertices, data[0].indices, objvertexBuffer2, objindexBuffer2);
        memPtr->create_vertex_index_buffer(devicePtr->vulkanDevice, vulkanGraphicsQueue, devicePtr, teapotVertices, teapotIndices, objvertexBuffer2, objindexBuffer2);
        newobject = new object();
        newobject->create_object(static_cast<unsigned int>(data[0].indices.size()), objvertexBuffer2.buf, objindexBuffer2.buf);
        objects.push_back(newobject);
    }

    memPtr->create_buffer(devicePtr->vulkanDevice, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sizeof(Projection), 1, uniformbuffer0);
    memPtr->create_buffer(devicePtr->vulkanDevice, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, /*sizeof(ObjectProperties)*/128, 2, uniformbuffer1);
    memPtr->create_buffer(devicePtr->vulkanDevice, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sizeof(lightSetting), 1, uniformbuffer2);
    memPtr->create_buffer(devicePtr->vulkanDevice, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sizeof(camera), 1, uniformbuffer3);
    memPtr->create_buffer(devicePtr->vulkanDevice, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sizeof(lightdata), 1, uniformbuffer4);

    updatebuffer();
}

void freebuffer()
{
    memPtr->free_buffer(devicePtr->vulkanDevice, objvertexBuffer);
    memPtr->free_buffer(devicePtr->vulkanDevice, objindexBuffer);
    memPtr->free_buffer(devicePtr->vulkanDevice, objvertexBuffer2);
    memPtr->free_buffer(devicePtr->vulkanDevice, objindexBuffer2);
    memPtr->free_buffer(devicePtr->vulkanDevice, fsqvertexBuffer);
    memPtr->free_buffer(devicePtr->vulkanDevice, fsqindexBuffer);
    memPtr->free_buffer(devicePtr->vulkanDevice, uniformbuffer0);
    memPtr->free_buffer(devicePtr->vulkanDevice, uniformbuffer1);
    memPtr->free_buffer(devicePtr->vulkanDevice, uniformbuffer2);
    memPtr->free_buffer(devicePtr->vulkanDevice, uniformbuffer3);
    memPtr->free_buffer(devicePtr->vulkanDevice, uniformbuffer4);

    memPtr->free_image(devicePtr->vulkanDevice, posframebufferimage);
    memPtr->free_image(devicePtr->vulkanDevice, normframebufferimage);
    memPtr->free_image(devicePtr->vulkanDevice, texframebufferimage);
    memPtr->free_image(devicePtr->vulkanDevice, albedoframebufferimage);

    memPtr->free_image(devicePtr->vulkanDevice, depthImage);
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

    if (windowPtr->pressed[GLFW_KEY_W])
    {
        camerapos += look * 5.0f * dt;
    }
    if (windowPtr->pressed[GLFW_KEY_S])
    {
        camerapos -= look * 5.0f * dt;
    }
    if (windowPtr->pressed[GLFW_KEY_A])
    {
        camerapos -= right * 5.0f * dt;
    }
    if (windowPtr->pressed[GLFW_KEY_D])
    {
        camerapos += right * 5.0f * dt;
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

    pipeline::closepipeline(devicePtr->vulkanDevice, vulkanPipeline);
    pipeline::closepipeline(devicePtr->vulkanDevice, gbufferPipeline);
    pipeline::close_pipelinelayout(devicePtr->vulkanDevice, vulkanPipelineLayout);
    pipeline::close_pipelinelayout(devicePtr->vulkanDevice, gbufferPipelineLayout);
    descriptor::close_descriptorset_layout(devicePtr->vulkanDevice, vulkanDescriptorSetLayout);
    descriptor::close_descriptorset_layout(devicePtr->vulkanDevice, gbufferDescriptorSetLayout);
    descriptor::close_descriptorpool(devicePtr->vulkanDevice, vulkanDescriptorPool);
    for (auto fb : vulkanFramebuffers)
    {
        renderpass::close_framebuffer(devicePtr->vulkanDevice, fb);
    }
    renderpass::close_framebuffer(devicePtr->vulkanDevice, gbufferFramebuffer);
    renderpass::close_renderpass(devicePtr->vulkanDevice, vulkanRenderPass);
    renderpass::close_renderpass(devicePtr->vulkanDevice, gbufferRenderPass);

    devicePtr->free_command_buffer(swapchainImageCount, vulkanCommandBuffers.data());
    devicePtr->free_command_buffer(1, &gbufferCommandbuffer);
    vkDestroySampler(devicePtr->vulkanDevice, vulkanSampler, VK_NULL_HANDLE);
    guiPtr->close(devicePtr->vulkanDevice);
    windowPtr->close_window();
    vkDestroyPipelineCache(devicePtr->vulkanDevice, vulkanPipelineCache, VK_NULL_HANDLE);
    vkDestroySemaphore(devicePtr->vulkanDevice, vulkanPresentSemaphore, VK_NULL_HANDLE);
    vkDestroySemaphore(devicePtr->vulkanDevice, vulkanRenderSemaphore, VK_NULL_HANDLE);
    vkDestroySemaphore(devicePtr->vulkanDevice, gbuffersemaphore, VK_NULL_HANDLE);
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

    guiPtr->init(vulkanInstance, vulkanGraphicsQueue, swapchainImageCount, windowPtr->glfwWindow, vulkanRenderPass, devicePtr);

    uint32_t imageIndex = 0;
    while (windowPtr->update_window_frame())
    {
        update(windowPtr->dt);
        updatebuffer();

        vkAcquireNextImageKHR(devicePtr->vulkanDevice, vulkanSwapchain, UINT64_MAX, vulkanPresentSemaphore, 0, &imageIndex);

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

                    ImGui::EndMenu();
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Menu")) 
            {
                if (ImGui::BeginMenu("Performance"))
                {
                    ImGui::Text("Gbuffer pass : %fms", gbufferpasstime / 1000000.0f);
                    ImGui::Text("Lighting pass : %fms", lightingpasstime / 1000000.0f);

                    ImGui::EndMenu();
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Setting"))
            {
                if (ImGui::BeginMenu("Deferred Target"))
                {
                    bool deferred[4] = { false };
                    deferred[lightsetting.outputTex] = true;

                    if (ImGui::MenuItem("Position##Target", "", deferred[outputMode::POSITION])) lightsetting.outputTex = outputMode::POSITION;
                    else if (ImGui::MenuItem("Normal##Target", "", deferred[outputMode::NORMAL])) lightsetting.outputTex = outputMode::NORMAL;
                    else if (ImGui::MenuItem("TextureCoordinate##Target", "", deferred[outputMode::TEXTURECOORDINATE])) lightsetting.outputTex = outputMode::TEXTURECOORDINATE;
                    else if (ImGui::MenuItem("Lighting##Target", "", deferred[outputMode::LIGHTING])) lightsetting.outputTex = outputMode::LIGHTING;

                    ImGui::EndMenu();
                }

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
            gbufferclearValues[1].color = { { 0.25f, 0.25f, 0.25f, 1.0f } };
            gbufferclearValues[2].color = { { 0.25f, 0.25f, 0.25f, 1.0f } };
            gbufferclearValues[3].color = { { 0.25f, 0.25f, 0.25f, 1.0f } };
            gbufferclearValues[4].depthStencil = { 1.0f, 0 };

            VkRenderPassBeginInfo renderPassBeginInfo{};
            renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassBeginInfo.renderPass = gbufferRenderPass;
            renderPassBeginInfo.renderArea.offset.x = 0;
            renderPassBeginInfo.renderArea.offset.y = 0;
            renderPassBeginInfo.renderArea.extent.width = windowPtr->windowWidth;
            renderPassBeginInfo.renderArea.extent.height = windowPtr->windowHeight;
            renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(gbufferclearValues.size());
            renderPassBeginInfo.pClearValues = gbufferclearValues.data();
            renderPassBeginInfo.framebuffer = gbufferFramebuffer;

            if (vkBeginCommandBuffer(gbufferCommandbuffer, &commandBufferBeginInfo) != VK_SUCCESS)
            {
                std::cout << "failed to begin command buffer!" << std::endl;
                return -1;
            }
            
            vkCmdResetQueryPool(gbufferCommandbuffer, vulkanTimestampQuery, 0, 2);

            vkCmdWriteTimestamp(gbufferCommandbuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, vulkanTimestampQuery, 0);

            vkCmdBeginRenderPass(gbufferCommandbuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(gbufferCommandbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gbufferPipeline);

            std::array<uint32_t, 1> offset;

            unsigned int i = 0;
            for (auto obj : objects)
            {
                offset = { i * 128 };
                vkCmdBindDescriptorSets(gbufferCommandbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gbufferPipelineLayout, 0, 1, &gbufferDescriptorSet, 1, offset.data());

                obj->draw_object(gbufferCommandbuffer);
                ++i;
            }

            vkCmdEndRenderPass(gbufferCommandbuffer);

            vkCmdWriteTimestamp(gbufferCommandbuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, vulkanTimestampQuery, 1);

            if (vkEndCommandBuffer(gbufferCommandbuffer) != VK_SUCCESS)
            {
                std::cout << "failed to end commnad buffer!" << std::endl;
                return -1;
            }

            std::array<VkCommandBuffer, 1> submitcommandbuffers = { gbufferCommandbuffer };

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            VkPipelineStageFlags pipelinestageFlag = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            submitInfo.pWaitDstStageMask = &pipelinestageFlag;
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = &vulkanPresentSemaphore;
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = &gbuffersemaphore;
            submitInfo.commandBufferCount = static_cast<uint32_t>(submitcommandbuffers.size());
            submitInfo.pCommandBuffers = submitcommandbuffers.data();
            if (vkQueueSubmit(vulkanGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
            {
                std::cout << "failed to submit queue" << std::endl;
                return -1;
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
            renderPassBeginInfo.renderPass = vulkanRenderPass;
            renderPassBeginInfo.framebuffer = vulkanFramebuffers[imageIndex];

            if (vkBeginCommandBuffer(vulkanCommandBuffers[imageIndex], &commandBufferBeginInfo) != VK_SUCCESS)
            {
                std::cout << "failed to end commnad buffer!" << std::endl;
                return -1;
            }

            // Reset timestamp query pool
            vkCmdResetQueryPool(vulkanCommandBuffers[imageIndex], vulkanTimestampQuery, 2, 2);
            vkCmdWriteTimestamp(vulkanCommandBuffers[imageIndex], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, vulkanTimestampQuery, 2);

            vkCmdResetQueryPool(vulkanCommandBuffers[imageIndex], vulkanPipelineStatisticsQuery, 0, pipelinestatisticsQueryCount);
            vkCmdBeginQuery(vulkanCommandBuffers[imageIndex], vulkanPipelineStatisticsQuery, 0, 0);
            
            vkCmdBeginRenderPass(vulkanCommandBuffers[imageIndex], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

            vkCmdBindPipeline(vulkanCommandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline);
            vkCmdBindDescriptorSets(vulkanCommandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipelineLayout, 0, 1, &vulkanDescriptorSet, 0, 0);

            VkDeviceSize offsets[1] = { 0 };

            vkCmdBindVertexBuffers(vulkanCommandBuffers[imageIndex], 0, 1, &fsqvertexBuffer.buf, offsets);
            vkCmdBindIndexBuffer(vulkanCommandBuffers[imageIndex], fsqindexBuffer.buf, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(vulkanCommandBuffers[imageIndex], 6, 1, 0, 0, 0);

            guiPtr->render(vulkanCommandBuffers[imageIndex]);

            vkCmdEndRenderPass(vulkanCommandBuffers[imageIndex]);

            vkCmdEndQuery(vulkanCommandBuffers[imageIndex], vulkanPipelineStatisticsQuery, 0);
            vkCmdWriteTimestamp(vulkanCommandBuffers[imageIndex], VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, vulkanTimestampQuery, 3);

            if (vkEndCommandBuffer(vulkanCommandBuffers[imageIndex]) != VK_SUCCESS)
            {
                std::cout << "failed to end commnad buffer!" << std::endl;
                return -1;
            }

            std::array<VkCommandBuffer, 1> submitcommandbuffers = { vulkanCommandBuffers[imageIndex] };

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            VkPipelineStageFlags pipelinestageFlag = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            submitInfo.pWaitDstStageMask = &pipelinestageFlag;
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = &gbuffersemaphore;
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = &vulkanRenderSemaphore;
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

        if (vulkanRenderSemaphore != VK_NULL_HANDLE)
        {
            presentInfo.pWaitSemaphores = &vulkanRenderSemaphore;
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

        gbufferpasstime = timequeryresult[1] - timequeryresult[0];
        lightingpasstime = timequeryresult[3] - timequeryresult[2];
    }

    close();

	return 1;
}