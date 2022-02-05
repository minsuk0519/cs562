#include "shapes.hpp"

#include <iostream>
#include <fstream>
#include <chrono>

VkPipelineShaderStageCreateInfo helper::loadShader(VkDevice device, std::string fileName, VkShaderStageFlagBits stage)
{
	VkPipelineShaderStageCreateInfo shaderStage = {};
	shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStage.stage = stage;

	std::ifstream is(fileName, std::ios::binary | std::ios::in | std::ios::ate);

	if (!is.is_open())
	{
		std::cout << "failed to open file : " << fileName << std::endl;
	}

	size_t size = is.tellg();
	is.seekg(0, std::ios::beg);
	char* shaderCode = new char[size];
	is.read(shaderCode, size);
	is.close();

	VkShaderModuleCreateInfo moduleCreateInfo{};
	moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	moduleCreateInfo.codeSize = size;
	moduleCreateInfo.pCode = (uint32_t*)shaderCode;

	if (vkCreateShaderModule(device, &moduleCreateInfo, VK_NULL_HANDLE, &shaderStage.module))
	{
		std::cout << "failed to create shader module!" << std::endl;
	}

	delete[] shaderCode;

	shaderStage.pName = "main";

	return shaderStage;
}

helper::vertindex helper::processMesh(aiMesh* mesh, const aiScene* /*scene*/)
{
	helper::vertindex data;

	for (unsigned int i = 0; i < mesh->mNumVertices; i++)
	{
		data.vertices.push_back(mesh->mVertices[i].x);
		data.vertices.push_back(mesh->mVertices[i].y);
		data.vertices.push_back(mesh->mVertices[i].z);

		data.vertices.push_back(mesh->mNormals[i].x);
		data.vertices.push_back(mesh->mNormals[i].y);
		data.vertices.push_back(mesh->mNormals[i].z);

		if (mesh->HasTextureCoords(i))
		{
			data.vertices.push_back(mesh->mTextureCoords[i]->x);
			data.vertices.push_back(mesh->mTextureCoords[i]->y);
		}
		else
		{
			data.vertices.push_back(0.0f);
			data.vertices.push_back(0.0f);
		}
	}
	
	for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
	{
		auto face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; ++j)
		{
			data.indices.push_back(face.mIndices[j]);
		}
	}


	return data;
}

void helper::processNode(aiNode* node, const aiScene* scene, std::vector<vertindex>& data)
{
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		data.push_back(processMesh(mesh, scene));
	}

	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		processNode(node->mChildren[i], scene, data);
	}
}

std::vector<helper::vertindex> helper::readassimp(std::string file_path)
{
	Assimp::Importer import;
	const aiScene* scene = import.ReadFile(file_path.c_str(), aiProcess_Triangulate | aiProcess_GenSmoothNormals);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cout << "failed to load assimp file L " << import.GetErrorString() << std::endl;
		return {};
	}

	std::vector<helper::vertindex> data;

	processNode(scene->mRootNode, scene, data);

	return data;
}

float helper::get_time(bool timestamp)
{
	static auto prev = std::chrono::high_resolution_clock::now();
	static auto current = std::chrono::high_resolution_clock::now();

	current = std::chrono::high_resolution_clock::now();

	float result = (float)(std::chrono::duration<double, std::ratio<1, 1>>(current - prev).count());

	if (timestamp) prev = current;

	return result;
}

std::ostream& operator<<(std::ostream& os, const glm::vec3 vec)
{
	os << "(" << vec.x << ", " << vec.y << ", " << vec.z << ")";
	return os;
}
