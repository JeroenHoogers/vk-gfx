#pragma once

#include <GLFW/glfw3.h>
#include <cstdint>
#include <string>

struct Window
{
	GLFWwindow* window;
};

Window create_window(std::uint32_t width, std::uint32_t height, const std::string& title);
void destroy_window(Window& window);
