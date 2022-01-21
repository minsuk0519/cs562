#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <bitset>

class window
{
public:
	GLFWwindow* glfwWindow = nullptr;

	bool shouldclose = false;
	float dt = 0.0f;

	unsigned int windowWidth = 1200;
	unsigned int windowHeight = 800;

	std::bitset<GLFW_KEY_LAST> pressed;
	std::bitset<GLFW_KEY_LAST> triggered;

	std::bitset<GLFW_MOUSE_BUTTON_LAST> mousepressed;
	std::bitset<GLFW_MOUSE_BUTTON_LAST> mousetriggered;

	float mousemovex = 0.0f;
	float prex = 0.0f;
	float currentx = 0.0f;
	float mousemovey = 0.0f;
	float prey = 0.0f;
	float currenty = 0.0f;

	bool create_window();
	bool create_surface(VkInstance instance, VkSurfaceKHR& surface);

	bool update_window_frame();

	void close_window();
};

namespace callback
{
	void keyboard_callback(GLFWwindow* glfwwindow, int key, int scancode, int action, int mods);
	void mousebutton_callback(GLFWwindow* glfwwindow, int button, int action, int mods);
	void cursor_callback(GLFWwindow* glfwwindow, double x, double y);
}