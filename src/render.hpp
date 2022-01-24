#pragma once
#include <vulkan/vulkan.h>

#include <vector>
#include <string>

#include "buffer.hpp"

namespace pipeline
{
	struct shaderinput
	{
		std::string filepath;
		VkShaderStageFlagBits shaderflag;
	};

	VkPipelineInputAssemblyStateCreateInfo getInputAssemblyCreateInfo();
	VkPipelineRasterizationStateCreateInfo getRasterizationCreateInfo(VkCullModeFlags cullmode);
	VkPipelineColorBlendAttachmentState getColorBlendAttachment(VkBool32 blendenable);
	VkPipelineColorBlendStateCreateInfo getColorBlendCreateInfo(uint32_t blendcount, VkPipelineColorBlendAttachmentState* blenddata);
	VkPipelineMultisampleStateCreateInfo getMultisampleCreateInfo(VkSampleCountFlagBits samplecount);
	VkViewport getViewport(uint32_t width, uint32_t height);
	VkRect2D getScissor(uint32_t width, uint32_t height);
	VkPipelineViewportStateCreateInfo getViewportCreateInfo(VkViewport* viewport, VkRect2D* scissor);
	VkPipelineDepthStencilStateCreateInfo getDepthStencilCreateInfo(VkBool32 depthEnable, VkCompareOp depthcomp);
	VkVertexInputBindingDescription getVertexinputbindingDescription(uint32_t stride);
	VkPipelineVertexInputStateCreateInfo getVertexinputAttributeDescription(VkVertexInputBindingDescription* vertexinputbinding, std::vector<VkVertexInputAttributeDescription>& vertexinputdescription);

	bool create_pipieline(VkDevice device, VkGraphicsPipelineCreateInfo& pipelineCreateInfo, VkPipeline& pipeline, VkPipelineCache pipelinecache, std::vector<shaderinput> shaderinfos);
	bool create_pipelinelayout(VkDevice device, VkDescriptorSetLayout descriptorsetlayout, VkPipelineLayout& pipelinelayout);

	void closepipeline(VkDevice device, VkPipeline& pipeline);
	void close_pipelinelayout(VkDevice device, VkPipelineLayout& pipelinelayout);
};

namespace renderpass
{
	struct attachmentDesc
	{
		VkFormat format;
		uint32_t location;
		bool swapchain;
		bool depth;
	};

	bool create_renderpass(VkDevice device, VkRenderPass& renderpass, std::vector<attachmentDesc> attachmentDescriptions);
	bool create_framebuffer(VkDevice device, VkRenderPass renderpass, VkFramebuffer& framebuffer, uint32_t width, uint32_t height, std::vector<VkImageView> imageviews);

	void close_renderpass(VkDevice device, VkRenderPass& renderpass);
	void close_framebuffer(VkDevice device, VkFramebuffer& framebuffer);
};

namespace descriptor
{
	struct layoutform
	{
		VkDescriptorType type;
		VkShaderStageFlags stage;
		uint32_t binding;
	};
	struct descriptordata
	{
		VkDescriptorType type;
		uint32_t binding;
		VkDescriptorBufferInfo buf;
		VkDescriptorImageInfo img;
	};

	bool create_descriptorpool(VkDevice device, VkDescriptorPool& descriptorpool, std::vector<VkDescriptorPoolSize> descriptorpoolsizes);
	bool create_descriptorset_layout(VkDevice device, VkDescriptorSetLayout& descriptorsetlayout, VkDescriptorSet& descriptorset, VkDescriptorPool descriptorpool, std::vector<layoutform> layoutforms);
	bool write_descriptorset(VkDevice device, VkDescriptorSet descriptorset, std::vector<descriptordata> descriptors);

	void close_descriptorpool(VkDevice device, VkDescriptorPool& descriptorpool);
	void close_descriptorset_layout(VkDevice device, VkDescriptorSetLayout& descriptorsetlayout);
};

namespace render
{
	void draw(VkCommandBuffer commandbuffer, VertexBuffer vertexbuffer);
}