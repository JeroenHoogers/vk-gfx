#pragma once

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <cstdint>
#include <string>
#include <gfx/window.h>


struct Window
{
	GLFWwindow* glfwWindow;
	gfx::Window* vkWindow;
};

Window create_window(std::uint32_t width, std::uint32_t height, const std::string& title);
bool poll_window_events(Window& window);
void close_window(Window& window);
