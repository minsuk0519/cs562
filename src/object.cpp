#include "object.hpp"

void object::create_object(unsigned int count, VkBuffer vert, VkBuffer index)
{
    indicescount = count;
    vertexbuffer = vert;
    indexbuffer = index;
}

void object::draw_object(VkCommandBuffer commandbuffer)
{
    VkDeviceSize offsets[1] = { 0 };

    vkCmdBindVertexBuffers(commandbuffer, 0, 1, &vertexbuffer, offsets);
    vkCmdBindIndexBuffer(commandbuffer, indexbuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(commandbuffer, indicescount, 1, 0, 0, 0);
}
