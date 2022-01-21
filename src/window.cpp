#include "window.hpp"
#include "helper.hpp"

#include <iostream>

const std::string title = "CS562 Project, fps : ";

bool window::create_window()
{
    if (!glfwInit())
    {
        std::cout << "failed to initialize glfw" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    glfwWindow = glfwCreateWindow(windowWidth, windowHeight, title.c_str(), nullptr, nullptr);

    glfwSetWindowUserPointer(glfwWindow, this);

    if (glfwWindow == nullptr)
    {
        std::cout << "failed to create window" << std::endl;
        return false;
    }

    glfwSetKeyCallback(glfwWindow, callback::keyboard_callback);
    glfwSetMouseButtonCallback(glfwWindow, callback::mousebutton_callback);
    glfwSetCursorPosCallback(glfwWindow, callback::cursor_callback);

    double x, y;
    glfwGetCursorPos(glfwWindow, &x, &y);
    prex = static_cast<float>(x);
    prey = static_cast<float>(y);
    currentx = prex;
    currenty = prey;

    return true;
}

bool window::create_surface(VkInstance instance, VkSurfaceKHR& surface)
{
    if (glfwCreateWindowSurface(instance, glfwWindow, VK_NULL_HANDLE, &surface) != VK_SUCCESS)
    {
        std::cout << "failed to create surface!" << std::endl;
        return false;
    }

    return true;
}

bool window::update_window_frame()
{
    glfwPollEvents();
    glfwSwapBuffers(glfwWindow);

    static uint32_t frameCount = 0;
    dt = helper::get_time(true);
    static float timestamp = 0.0f;
    timestamp += dt;
    ++frameCount;

    if (timestamp > 1.0f)
    {
        uint32_t lastFPS = static_cast<uint32_t>((float)frameCount * (timestamp));

        timestamp = 0.0f;
        frameCount = 0;

        std::string newtitle = title + std::to_string(lastFPS);

        glfwSetWindowTitle(glfwWindow, newtitle.c_str());
    }

    mousemovex = currentx - prex;
    mousemovey = currenty - prey;

    prex = currentx;
    prey = currenty;

    triggered.reset();

    return (!glfwWindowShouldClose(glfwWindow) && !shouldclose);
}

void window::close_window()
{
    glfwDestroyWindow(glfwWindow);
}

void callback::keyboard_callback(GLFWwindow* glfwwindow, int key, int /*scancode*/, int action, int /*mods*/)
{
    window* windowPtr = static_cast<window*>(glfwGetWindowUserPointer(glfwwindow));

    if (action == GLFW_PRESS)
    {
        windowPtr->pressed[key] = true;
        windowPtr->triggered[key] = true;
    }
    else if (action == GLFW_RELEASE)
    {
        windowPtr->pressed[key] = false;
    }
}

void callback::mousebutton_callback(GLFWwindow* glfwwindow, int button, int action, int /*mods*/)
{
    window* windowPtr = static_cast<window*>(glfwGetWindowUserPointer(glfwwindow));

    if (action == GLFW_PRESS)
    {
        windowPtr->mousepressed[button] = true;
        windowPtr->mousetriggered[button] = true;
    }
    else if (action == GLFW_RELEASE)
    {
        windowPtr->mousepressed[button] = false;
    }
}

void callback::cursor_callback(GLFWwindow* glfwwindow, double x, double y)
{
    window* windowPtr = static_cast<window*>(glfwGetWindowUserPointer(glfwwindow));

    windowPtr->currentx = static_cast<float>(x);
    windowPtr->currenty = static_cast<float>(y);
}
