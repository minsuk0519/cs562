#include "device.hpp"
#include "debug.hpp"

#include <iostream>
#include <vector>

bool device::select_physical_device(VkInstance instance)
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0)
    {
        std::cout << "failed to find GPUs with Vulkan support!" << std::endl;
        return false;
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    if (vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()) != VK_SUCCESS)
    {
        std::cout << "failed to enumerate physical devices" << std::endl;
        return false;
    }

    //this program will use the very first graphics card
    vulkanPhysicalDevice = devices[0];

    vkGetPhysicalDeviceProperties(vulkanPhysicalDevice, &vulkandeviceProperties);
    vkGetPhysicalDeviceFeatures(vulkanPhysicalDevice, &vulkandeviceFeatures);
    vkGetPhysicalDeviceMemoryProperties(vulkanPhysicalDevice, &vulkandeviceMemoryProperties);

    return true;
}

bool device::create_logical_device(VkInstance instance, VkSurfaceKHR surface, Enable_FeatureFlags featureflags)
{
    VkPhysicalDeviceFeatures enabledeviceFeatures{};
    //enable sample shading
    if ((featureflags & SampleRate_Shading) && vulkandeviceFeatures.sampleRateShading != VK_TRUE)
    {
        std::cout << "device not support sample shading" << std::endl;
        return false;
    }
    enabledeviceFeatures.sampleRateShading = VK_TRUE;

    //enable geometry shader
    if ((featureflags & Geometry_Shader) && vulkandeviceFeatures.geometryShader != VK_TRUE)
    {
        std::cout << "device not support geometry shader" << std::endl;
        return false;
    }
    enabledeviceFeatures.geometryShader = VK_TRUE;

    //enable pipelinestatistics
    if ((featureflags & Pipeline_Statistics_Query) && vulkandeviceFeatures.pipelineStatisticsQuery != VK_TRUE)
    {
        std::cout << "device not support pipelinestatics query" << std::endl;
        return false;
    }
    enabledeviceFeatures.pipelineStatisticsQuery = VK_TRUE;

    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(vulkanPhysicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilyProperties;
    queueFamilyProperties.resize(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(vulkanPhysicalDevice, &queueFamilyCount, queueFamilyProperties.data());

    int queueindex = 0;
    for (const auto& queueFamily : queueFamilyProperties)
    {
        VkBool32 presentsupport;
        vkGetPhysicalDeviceSurfaceSupportKHR(vulkanPhysicalDevice, queueindex, surface, &presentsupport);

        if (presentsupport == VK_TRUE)
        {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                graphicsFamily = queueindex;
            }
            else if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
            {
                computeFamily = queueindex;
            }
        }

        ++queueindex;
    }

    std::vector<uint32_t> queueFamilies = { graphicsFamily, computeFamily };

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    float queuePriority = { 1.0f };
    for (uint32_t queueFamily : queueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    //will use swapchain
    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.pEnabledFeatures = &enabledeviceFeatures;
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

    debug::DeviceCreateInfoLayerRegister(deviceCreateInfo);

    if (vkCreateDevice(vulkanPhysicalDevice, &deviceCreateInfo, VK_NULL_HANDLE, &vulkanDevice) != VK_SUCCESS)
    {
        std::cout << "failed to create logical device!" << std::endl;
        return false;
    }

    return true;
}

bool device::create_command_pool()
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = graphicsFamily;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(vulkanDevice, &poolInfo, VK_NULL_HANDLE, &vulkanCommandPool) != VK_SUCCESS)
    {
        std::cout << "failed to create command pool!" << std::endl;
        return false;
    }

    return true;
}

void device::request_queue(VkQueue& graphics, VkQueue& compute)
{
    vkGetDeviceQueue(vulkanDevice, graphicsFamily, 0, &graphics);
    vkGetDeviceQueue(vulkanDevice, computeFamily, 0, &compute);
}

VkCommandBufferAllocateInfo device::commandbuffer_allocateinfo(uint32_t count)
{
    VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = vulkanCommandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = count;

    return commandBufferAllocateInfo;
}

bool device::create_single_commandbuffer_begin(VkCommandBuffer& commandBuffer)
{
    VkCommandBufferAllocateInfo commandBufferAllocateInfo = commandbuffer_allocateinfo(1);

    if (vkAllocateCommandBuffers(vulkanDevice, &commandBufferAllocateInfo, &commandBuffer) != VK_SUCCESS)
    {
        std::cout << "failed to allocate command buffers!" << std::endl;
        return false;
    }
    VkCommandBufferBeginInfo commandBufferBeginInfo{};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo) != VK_SUCCESS)
    {
        std::cout << "failed to begin command buffer!" << std::endl;
        return false;
    }

    return commandBuffer;
}

bool device::end_commandbuffer_submit(VkQueue graphicsqueue, VkCommandBuffer commandbuffer)
{
    if (vkEndCommandBuffer(commandbuffer) != VK_SUCCESS)
    {
        std::cout << "failed to end command buffer" << std::endl;
        return false;
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandbuffer;

    VkFenceCreateInfo fenceCreateInfo{};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = 0;

    VkFence fence;
    if (vkCreateFence(vulkanDevice, &fenceCreateInfo, nullptr, &fence) != VK_SUCCESS)
    {
        std::cout << "failed to create fence" << std::endl;
        return false;
    }

    if (vkQueueSubmit(graphicsqueue, 1, &submitInfo, fence) != VK_SUCCESS)
    {
        std::cout << "failed to submit queue" << std::endl;
        return -1;
    }

    if (vkWaitForFences(vulkanDevice, 1, &fence, VK_TRUE, 1000000) != VK_SUCCESS)
    {
        std::cout << "failed to wait fence" << std::endl;
        return -1;
    }

    vkDestroyFence(vulkanDevice, fence, nullptr);

    free_command_buffer(1, &commandbuffer);
}

void device::free_command_buffer(uint32_t count, VkCommandBuffer* commandbuffer)
{
    vkFreeCommandBuffers(vulkanDevice, vulkanCommandPool, count, commandbuffer);
}

void device::close()
{
    vkDestroyCommandPool(vulkanDevice, vulkanCommandPool, VK_NULL_HANDLE);

    vkDestroyDevice(vulkanDevice, VK_NULL_HANDLE);
}

