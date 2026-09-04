#include "glfw_window.h"

Window create_window(std::uint32_t width, std::uint32_t height, const std::string& title) {
	glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	Window window {};
	window.window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
	return window;
}

void destroy_window(Window& window)
{
	glfwDestroyWindow(window.window);
	glfwTerminate();

	window.window = nullptr;
}
