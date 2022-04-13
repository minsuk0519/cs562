#include "buffer.hpp"
#include "render.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

void memory::init(VkPhysicalDeviceMemoryProperties properties)
{
    vulkandeviceMemoryProperties = properties;

    //set all loaded textures are vertically flipped
    //stbi_set_flip_vertically_on_load(true);
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

    devicePtr->end_commandbuffer_submit(graphicsqueue, commandBuffer, false);

    vkDestroyBuffer(devicePtr->vulkanDevice, vertexstagingBuffer.buf, VK_NULL_HANDLE);
    vkFreeMemory(devicePtr->vulkanDevice, vertexstagingBuffer.memory, VK_NULL_HANDLE);
    vkDestroyBuffer(devicePtr->vulkanDevice, indexstagingBuffer.buf, VK_NULL_HANDLE);
    vkFreeMemory(devicePtr->vulkanDevice, indexstagingBuffer.memory, VK_NULL_HANDLE);

    vertex.indexsize = static_cast<uint32_t>(indices.size());

    return true;
}

bool memory::create_depth_image(device* devicePtr, VkQueue /*graphicsqueue*/, VkFormat depthformat, uint32_t width, uint32_t height, uint32_t layer, Image*& image)
{
    image = new Image();

    VkDevice vulkandevice = devicePtr->vulkanDevice;

    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = depthformat;
    imageCreateInfo.extent = { width, height, 1 };
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = layer;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    if(width == 2048) imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (vkCreateImage(vulkandevice, &imageCreateInfo, nullptr, &image->image) != VK_SUCCESS)
    {
        std::cout << "failed to create image!" << std::endl;
        return false;
    }

    image->format = depthformat;
    image->width = width;
    image->height = height;
    image->layer = layer;

    VkMemoryRequirements memReqs{};
    vkGetImageMemoryRequirements(vulkandevice, image->image, &memReqs);

    VkMemoryAllocateInfo memAllloc{};
    memAllloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAllloc.allocationSize = memReqs.size;
    memAllloc.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(vulkandevice, &memAllloc, nullptr, &image->memory) != VK_SUCCESS)
    {
        std::cout << "failed to allocate memory!" << std::endl;
        return false;
    }
    if (vkBindImageMemory(vulkandevice, image->image, image->memory, 0) != VK_SUCCESS)
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
    imageviewCreateInfo.viewType = (layer > 1) ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
    imageviewCreateInfo.image = image->image;
    imageviewCreateInfo.format = depthformat;
    imageviewCreateInfo.subresourceRange.baseMipLevel = 0;
    imageviewCreateInfo.subresourceRange.levelCount = 1;
    imageviewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageviewCreateInfo.subresourceRange.layerCount = layer;
    imageviewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

    if (depthformat >= VK_FORMAT_D16_UNORM_S8_UINT)
    {
        imageviewCreateInfo.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    if (vkCreateImageView(vulkandevice, &imageviewCreateInfo, nullptr, &image->imageView) != VK_SUCCESS)
    {
        std::cout << "failed to create image view" << std::endl;
        return false;
    }

    return true;
}

bool memory::create_fb_image(VkDevice vulkandevice, VkFormat format, uint32_t width, uint32_t height, uint32_t layer, Image*& image)
{
    image = new Image();

    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = format;
    imageCreateInfo.extent.width = width;
    imageCreateInfo.extent.height = height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = layer;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (vkCreateImage(vulkandevice, &imageCreateInfo, nullptr, &image->image))
    {
        std::cout << "failed to create image!" << std::endl;
        return false;
    }

    image->format = format;
    image->width = width;
    image->height = height;
    image->layer = layer;

    VkMemoryRequirements memReqs{};
    vkGetImageMemoryRequirements(vulkandevice, image->image, &memReqs);

    VkMemoryAllocateInfo memAllloc{};
    memAllloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAllloc.allocationSize = memReqs.size;
    memAllloc.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(vulkandevice, &memAllloc, nullptr, &image->memory) != VK_SUCCESS)
    {
        std::cout << "failed to allocate memory!" << std::endl;
        return false;
    }
    if (vkBindImageMemory(vulkandevice, image->image, image->memory, 0) != VK_SUCCESS)
    {
        std::cout << "failed to bind image memory!" << std::endl;
        return false;
    }

    VkImageViewCreateInfo imageviewCreateInfo{};
    imageviewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageviewCreateInfo.viewType = (layer > 1) ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
    imageviewCreateInfo.format = format;
    imageviewCreateInfo.image = image->image;
    imageviewCreateInfo.subresourceRange = {};
    imageviewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageviewCreateInfo.subresourceRange.baseMipLevel = 0;
    imageviewCreateInfo.subresourceRange.levelCount = 1;
    imageviewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageviewCreateInfo.subresourceRange.layerCount = layer;

    if (vkCreateImageView(vulkandevice, &imageviewCreateInfo, nullptr, &image->imageView) != VK_SUCCESS)
    {
        std::cout << "failed to create image view" << std::endl;
        return false;
    }

    return true;
}

void memory::transitionImage(device* devicePtr, VkQueue graphicsqueue, VkImage image, uint32_t miplevel, VkImageAspectFlags aspectmask, VkImageLayout oldImageLayout, VkImageLayout newImageLayout,
    VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask)
{
    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = aspectmask;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = miplevel;
    subresourceRange.layerCount = 1;

    VkImageMemoryBarrier imageMemoryBarrier{};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.oldLayout = oldImageLayout;
    imageMemoryBarrier.newLayout = newImageLayout;
    imageMemoryBarrier.image = image;
    imageMemoryBarrier.subresourceRange = subresourceRange;

    setimagebarriermask(imageMemoryBarrier, oldImageLayout, newImageLayout);

    VkCommandBuffer commandBuffer;

    devicePtr->create_single_commandbuffer_begin(commandBuffer);

    vkCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

    devicePtr->end_commandbuffer_submit(graphicsqueue, commandBuffer, false);
}

void memory::free_buffer(VkDevice device, Buffer& buf)
{
    vkDestroyBuffer(device, buf.buf, VK_NULL_HANDLE);
    vkFreeMemory(device, buf.memory, VK_NULL_HANDLE);
}

void memory::free_image(VkDevice device, Image*& img)
{
    vkDestroyImageView(device, img->imageView, VK_NULL_HANDLE);
    vkDestroyImage(device, img->image, VK_NULL_HANDLE);
    vkFreeMemory(device, img->memory, VK_NULL_HANDLE);
}

bool memory::load_texture_image(device* devicePtr, VkQueue graphicsqueue, std::string filepath, Image*& image, uint32_t& miplevel, VkImageLayout imagelayout)
{
    image = new Image();

    int texWidth, texHeight, texChannels;
    float* pixels = stbi_loadf(("data/textures/" + filepath).c_str(), &texWidth, &texHeight, &texChannels, 0);

    if (!pixels)
    {
        throw std::runtime_error("failed to load texture image!");
    }

    float* pixeldata = new float[texWidth * texHeight * 4];
    unsigned int dataindex = 0;
    for (int i = 0; i < texWidth * texHeight * 3; i += 3)
    {     
        pixeldata[dataindex++] = pixels[i + 0];
        pixeldata[dataindex++] = pixels[i + 1];
        pixeldata[dataindex++] = pixels[i + 2];
        pixeldata[dataindex++] = 1.0f;
    }
    stbi_image_free(pixels);

    miplevel = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;;

    Buffer buf;
    VkDeviceSize imageSize = texWidth * texHeight * sizeof(float) * 4;

    create_buffer(devicePtr->vulkanDevice, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, imageSize, 1, buf, pixeldata);

    void* data;
    vkMapMemory(devicePtr->vulkanDevice, buf.memory, 0, imageSize, 0, &data);
    memcpy(data, pixeldata, static_cast<size_t>(imageSize));
    vkUnmapMemory(devicePtr->vulkanDevice, buf.memory);

    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    //16 bits for hdr image (will be changed later for general texture)
    imageCreateInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    imageCreateInfo.extent = { static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1 };
    imageCreateInfo.mipLevels = miplevel;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (vkCreateImage(devicePtr->vulkanDevice, &imageCreateInfo, VK_NULL_HANDLE, &image->image) != VK_SUCCESS)
    {
        std::cout << "failed to create image!" << std::endl;
        return false;
    }

    image->format = imageCreateInfo.format;
    image->width = texWidth;
    image->height = texHeight;
    image->layer = 1;

    VkMemoryRequirements memReqs{};
    vkGetImageMemoryRequirements(devicePtr->vulkanDevice, image->image, &memReqs);

    VkMemoryAllocateInfo memAllloc{};
    memAllloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAllloc.allocationSize = memReqs.size;
    memAllloc.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(devicePtr->vulkanDevice, &memAllloc, nullptr, &image->memory) != VK_SUCCESS)
    {
        std::cout << "failed to allocate memory!" << std::endl;
        return false;
    }
    if (vkBindImageMemory(devicePtr->vulkanDevice, image->image, image->memory, 0) != VK_SUCCESS)
    {
        std::cout << "failed to bind image memory!" << std::endl;
        return false;
    }

    transitionImage(devicePtr, graphicsqueue, image->image, miplevel, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkCommandBuffer commandBuffer;
    VkBufferImageCopy bufferimageCopy{};
    bufferimageCopy.bufferOffset = 0;
    bufferimageCopy.bufferRowLength = 0;
    bufferimageCopy.bufferImageHeight = 0;
    bufferimageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    bufferimageCopy.imageSubresource.mipLevel = 0;
    bufferimageCopy.imageSubresource.baseArrayLayer = 0;
    bufferimageCopy.imageSubresource.layerCount = 1;
    bufferimageCopy.imageOffset = { 0, 0, 0 };
    bufferimageCopy.imageExtent = imageCreateInfo.extent;

    devicePtr->create_single_commandbuffer_begin(commandBuffer);
    vkCmdCopyBufferToImage(commandBuffer, buf.buf, image->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferimageCopy);
    devicePtr->end_commandbuffer_submit(graphicsqueue, commandBuffer, false);

    generate_mipmap(devicePtr, graphicsqueue, imageCreateInfo.format, image->image, texWidth, texHeight, miplevel, imagelayout);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image->image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = image->format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = miplevel;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(devicePtr->vulkanDevice, &viewInfo, VK_NULL_HANDLE, &image->imageView) != VK_SUCCESS)
    {
        std::cout << "failed to create image view!" << std::endl;
        return false;
    }

    free_buffer(devicePtr->vulkanDevice, buf);

    delete[] pixeldata;

    return true;
}

//the imagelayout must be 
void memory::generate_mipmap(device* devicePtr, VkQueue graphicsqueue, VkFormat imageFormat, VkImage& image, uint32_t width, uint32_t height, uint32_t mipmap,
    VkImageLayout imagelayout)
{
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(devicePtr->vulkanPhysicalDevice, imageFormat, &formatProperties);

    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) 
    {
        std::cout << "texture image format does not support linear blitting!" << std::endl;
    }

    VkCommandBuffer commandBuffer;
    devicePtr->create_single_commandbuffer_begin(commandBuffer);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = width;
    int32_t mipHeight = height;

    for (uint32_t i = 1; i < mipmap; i++) 
    {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        VkImageBlit blit{};
        blit.srcOffsets[0] = { 0, 0, 0 };
        blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = { 0, 0, 0 };
        blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(commandBuffer,
            image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = imagelayout;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = mipmap - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = imagelayout;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr,
        0, nullptr,
        1, &barrier);

    devicePtr->end_commandbuffer_submit(graphicsqueue, commandBuffer, false);
}

void memory::generate_filteredtex(device* devicePtr, VkQueue graphicsqueue, VkQueue computequeue, Image*& src, Image*& target, VkSampler sampler)
{
    create_fb_image(devicePtr->vulkanDevice, src->format, 400, 200, 1, target);
    transitionImage(devicePtr, graphicsqueue, target->image, 1, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    VkDescriptorSetLayout descriptorsetlayout;
    VkDescriptorSet descriptorset;
    VkPipeline pipeline;
    VkPipelineLayout pipelinelayout;

    VkDescriptorPool descriptorpool;
    descriptor::create_descriptorpool(devicePtr->vulkanDevice, descriptorpool, {
    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2 },
    { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2 },
        });

    descriptor::create_descriptorset_layout(devicePtr->vulkanDevice, descriptorsetlayout, descriptorset, descriptorpool, {
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 0},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 1},
        });

    descriptor::write_descriptorset(devicePtr->vulkanDevice, descriptorset, {
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0, {}, {sampler, src->imageView, VK_IMAGE_LAYOUT_GENERAL}},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, {}, {sampler, target->imageView, VK_IMAGE_LAYOUT_GENERAL}},
        });

    pipeline::create_pipelinelayout(devicePtr->vulkanDevice, descriptorsetlayout, pipelinelayout, {});

    VkComputePipelineCreateInfo pipelineCreateInfo{};
    pipelineCreateInfo.layout = pipelinelayout;

    pipeline::create_compute_pipeline(devicePtr->vulkanDevice, pipelineCreateInfo, pipeline, VK_NULL_HANDLE,
        { "data/shaders/envmapping.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT });

    VkCommandBuffer commandBuffer;
    VkCommandBufferAllocateInfo commandBufferAllocateInfo = devicePtr->commandbuffer_allocateinfo(1, true);

    if (vkAllocateCommandBuffers(devicePtr->vulkanDevice, &commandBufferAllocateInfo, &commandBuffer) != VK_SUCCESS)
    {
        std::cout << "failed to allocate command buffers!" << std::endl;
        return;
    }
    VkCommandBufferBeginInfo commandBufferBeginInfo{};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo) != VK_SUCCESS)
    {
        std::cout << "failed to begin command buffer!" << std::endl;
        return;
    }

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelinelayout, 0, 1,
        &descriptorset, 0, 0);

    vkCmdDispatch(commandBuffer, target->width, target->height, 1);

    devicePtr->end_commandbuffer_submit(computequeue, commandBuffer, true);

    descriptor::close_descriptorset_layout(devicePtr->vulkanDevice, descriptorsetlayout);
    pipeline::closepipeline(devicePtr->vulkanDevice, pipeline);
    pipeline::close_pipelinelayout(devicePtr->vulkanDevice, pipelinelayout);
    descriptor::close_descriptorpool(devicePtr->vulkanDevice, descriptorpool);
}

void memory::setimagebarriermask(VkImageMemoryBarrier& imageMemoryBarrier, VkImageLayout oldImageLayout, VkImageLayout newImageLayout)
{
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
}

bool memory::create_sampler(device* devicePtr, VkSampler& sampler, uint32_t miplevel)
{
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
    samplerCreateInfo.maxLod = static_cast<float>(miplevel);
    samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

    if (vkCreateSampler(devicePtr->vulkanDevice, &samplerCreateInfo, VK_NULL_HANDLE, &sampler))
    {
        std::cout << "failed to create sampler!" << std::endl;
        return false;
    }

    return true;
}
