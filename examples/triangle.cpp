#include <cstdint>
#include "glfw_window.h"
#include "common.h"
#include <vk_gfx.h>

int main() {
	constexpr std::uint32_t width = 800;
	constexpr std::uint32_t height = 600;
	constexpr bool enableValidationLayers = true;

	std::string appName = "triangle example";
	Window window = create_window(width, height, appName);

	gfx::DeviceInit deviceInit = gfx::create_device({
		.appname = appName,
		.extensions = {},
		.swapchainFormat = VK_FORMAT_B8G8R8A8_SRGB,
		.windows = {&window.callbacks},
		.enableValidation = enableValidationLayers
	});

	gfx::Device* device = deviceInit.device;
	gfx::Window* mainWindow = device->windows[0];
	glfwSetWindowUserPointer(window.window, mainWindow); // TODO: get window

	// if () { // TODO: error handling
		// gfx::destroy_device(device);
		// close_window(window);
	// }
	gfx::Pipeline* pipeline = gfx::create_graphics_pipeline(device, {
		.vertex_shader = load_shader("shaders/triangle.vert.spv"),
		.fragment_shader = load_shader("shaders/triangle.frag.spv"),
	});

	while (poll_window_events(window)) {
		const gfx::SwapchainFrame frame = gfx::acquire(device, mainWindow);
		gfx::CommandBuffer* commands = gfx::begin_commands(frame.window);
		gfx::begin_render_pass(device, commands, &frame);
		gfx::bind_pipeline(pipeline, commands, frame.dynamicState);
		gfx::draw(commands, {}, 3);
		gfx::end_render_pass(commands);
		gfx::end_commands(commands);
		gfx::submit_and_present(device, frame.window, commands);
	}

	gfx::wait_idle(device);

	gfx::destroy_pipeline(device, pipeline);
	gfx::destroy_device(device);
	close_window(window);

	return EXIT_SUCCESS;
}
