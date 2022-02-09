#include "buffer.hpp"

void memory::init(VkPhysicalDeviceMemoryProperties properties)
{
    vulkandeviceMemoryProperties = properties;
}

uint32_t memory::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    for (uint32_t i = 0; i < vulkandeviceMemoryProperties.memoryTypeCount; ++i)
    {
        if ((typeFilter & (1 << i)) && (vulkandeviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    std::cout << "failed to find suitable memory type!" << std::endl;
    return static_cast<uint32_t>(-1);
}

bool memory::create_buffer(VkDevice device, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryproperty, VkDeviceSize size, uint32_t num, Buffer& buffer, void* data)
{
    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.usage = usage;
    bufferCreateInfo.size = size * num;

    if (vkCreateBuffer(device, &bufferCreateInfo, VK_NULL_HANDLE, &buffer.buf) != VK_SUCCESS)
    {
        std::cout << "failed to create buffer!" << std::endl;
        return false;
    }

    VkMemoryRequirements memoryRequirements{};
    VkMemoryAllocateInfo memoryAllocateInfo{};
    memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

    vkGetBufferMemoryRequirements(device, buffer.buf, &memoryRequirements);

    memoryAllocateInfo.allocationSize = memoryRequirements.size;
    memoryAllocateInfo.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, memoryproperty);

    VkMemoryAllocateFlagsInfoKHR allocateFlagsInfo{};
    if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
    {
        allocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
        allocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
        memoryAllocateInfo.pNext = &allocateFlagsInfo;
    }
    if (vkAllocateMemory(device, &memoryAllocateInfo, VK_NULL_HANDLE, &buffer.memory) != VK_SUCCESS)
    {
        std::cout << "failed to allocate memory!" << std::endl;
        return false;
    }

    if (data != nullptr)
    {
        void* mapped;
        if (vkMapMemory(device, buffer.memory, 0, size * num, 0, &mapped) != VK_SUCCESS)
        {
            std::cout << "failed to mapping memory!" << std::endl;
            return false;
        }

        memcpy(mapped, data, size * num);

        if ((memoryproperty & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
        {
            VkMappedMemoryRange mappedMemoryRange{};
            mappedMemoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            mappedMemoryRange.memory = buffer.memory;
            mappedMemoryRange.offset = 0;
            mappedMemoryRange.size = size * num;
            vkFlushMappedMemoryRanges(device, 1, &mappedMemoryRange);
        }
        vkUnmapMemory(device, buffer.memory);
    }

    if (vkBindBufferMemory(device, buffer.buf, buffer.memory, 0) != VK_SUCCESS)
    {
        std::cout << "failed to bind buffer memory!" << std::endl;
        return false;
    }

    buffer.size = size * num;
    buffer.range = size;

    return true;
}

bool memory::create_vertex_index_buffer(VkDevice vulkandevice, VkQueue graphicsqueue, device* devicePtr, std::vector<float> vertices, std::vector<uint32_t> indices, VertexBuffer& vertex)
{
    VkDeviceSize vertexBufferSize = vertices.size() * sizeof(float);
    VkDeviceSize indexBufferSize = indices.size() * sizeof(uint32_t);

    Buffer vertexstagingBuffer{};
    Buffer indexstagingBuffer{};

    create_buffer(vulkandevice, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vertices.size() * sizeof(float), 1, vertexstagingBuffer, vertices.data());
    create_buffer(vulkandevice, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, indices.size() * sizeof(uint32_t), 1, indexstagingBuffer, indices.data());
    create_buffer(vulkandevice, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertices.size() * sizeof(float), 1, vertex.vertexbuffer);
    create_buffer(vulkandevice, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indices.size() * sizeof(uint32_t), 1, vertex.indexbuffer);

    VkCommandBuffer commandBuffer;

    devicePtr->create_single_commandbuffer_begin(commandBuffer);

    VkBufferCopy copyRegion = {};

    copyRegion.size = vertexBufferSize;
    vkCmdCopyBuffer(commandBuffer, vertexstagingBuffer.buf, vertex.vertexbuffer.buf, 1, &copyRegion);

    copyRegion.size = indexBufferSize;
    vkCmdCopyBuffer(commandBuffer, indexstagingBuffer.buf, vertex.indexbuffer.buf, 1, &copyRegion);

    devicePtr->end_commandbuffer_submit(graphicsqueue, commandBuffer);

    vkDestroyBuffer(devicePtr->vulkanDevice, vertexstagingBuffer.buf, VK_NULL_HANDLE);
    vkFreeMemory(devicePtr->vulkanDevice, vertexstagingBuffer.memory, VK_NULL_HANDLE);
    vkDestroyBuffer(devicePtr->vulkanDevice, indexstagingBuffer.buf, VK_NULL_HANDLE);
    vkFreeMemory(devicePtr->vulkanDevice, indexstagingBuffer.memory, VK_NULL_HANDLE);

    vertex.indexsize = static_cast<uint32_t>(indices.size());

    return true;
}

bool memory::create_depth_image(device* devicePtr, VkQueue graphicsqueue, VkFormat depthformat, uint32_t width, uint32_t height, Image& image)
{
    VkDevice vulkandevice = devicePtr->vulkanDevice;

    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = depthformat;
    imageCreateInfo.extent = { width, height, 1 };
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    if(width == 2048) imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (vkCreateImage(vulkandevice, &imageCreateInfo, nullptr, &image.image) != VK_SUCCESS)
    {
        std::cout << "failed to create image!" << std::endl;
        return false;
    }

    image.format = depthformat;

    VkMemoryRequirements memReqs{};
    vkGetImageMemoryRequirements(vulkandevice, image.image, &memReqs);

    VkMemoryAllocateInfo memAllloc{};
    memAllloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAllloc.allocationSize = memReqs.size;
    memAllloc.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(vulkandevice, &memAllloc, nullptr, &image.memory) != VK_SUCCESS)
    {
        std::cout << "failed to allocate memory!" << std::endl;
        return false;
    }
    if (vkBindImageMemory(vulkandevice, image.image, image.memory, 0) != VK_SUCCESS)
    {
        std::cout << "failed to bind image memory!" << std::endl;
        return false;
    }

    //deprecated but be later used
    //VkCommandBuffer commandBuffer;

    //devicePtr->create_single_commandbuffer_begin(commandBuffer);

    //VkImageSubresourceRange subresourceRange = {};
    //subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    //subresourceRange.baseMipLevel = 0;
    //subresourceRange.levelCount = 1;
    //subresourceRange.layerCount = 1;

    //VkImageMemoryBarrier imageMemoryBarrier{};
    //imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    //imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    //imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    //imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    //imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    //imageMemoryBarrier.image = image.image;
    //imageMemoryBarrier.subresourceRange = subresourceRange;
    //imageMemoryBarrier.srcAccessMask = 0;
    //imageMemoryBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    //vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

    //devicePtr->end_commandbuffer_submit(graphicsqueue, commandBuffer);



    VkImageViewCreateInfo imageviewCreateInfo{};
    imageviewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageviewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageviewCreateInfo.image = image.image;
    imageviewCreateInfo.format = depthformat;
    imageviewCreateInfo.subresourceRange.baseMipLevel = 0;
    imageviewCreateInfo.subresourceRange.levelCount = 1;
    imageviewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageviewCreateInfo.subresourceRange.layerCount = 1;
    imageviewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

    if (depthformat >= VK_FORMAT_D16_UNORM_S8_UINT)
    {
        imageviewCreateInfo.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    if (vkCreateImageView(vulkandevice, &imageviewCreateInfo, nullptr, &image.imageView) != VK_SUCCESS)
    {
        std::cout << "failed to create image view" << std::endl;
        return false;
    }

    return true;
}

bool memory::create_fb_image(VkDevice vulkandevice, VkFormat format, uint32_t width, uint32_t height, Image& image)
{
    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = format;
    imageCreateInfo.extent.width = width;
    imageCreateInfo.extent.height = height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    if (vkCreateImage(vulkandevice, &imageCreateInfo, nullptr, &image.image))
    {
        std::cout << "failed to create image!" << std::endl;
        return false;
    }

    image.format = format;

    VkMemoryRequirements memReqs{};
    vkGetImageMemoryRequirements(vulkandevice, image.image, &memReqs);

    VkMemoryAllocateInfo memAllloc{};
    memAllloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAllloc.allocationSize = memReqs.size;
    memAllloc.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(vulkandevice, &memAllloc, nullptr, &image.memory) != VK_SUCCESS)
    {
        std::cout << "failed to allocate memory!" << std::endl;
        return false;
    }
    if (vkBindImageMemory(vulkandevice, image.image, image.memory, 0) != VK_SUCCESS)
    {
        std::cout << "failed to bind image memory!" << std::endl;
        return false;
    }

    VkImageViewCreateInfo imageviewCreateInfo{};
    imageviewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageviewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageviewCreateInfo.format = format;
    imageviewCreateInfo.image = image.image;
    imageviewCreateInfo.subresourceRange = {};
    imageviewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageviewCreateInfo.subresourceRange.baseMipLevel = 0;
    imageviewCreateInfo.subresourceRange.levelCount = 1;
    imageviewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageviewCreateInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(vulkandevice, &imageviewCreateInfo, nullptr, &image.imageView) != VK_SUCCESS)
    {
        std::cout << "failed to create image view" << std::endl;
        return false;
    }

    return true;
}

void memory::transitionImage(device* devicePtr, VkQueue graphicsqueue, Image& image, VkImageAspectFlags aspectmask, VkImageLayout oldImageLayout, VkImageLayout newImageLayout,
    VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask)
{
    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = aspectmask;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = 1;
    subresourceRange.layerCount = 1;

    VkImageMemoryBarrier imageMemoryBarrier{};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.oldLayout = oldImageLayout;
    imageMemoryBarrier.newLayout = newImageLayout;
    imageMemoryBarrier.image = image.image;
    imageMemoryBarrier.subresourceRange = subresourceRange;

    switch (oldImageLayout)
    {
    case VK_IMAGE_LAYOUT_UNDEFINED:
        imageMemoryBarrier.srcAccessMask = 0;
        break;

    case VK_IMAGE_LAYOUT_PREINITIALIZED:
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;

    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
    default:
        break;
    }

    switch (newImageLayout)
    {
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        break;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        if (imageMemoryBarrier.srcAccessMask == 0)
        {
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        }
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
    default:
        break;
    }

    VkCommandBuffer commandBuffer;

    devicePtr->create_single_commandbuffer_begin(commandBuffer);

    vkCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

    devicePtr->end_commandbuffer_submit(graphicsqueue, commandBuffer);
}

void memory::free_buffer(VkDevice device, Buffer& buf)
{
    vkDestroyBuffer(device, buf.buf, VK_NULL_HANDLE);
    vkFreeMemory(device, buf.memory, VK_NULL_HANDLE);
}

void memory::free_image(VkDevice device, Image& img)
{
    vkDestroyImageView(device, img.imageView, VK_NULL_HANDLE);
    vkDestroyImage(device, img.image, VK_NULL_HANDLE);
    vkFreeMemory(device, img.memory, VK_NULL_HANDLE);
}
