#pragma once

#include <GLFW/glfw3.h>
#include <cstdint>
#include <string>
#include <vector>

struct Window
{
	GLFWwindow* window;
	std::vector<const char*> extensions;
};

Window create_window(std::uint32_t width, std::uint32_t height, const std::string& title);


bool poll_window_events(Window& window);
void close_window(Window& window);
