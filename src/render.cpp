#include "render.hpp"
#include "helper.hpp"

#include <iostream>
#include <array>
#include <vector>

#include <glm/glm.hpp>

bool pipeline::create_pipieline(VkDevice device, VkGraphicsPipelineCreateInfo& pipelineCreateInfo, VkPipeline& pipeline, VkPipelineCache pipelinecache, std::vector<shaderinput> shaderinfos)
{
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.flags = 0;
    pipelineCreateInfo.basePipelineIndex = -1;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;

    std::vector<VkPipelineShaderStageCreateInfo> shadercreateinfos;
    for (auto shaderinfo : shaderinfos)
    {
        shadercreateinfos.push_back(helper::loadShader(device, shaderinfo.filepath, shaderinfo.shaderflag));
    }

    pipelineCreateInfo.stageCount = static_cast<uint32_t>(shadercreateinfos.size());
    pipelineCreateInfo.pStages = shadercreateinfos.data();

    if (vkCreateGraphicsPipelines(device, pipelinecache, 1, &pipelineCreateInfo, VK_NULL_HANDLE, &pipeline) != VK_SUCCESS)
    {
        std::cout << "failed to create graphics pipeline!" << std::endl;

        return false;
    }

    vkDestroyShaderModule(device, shadercreateinfos[0].module, VK_NULL_HANDLE);
    vkDestroyShaderModule(device, shadercreateinfos[1].module, VK_NULL_HANDLE);

    return true;
}

bool pipeline::create_pipelinelayout(VkDevice device, VkDescriptorSetLayout descriptorsetlayout, VkPipelineLayout& pipelinelayout)
{
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &descriptorsetlayout;

    if (vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, VK_NULL_HANDLE, &pipelinelayout) != VK_SUCCESS)
    {
        std::cout << "failed to create pipeline layout" << std::endl;
        return false;
    }

    return true;
}

void pipeline::closepipeline(VkDevice device, VkPipeline& pipeline)
{
    vkDestroyPipeline(device, pipeline, VK_NULL_HANDLE);
}

void pipeline::close_pipelinelayout(VkDevice device, VkPipelineLayout& pipelinelayout)
{
    vkDestroyPipelineLayout(device, pipelinelayout, VK_NULL_HANDLE);
}

VkPipelineInputAssemblyStateCreateInfo pipeline::getInputAssemblyCreateInfo()
{
    VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo{};
    pipelineInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    pipelineInputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipelineInputAssemblyStateCreateInfo.flags = 0;
    pipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;
    
    return pipelineInputAssemblyStateCreateInfo;
}

VkPipelineRasterizationStateCreateInfo pipeline::getRasterizationCreateInfo(VkCullModeFlags cullmode)
{
    VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo{};
    pipelineRasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    pipelineRasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    pipelineRasterizationStateCreateInfo.cullMode = cullmode;
    pipelineRasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    pipelineRasterizationStateCreateInfo.flags = 0;
    pipelineRasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
    pipelineRasterizationStateCreateInfo.lineWidth = 1.0f;

    return pipelineRasterizationStateCreateInfo;
}

VkPipelineColorBlendAttachmentState pipeline::getColorBlendAttachment(VkBool32 blendenable)
{
    VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState{};
    pipelineColorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;
    pipelineColorBlendAttachmentState.blendEnable = blendenable;
    pipelineColorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    pipelineColorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    pipelineColorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
    pipelineColorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    pipelineColorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    pipelineColorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

    return pipelineColorBlendAttachmentState;
}

VkPipelineColorBlendStateCreateInfo pipeline::getColorBlendCreateInfo(uint32_t blendcount, VkPipelineColorBlendAttachmentState* blenddata)
{
    VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo{};
    pipelineColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    pipelineColorBlendStateCreateInfo.attachmentCount = blendcount;
    pipelineColorBlendStateCreateInfo.pAttachments = blenddata;

    return pipelineColorBlendStateCreateInfo;
}

VkPipelineMultisampleStateCreateInfo pipeline::getMultisampleCreateInfo(VkSampleCountFlagBits samplecount)
{
    VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo{};
    pipelineMultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pipelineMultisampleStateCreateInfo.rasterizationSamples = samplecount;
    pipelineMultisampleStateCreateInfo.flags = 0;

    return pipelineMultisampleStateCreateInfo;
}

VkViewport pipeline::getViewport(uint32_t width, uint32_t height)
{
    VkViewport viewport{};
    viewport.width = static_cast<float>(width);
    viewport.height = static_cast<float>(height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    return viewport;
}

VkRect2D pipeline::getScissor(uint32_t width, uint32_t height)
{
    VkRect2D scissor{};
    scissor.extent.width = width;
    scissor.extent.height = height;
    scissor.offset.x = 0;
    scissor.offset.y = 0;

    return scissor;
}

VkPipelineViewportStateCreateInfo pipeline::getViewportCreateInfo(VkViewport* viewport, VkRect2D* scissor)
{
    VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo{};
    pipelineViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    pipelineViewportStateCreateInfo.viewportCount = 1;
    pipelineViewportStateCreateInfo.pViewports = viewport;
    pipelineViewportStateCreateInfo.scissorCount = 1;
    pipelineViewportStateCreateInfo.pScissors = scissor;
    pipelineViewportStateCreateInfo.flags = 0;

    return pipelineViewportStateCreateInfo;
}

VkPipelineDepthStencilStateCreateInfo pipeline::getDepthStencilCreateInfo(VkBool32 depthEnable, VkCompareOp depthcomp)
{
    VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo{};
    pipelineDepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    pipelineDepthStencilStateCreateInfo.depthTestEnable = depthEnable;
    pipelineDepthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
    pipelineDepthStencilStateCreateInfo.depthCompareOp = depthcomp;
    pipelineDepthStencilStateCreateInfo.back.compareOp = VK_COMPARE_OP_ALWAYS;

    return pipelineDepthStencilStateCreateInfo;
}

VkVertexInputBindingDescription pipeline::getVertexinputbindingDescription(uint32_t stride)
{
    VkVertexInputBindingDescription vertexInputBindingDescription{};
    vertexInputBindingDescription.binding = 0;
    vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    vertexInputBindingDescription.stride = stride;

    return vertexInputBindingDescription;
}

VkPipelineVertexInputStateCreateInfo pipeline::getVertexinputAttributeDescription(VkVertexInputBindingDescription* vertexinputbinding, std::vector<VkVertexInputAttributeDescription>& vertexinputdescription)
{
    VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{};
    pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
    pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = vertexinputbinding;
    pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexinputdescription.size());
    pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexinputdescription.data();

    return pipelineVertexInputStateCreateInfo;
}

bool renderpass::create_renderpass(VkDevice device, VkRenderPass& renderpass, std::vector<attachmentDesc> attachmentDescriptions)
{
    std::vector<VkAttachmentDescription> renderpassAttachments{};

    VkAttachmentReference depthReference{};
    depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    std::vector<VkAttachmentReference> colorReferences = {};
    VkAttachmentReference colorReference;
    colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    for (auto attachdesc : attachmentDescriptions)
    {
        VkAttachmentDescription attachmentdescription{};
        attachmentdescription.format = attachdesc.format;
        attachmentdescription.samples = VK_SAMPLE_COUNT_1_BIT;
        attachmentdescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachmentdescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachmentdescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachmentdescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachmentdescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachmentdescription.finalLayout = (attachdesc.swapchain) ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : (attachdesc.depth) ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        renderpassAttachments.push_back(attachmentdescription);
        if (attachdesc.depth) 
        {
            depthReference.attachment = attachdesc.location;
        }
        else
        {
            colorReference.attachment = attachdesc.location;
            colorReferences.push_back(colorReference);
        }
    }

    VkSubpassDescription subpassDescription = {};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());
    subpassDescription.pColorAttachments = colorReferences.data();
    subpassDescription.pDepthStencilAttachment = &depthReference;
    subpassDescription.inputAttachmentCount = 0;
    subpassDescription.pInputAttachments = nullptr;
    subpassDescription.preserveAttachmentCount = 0;
    subpassDescription.pPreserveAttachments = nullptr;
    subpassDescription.pResolveAttachments = nullptr;

    std::array<VkSubpassDependency, 2> dependencies;

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(renderpassAttachments.size());
    renderPassInfo.pAttachments = renderpassAttachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpassDescription;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderpass) != VK_SUCCESS)
    {
        std::cout << "failed to create renderpass!" << std::endl;
        return false;
    }

    return true;
}

bool renderpass::create_framebuffer(VkDevice device, VkRenderPass renderpass, VkFramebuffer& framebuffer, uint32_t width, uint32_t height, std::vector<VkImageView> imageviews)
{
    VkFramebufferCreateInfo frameBufferCreateInfo = {};
    frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBufferCreateInfo.pNext = NULL;
    frameBufferCreateInfo.renderPass = renderpass;
    frameBufferCreateInfo.attachmentCount = static_cast<uint32_t>(imageviews.size());
    frameBufferCreateInfo.pAttachments = imageviews.data();
    frameBufferCreateInfo.width = width;
    frameBufferCreateInfo.height = height;
    frameBufferCreateInfo.layers = 1;

    if (vkCreateFramebuffer(device, &frameBufferCreateInfo, nullptr, &framebuffer) != VK_SUCCESS)
    {
        std::cout << "failed to create framebuffer!" << std::endl;
        return false;
    }

    return true;
}

void renderpass::close_renderpass(VkDevice device, VkRenderPass& renderpass)
{
    vkDestroyRenderPass(device, renderpass, VK_NULL_HANDLE);
}

void renderpass::close_framebuffer(VkDevice device, VkFramebuffer& framebuffer)
{
    vkDestroyFramebuffer(device, framebuffer, VK_NULL_HANDLE);
}

bool descriptor::create_descriptorpool(VkDevice device, VkDescriptorPool& descriptorpool, std::vector<VkDescriptorPoolSize> descriptorpoolsizes)
{
    uint32_t sets = 0;
    for (auto poolsize : descriptorpoolsizes)
    {
        sets += poolsize.descriptorCount;
    }

    VkDescriptorPoolCreateInfo descriptorPoolInfo{};
    descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(descriptorpoolsizes.size());
    descriptorPoolInfo.pPoolSizes = descriptorpoolsizes.data();
    descriptorPoolInfo.maxSets = sets;

    if (vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorpool) != VK_SUCCESS)
    {
        std::cout << "failed to create descriptorpool!" << std::endl;
        return false;
    }

    return true;
}

bool descriptor::create_descriptorset_layout(VkDevice device, VkDescriptorSetLayout& descriptorsetlayout, VkDescriptorSet& descriptorset, VkDescriptorPool descriptorpool, std::vector<layoutform> layoutforms)
{
    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings;

    VkDescriptorSetLayoutBinding setLayoutBinding{};
    setLayoutBinding.descriptorCount = 1;

    for (auto layout : layoutforms)
    {
        setLayoutBinding.descriptorType = layout.type;
        setLayoutBinding.binding = layout.binding;
        setLayoutBinding.stageFlags = layout.stage;
        setLayoutBindings.push_back(setLayoutBinding);
    }

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
    descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo.pBindings = setLayoutBindings.data();
    descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());

    if (vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCreateInfo, VK_NULL_HANDLE, &descriptorsetlayout) != VK_SUCCESS)
    {
        std::cout << "failed to create descriptorset layout!" << std::endl;
        return false;
    }

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.descriptorPool = descriptorpool;
    descriptorSetAllocateInfo.pSetLayouts = &descriptorsetlayout;
    descriptorSetAllocateInfo.descriptorSetCount = 1;

    if (vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, &descriptorset) != VK_SUCCESS)
    {
        std::cout << "failed to allocatedescriptorsets!" << std::endl;
        return false;
    }

    return true;
}

bool descriptor::write_descriptorset(VkDevice device, VkDescriptorSet descriptorset, std::vector<descriptordata> descriptors)
{
    std::vector<VkWriteDescriptorSet> writeDescriptorSets = {};

    VkWriteDescriptorSet writeDescriptorSet{};
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.dstSet = descriptorset;
    writeDescriptorSet.descriptorCount = 1;

    VkDescriptorBufferInfo descriptorBufferInfo{};
    for (const auto& data : descriptors)
    {
        writeDescriptorSet.descriptorType = data.type;
        writeDescriptorSet.dstBinding = data.binding;
        writeDescriptorSets.push_back(writeDescriptorSet);
        if(data.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER || data.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
        {
            writeDescriptorSets.back().pBufferInfo = &(data.buf);
            writeDescriptorSets.back().pImageInfo = nullptr;
        }
        else
        {
            writeDescriptorSets.back().pBufferInfo = nullptr;
            writeDescriptorSets.back().pImageInfo = &data.img;
        }
    }

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, VK_NULL_HANDLE);

    return true;
}

void descriptor::close_descriptorpool(VkDevice device, VkDescriptorPool& descriptorpool)
{
    vkDestroyDescriptorPool(device, descriptorpool, VK_NULL_HANDLE);
}

void descriptor::close_descriptorset_layout(VkDevice device, VkDescriptorSetLayout& descriptorsetlayout)
{
    vkDestroyDescriptorSetLayout(device, descriptorsetlayout, VK_NULL_HANDLE);
}

void render::draw(VkCommandBuffer commandbuffer, VertexBuffer vertexbuffer)
{
    VkDeviceSize vertexoffsets[1] = { 0 };

    vkCmdBindVertexBuffers(commandbuffer, 0, 1, &vertexbuffer.vertexbuffer.buf, vertexoffsets);
    vkCmdBindIndexBuffer(commandbuffer, vertexbuffer.indexbuffer.buf, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(commandbuffer, vertexbuffer.indexsize, 1, 0, 0, 0);
}
