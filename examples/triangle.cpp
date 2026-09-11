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
		.window = &window.callbacks,
		.enableValidation = enableValidationLayers
	});

	gfx::Device* device = deviceInit.device;
	// if () { // TODO: error handling
		// gfx::destroy_device(device);
		// close_window(window);
	// }
	gfx::Pipeline* pipeline = gfx::create_graphics_pipeline(device, {
		.vertex_shader = load_shader("shaders/triangle.vert.spv"),
		.fragment_shader = load_shader("shaders/triangle.frag.spv"),
	});

	gfx::Semaphore imageAvailableSemaphore = gfx::create_semaphore(device);
	gfx::Semaphore renderFinishedSemaphore = gfx::create_semaphore(device);
	gfx::Fence inFlightFence = gfx::create_fence(device);

	while (poll_window_events(window)) {
		gfx::wait_for_fence(device, &inFlightFence);

		const gfx::SwapchainFrame frame = gfx::aquire(device, &imageAvailableSemaphore);
		gfx::reset_fence(device, &inFlightFence);
		gfx::CommandBuffer* commands = gfx::begin_commands(device);
		gfx::begin_render_pass(device, commands, { .colors = { .renderView = frame.imageView, .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR }});
		gfx::bind_pipeline(device, pipeline, commands);
		gfx::draw(commands, {}, 3);
		gfx::end_render_pass(commands);
		gfx::end_commands(commands);
		gfx::submit_and_present(device, commands, &inFlightFence, &imageAvailableSemaphore, &renderFinishedSemaphore);
	}

	gfx::wait_idle(device);

	gfx::destroy_semaphore(device, &imageAvailableSemaphore);
	gfx::destroy_semaphore(device, &renderFinishedSemaphore);
	gfx::destroy_fence(device, &inFlightFence);

	gfx::destroy_pipeline(device, pipeline);
	gfx::destroy_device(device);
	close_window(window);

	return EXIT_SUCCESS;
}
