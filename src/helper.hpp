#pragma once
#include <vulkan/vulkan.h>

#include <vector>
#include <string>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>

#include <iostream>

namespace helper
{
    VkPipelineShaderStageCreateInfo loadShader(VkDevice device, std::string fileName, VkShaderStageFlagBits stage);

    struct vertindex
    {
        std::vector<float> vertices;
        std::vector<unsigned int> indices;
    };

    vertindex processMesh(aiMesh* mesh, const aiScene* scene);
    void processNode(aiNode* node, const aiScene* scene, std::vector<vertindex>& data);
    std::vector<vertindex> readassimp(std::string file_path);

    float get_time(bool timestamp = false);
}

std::ostream& operator<<(std::ostream& os, const glm::vec3 vec);