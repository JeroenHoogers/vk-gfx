// Copyright(c) 2026, Jeroen Hoogers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#include <cstdint>
#include "glfw_window.h"
#include "common.h"
#include <vk_gfx.h>

int main() {
	#ifdef DEBUG
		constexpr bool enableValidationLayers = true;
	#else
		constexpr bool enableValidationLayers = false;
	#endif

	constexpr std::uint32_t width = 800;
	constexpr std::uint32_t height = 600;

	std::string appName = "triangle example";
	Window window = create_window(width, height, appName);

	gfx::Device* device = gfx::create_device({
		.appname = appName,
		.apiVersion = VK_API_VERSION_1_1, // slang compilation of a vertex shader using SV_VertexID doesn't compile to spirv_1_0 so we need to raise API version
		.deviceExtensions = { VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME }, // Required by SV_VertexID
		.windows = {
			{ .window = window.vkWindow, .swapchain = { .format = VK_FORMAT_B8G8R8A8_SRGB }}
		},
		.enableValidation = enableValidationLayers
	});

	gfx::Pipeline* pipeline = gfx::create_graphics_pipeline(device, {
		.vertex_shader = load_shader("shaders/triangle.vertex.spv"),
		.fragment_shader = load_shader("shaders/triangle.fragment.spv"),
	});

	while (poll_window_events(window)) {
		const gfx::SwapchainFrame frame = gfx::acquire(device, window.vkWindow);
		gfx::CommandBuffer commands = gfx::begin_commands(frame.window);
		gfx::begin_render_pass(device, commands, &frame);
		gfx::bind_pipeline(pipeline, commands, frame.dynamicState);
		gfx::draw(commands, {}, 3);
		gfx::end_render_pass(commands);
		gfx::end_commands(commands);
		gfx::submit_and_present(device, frame, commands);
	}

	gfx::wait_idle(device);

	gfx::destroy_pipeline(device, pipeline);
	gfx::destroy_window(device, window.vkWindow);
	gfx::destroy_device(device);
	close_window(window);

	return EXIT_SUCCESS;
}
