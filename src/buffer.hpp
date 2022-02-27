#pragma once
#include <vulkan/vulkan.h>

#include <iostream>
#include <vector>

#include "device.hpp"

struct Buffer
{
    VkBuffer buf;
    VkDeviceMemory memory;
    VkDeviceSize size;
    VkDeviceSize range;
};

struct Image
{
    VkImage image;
    VkImageView imageView;
    VkFormat format;
    VkDeviceMemory memory;
};

struct VertexBuffer
{
    Buffer vertexbuffer;
    Buffer indexbuffer;
    uint32_t indexsize;
};

class memory
{
private:
    VkPhysicalDeviceMemoryProperties vulkandeviceMemoryProperties;

public:
    void init(VkPhysicalDeviceMemoryProperties properties);

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    bool create_buffer(VkDevice device, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryproperty, VkDeviceSize size, uint32_t num, Buffer& buffer, void* data = nullptr);

    bool create_vertex_index_buffer(VkDevice vulkandevice, VkQueue graphicsqueue, device* devicePtr, std::vector<float> vertices, std::vector<uint32_t> indices, VertexBuffer& vertex);

    bool create_depth_image(device* devicePtr, VkQueue graphicsqueue, VkFormat depthformat, uint32_t width, uint32_t height, Image*& image);
    bool create_fb_image(VkDevice vulkandevice, VkFormat format, uint32_t width, uint32_t height, Image*& image);

    void transitionImage(device* devicePtr, VkQueue graphicsqueue, VkImage image, uint32_t miplevel, VkImageAspectFlags aspectmask, VkImageLayout oldImageLayout, VkImageLayout newImageLayout,
        VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

    void free_buffer(VkDevice device, Buffer& buf);
    void free_image(VkDevice device, Image*& img);

    bool load_texture_image(device* devicePtr, VkQueue graphicsqueue, std::string filepath, Image*& image, uint32_t& miplevel);
    void generate_mipmap(device* devicePtr, VkQueue graphicsqueue, VkFormat imageFormat, VkImage& image, uint32_t width, uint32_t height, uint32_t mipmap);

    bool create_sampler(device* devicePtr, VkSampler& sampler, uint32_t miplevel);
};