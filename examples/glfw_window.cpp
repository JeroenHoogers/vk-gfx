#include "glfw_window.h"
#include <vk_gfx.h>

static void glfw_error_callback(int error, const char* description) {
	std::printf("GLFW Error %d: %s\n", error, description);
};

static VkResult glfw_get_required_instance_extensions(std::uint32_t* count, const char** names, void*) {
	std::uint32_t glfw_count;
	const char** glfw_names = glfwGetRequiredInstanceExtensions(&glfw_count);

	if (!glfw_names) {
		return VK_ERROR_INITIALIZATION_FAILED;
	}

	// get size only
	if (!names) {
		*count = glfw_count;
		return VK_SUCCESS;
	}

	if (*count < glfw_count) {
		*count = glfw_count;
		return VK_INCOMPLETE;
	}

	for (uint32_t i = 0; i < glfw_count; ++i) {
		names[i] = glfw_names[i];
	}

	*count = glfw_count;
	return VK_SUCCESS;
};

static VkResult glfw_create_surface(VkInstance instance, VkSurfaceKHR* surface, void* data) {
	GLFWwindow* window = (GLFWwindow*)data;
	return glfwCreateWindowSurface(instance, window, nullptr, surface);
};

static void glfw_get_framebuffer_size(uint32_t* width, uint32_t* height, void* data) {
	GLFWwindow* window = (GLFWwindow*)data;
	int w, h;
	glfwGetFramebufferSize(window, &w, &h);
	*width = static_cast<uint32_t>(w);
	*height = static_cast<uint32_t>(h);
};

static void glfw_resize_callback(GLFWwindow* window, int width, int height) {
	std::printf("GLFW resized %d, %d\n", width, height);
	gfx::Device* device = reinterpret_cast<gfx::Device*>(glfwGetWindowUserPointer(window));
	device->swapchain->resized = true;
};

Window create_window(std::uint32_t width, std::uint32_t height, const std::string& title) {
	glfwSetErrorCallback(glfw_error_callback);
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	// glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	GLFWwindow* window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
	glfwSetFramebufferSizeCallback(window, glfw_resize_callback);

	if (!glfwVulkanSupported()) {
		std::printf("GLFW: Vulkan Not Supported\n");
		abort();
	}

	gfx::WindowCallbacks windowCallbacks{
		.get_required_instance_extensions = glfw_get_required_instance_extensions,
		.get_framebuffer_size = glfw_get_framebuffer_size,
		.create_surface = glfw_create_surface,
		.user_data = window
	};

	return Window{
		.window = window,
		.callbacks = windowCallbacks
	};
}

bool poll_window_events(Window& window) {
	if (glfwWindowShouldClose(window.window))
		return false;

	// pauze while minimized
	int width = 0, height = 0;
	glfwGetFramebufferSize(window.window, &width, &height);
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(window.window, &width, &height);
		glfwWaitEvents();
	}

	glfwPollEvents();

	return true;
}

void close_window(Window& window) {
	glfwDestroyWindow(window.window);
	glfwTerminate();

	window.window = nullptr;
}
