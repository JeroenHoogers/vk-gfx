#include "common.h"
#include "glfw_window.h"
#include <cstdint>
#include <vk_gfx.h>
#include <chrono>
#include <cstring>
#include <glm/gtc/matrix_transform.hpp>

void updateUniformBuffer(gfx::Device* device, gfx::UniformBuffer* uniformBuffer) {
	// TODO: pass from frame?
	uint32_t imageIndex = device->currentFrame;
	uint32_t width = device->swapchain->extent.width;
	uint32_t height = device->swapchain->extent.height;

    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	UniformBufferObject ubo{};
	ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), width / (float)height, 0.1f, 10.0f);
	ubo.proj[1][1] *= -1;

	memcpy(uniformBuffer->mappedMemory[imageIndex], &ubo, sizeof(ubo));
}

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

	gfx::UniformBuffer* ubo = gfx::create_uniform_buffer(device, {
		.size = sizeof(UniformBufferObject),
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT
	});

	// if () { // TODO: error handling
	// gfx::destroy_device(device);
	// close_window(window);
	// }

	gfx::Pipeline* pipeline = gfx::create_graphics_pipeline(device, {
		.vertex_shader = load_shader("shaders/shader.vert.spv"),
		.fragment_shader = load_shader("shaders/shader.frag.spv"),
		.vertex_layout = &vertexLayout,
		.uniform_buffers = {ubo},
		.rasterizer = { .front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE }
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
		(void)frame;
		updateUniformBuffer(device, ubo);
		gfx::CommandBuffer* commands = gfx::begin_commands(device);
		gfx::begin_render_pass(device, commands); // TODO: fix
		gfx::bind_pipeline(device, pipeline, commands);
		gfx::bind_uniform_buffer(device, pipeline, commands, ubo);
		gfx::draw_indexed(commands, &cube, indices.size());
		gfx::end_render_pass(commands);
		gfx::end_commands(commands);
		gfx::submit_and_present(device, commands);
	}

	gfx::wait_idle(device);

	gfx::destroy_mesh(device, &cube);

	gfx::destroy_pipeline(device, pipeline);
	gfx::destroy_uniform_buffer(device, ubo);
	gfx::destroy_device(device);
	close_window(window);

	return EXIT_SUCCESS;
}
