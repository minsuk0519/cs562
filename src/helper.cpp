#include "shapes.hpp"

#include <fstream>

VkPipelineShaderStageCreateInfo helper::loadShader(VkDevice device, std::string fileName, VkShaderStageFlagBits stage)
{
	VkPipelineShaderStageCreateInfo shaderStage = {};
	shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStage.stage = stage;

	std::ifstream is(fileName, std::ios::binary | std::ios::in | std::ios::ate);

	if (is.is_open())
	{
		size_t size = is.tellg();
		is.seekg(0, std::ios::beg);
		char* shaderCode = new char[size];
		is.read(shaderCode, size);
		is.close();

		assert(size > 0);

		VkShaderModule shaderModule;
		VkShaderModuleCreateInfo moduleCreateInfo{};
		moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		moduleCreateInfo.codeSize = size;
		moduleCreateInfo.pCode = (uint32_t*)shaderCode;

		VK_CHECK_RESULT(vkCreateShaderModule(device, &moduleCreateInfo, NULL, &shaderModule));

		delete[] shaderCode;

		return shaderModule;
	}
	else
	{
		std::cerr << "Error: Could not open shader file \"" << fileName << "\"" << "\n";
		return VK_NULL_HANDLE;
	}

	shaderStage.module = vks::tools::loadShader(fileName.c_str(), device);
	shaderStage.pName = "main";

	assert(shaderStage.module != VK_NULL_HANDLE);

	shaderModules.push_back(shaderStage.module);
	return shaderStage;
}

helper::vertindex helper::processMesh(aiMesh* mesh, const aiScene* scene)
{
	return vertindex();
}

void helper::processNode(aiNode* node, const aiScene* scene, std::vector<vertindex>& data)
{
}

std::vector<helper::vertindex> helper::readassimp(std::string file_path)
{
	return std::vector<vertindex>();
}

float helper::get_time(bool timestamp)
{
	return 0.0f;
}
