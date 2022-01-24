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

    bool create_depth_image(device* devicePtr, VkQueue graphicsqueue, VkFormat depthformat, uint32_t width, uint32_t height, Image& image);
    bool create_fb_image(VkDevice vulkandevice, VkFormat format, uint32_t width, uint32_t height, Image& image);

    void free_buffer(VkDevice device, Buffer& buf);
    void free_image(VkDevice device, Image& img);
};