#include "glfw_window.h"

static void glfw_error_callback(int error, const char* description)
{
    std::printf("GLFW Error %d: %s\n", error, description);
}

Window create_window(std::uint32_t width, std::uint32_t height, const std::string& title) {
    glfwSetErrorCallback(glfw_error_callback);
    glfwInitHint(GLFW_WAYLAND_LIBDECOR, GLFW_WAYLAND_DISABLE_LIBDECOR);

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    GLFWwindow* window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);

    if (!glfwVulkanSupported()) {
        std::printf( "GLFW: Vulkan Not Supported\n");
        abort();
    }

    // gfx::Window vk_window = gfx::create_window(title);

    // // create window surface
    // VkSurfaceKHR surface;
    // VkResult result = glfwCreateWindowSurface(instance, window, nullptr, &surface);
    // if(result != VK_SUCCESS) {
    //     std::printf("GLFW: Vulkan Not Supported\n");
    //     abort();
    // }

	return Window {
		.window = window,
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
