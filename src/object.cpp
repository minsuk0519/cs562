#include "object.hpp"
#include "uniforms.hpp"

object::object()
{
    prop = new ObjectProperties();
}

void object::create_object(unsigned int count, VertexBuffer& buffer)
{
    indicescount = count;
    vertexbuffer = buffer.vertexbuffer.buf;
    indexbuffer = buffer.indexbuffer.buf;
}

void object::draw_object(VkCommandBuffer commandbuffer)
{
    VkDeviceSize offsets[1] = { 0 };

    vkCmdBindVertexBuffers(commandbuffer, 0, 1, &vertexbuffer, offsets);
    vkCmdBindIndexBuffer(commandbuffer, indexbuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(commandbuffer, indicescount, 1, 0, 0, 0);
}
