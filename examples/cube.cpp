#include "common.h"
#include "glfw_window.h"
#include <cstdint>
#include <vk_gfx.h>

int main()
{
	constexpr std::uint32_t width = 800;
	constexpr std::uint32_t height = 600;
	constexpr bool enableValidationLayers = true;

	const std::vector<Vertex> vertices = {
		{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
		{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
		{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
		{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
	};

	const std::vector<uint16_t> indices = {
		0, 1, 2, 2, 3, 0
	};

	gfx::VertexLayout vertexLayout = gfx::create_vertex_layout({{
		.stride = sizeof(Vertex),
		.attributes = {
			{.format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(Vertex, pos)},
			{.format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, color)}
		}
	}});

	std::string appName = "textured cube example";
	Window window = create_window(width, height, appName);

	gfx::DeviceInit deviceInit = gfx::create_device({.appname = appName, .extensions = {}, .swapchainFormat = VK_FORMAT_B8G8R8A8_SRGB, .window = &window.callbacks, .enableValidation = enableValidationLayers});

	gfx::Device* device = deviceInit.device;
	// if () { // TODO: error handling
	// gfx::destroy_device(device);
	// close_window(window);
	// }

	gfx::Pipeline* pipeline = gfx::create_graphics_pipeline(device, {
		.vertex_shader = load_shader("shaders/shader.vert.spv"),
		.fragment_shader = load_shader("shaders/shader.frag.spv"),
		.vertex_layout = &vertexLayout
	});

	gfx::Mesh cube = gfx::create_mesh(device, {
		.vertices {
			.data = vertices.data(),
			.size = sizeof(Vertex) * vertices.size(),
			.stride = sizeof(Vertex)
		},
		.indices {
			.data = indices.data(),
			.size = sizeof(uint16_t) * indices.size(),
			.stride = sizeof(uint16_t)
		}
	});

	while (poll_window_events(window)) {
		const gfx::SwapchainFrame frame = gfx::aquire(device);
		gfx::CommandBuffer* commands = gfx::begin_commands(device);
		gfx::begin_render_pass(device, commands, {.colors = {.renderView = frame.imageView, .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR}});
		gfx::bind_pipeline(device, pipeline, commands);
		gfx::draw_indexed(commands, &cube, indices.size());
		gfx::end_render_pass(commands);
		gfx::end_commands(commands);
		gfx::submit_and_present(device, commands);
	}

	gfx::wait_idle(device);

	gfx::destroy_mesh(device, &cube);

	gfx::destroy_pipeline(device, pipeline);
	gfx::destroy_device(device);
	close_window(window);

	return EXIT_SUCCESS;
}
