#include "glfw_window.h"

Window create_window(std::uint32_t width, std::uint32_t height, const std::string& title) {
	glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	return Window {
		.window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr),
		.extensions = extensions
	};
}

bool poll_window_events(Window& window) {
	if (glfwWindowShouldClose(window.window))
		return false;

	glfwPollEvents();

	return true;
}

void close_window(Window& window)
{
	glfwDestroyWindow(window.window);
	glfwTerminate();

	window.window = nullptr;
}
