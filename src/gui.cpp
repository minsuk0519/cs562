#include "gui.hpp"

#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_vulkan.h"

bool gui::init(VkInstance instance, VkQueue graphicsqueue, uint32_t imagecount, GLFWwindow* window, VkRenderPass renderpass, device* devicePtr)
{
    VkDescriptorPoolSize pool_sizes[] =
    {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
    pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;
    if (vkCreateDescriptorPool(devicePtr->vulkanDevice, &pool_info, nullptr, &guiDescriptorPool))
    {
        std::cout << "Failed to create decriptorpool for imgui!" << std::endl;
        return false;
    }

    ImGui_ImplVulkan_InitInfo guivulkanInitInfo{};

    guivulkanInitInfo.Device = devicePtr->vulkanDevice;
    guivulkanInitInfo.Instance = instance;
    guivulkanInitInfo.PhysicalDevice = devicePtr->vulkanPhysicalDevice;
    guivulkanInitInfo.Queue = graphicsqueue;
    guivulkanInitInfo.DescriptorPool = guiDescriptorPool;
    guivulkanInitInfo.MinImageCount = 2;
    guivulkanInitInfo.ImageCount = imagecount;

    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForVulkan(window, true);
    ImGui_ImplVulkan_Init(&guivulkanInitInfo, renderpass);

    VkCommandBuffer commandBuffer;
    devicePtr->create_single_commandbuffer_begin(commandBuffer);

    ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);

    devicePtr->end_commandbuffer_submit(graphicsqueue, commandBuffer);

    ImGui_ImplVulkan_DestroyFontUploadObjects();

    return true;
}

void gui::pre_upate()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void gui::render(VkCommandBuffer commandbuffer)
{
    ImGui::Render();

    ImDrawData* draw_data = ImGui::GetDrawData();

    ImGui_ImplVulkan_RenderDrawData(draw_data, commandbuffer);
}

void gui::close(VkDevice device)
{
    ImGui_ImplVulkan_Shutdown();

    vkDestroyDescriptorPool(device, guiDescriptorPool, VK_NULL_HANDLE);
}
