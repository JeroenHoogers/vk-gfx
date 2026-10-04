#include "common.h"
#include "glfw_window.h"
#include <chrono>
#include <cstdint>
#include <cstring>
#include <glm/gtc/matrix_transform.hpp>
#include <vk_gfx.h>

void updateUniformBuffer(gfx::UniformBuffer* uniformBuffer, const gfx::SwapchainFrame& frame)
{
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	UniformBufferObject ubo{
		.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
		.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
		.proj = glm::perspective(glm::radians(45.0f), frame.extent.width / (float)frame.extent.height, 0.1f, 10.0f)
	};
	ubo.proj[1][1] *= -1;

	memcpy(uniformBuffer->mappedMemory[frame.index], &ubo, sizeof(ubo));
}

struct Material {
	glm::vec3 tint;
};

int main()
{
	constexpr std::uint32_t width = 800;
	constexpr std::uint32_t height = 600;
	constexpr bool enableValidationLayers = true;

	// const std::vector<Vertex> vertices = {
	// 	{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
	// 	{{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
	// 	{{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
	// 	{{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},

	// 	{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
	// 	{{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
	// 	{{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
	// 	{{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
	// };

	// const std::vector<uint16_t> indices = {
	// 	0, 1, 2, 2, 3, 0,
	// 	4, 5, 6, 6, 7, 4
	// };

	const std::string MODEL_PATH = "assets/models/viking_room.obj";
	const std::string TEXTURE_PATH = "assets/textures/viking_room.png";

	gfx::VertexLayout vertexLayout = gfx::create_vertex_layout({{
		.stride = sizeof(Vertex),
		.attributes = {
			{.format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, pos)},
			{.format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, color)},
			{.format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(Vertex, uv)},
		}
	}});

	std::string appName = "textured cube example";
	Window window = create_window(width, height, appName);

	constexpr uint32_t framesInFlight = 2;

	VkPhysicalDeviceFeatures features = {};
	features.samplerAnisotropy = VK_TRUE;

	gfx::DeviceInit deviceInit = gfx::create_device({
		.appname = appName,
		.extensions = {},
		.framesInFlight = framesInFlight,
		.features = features,
		.enableDepth = true,
		.msaaSamples = VK_SAMPLE_COUNT_4_BIT,
		.windows = {
			{ .window = window.vkWindow, .swapchain = { .format = VK_FORMAT_B8G8R8A8_SRGB }}
		},
		.resourcePool = {
			.sizes = {
				{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = static_cast<uint32_t>(10)},
				{.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = static_cast<uint32_t>(10)}},
			.max_sets = framesInFlight + 1,
		},
		.enableValidation = enableValidationLayers,
	});

	gfx::Device* device = deviceInit.device;

	// if () { // TODO: error handling
	// gfx::destroy_device(device);
	// close_window(window);
	// }

	gfx::Texture* texture = load_image(device, TEXTURE_PATH);
	gfx::Mesh model = load_model(device, MODEL_PATH);

	// gfx::Texture* texture = load_image(device, "../assets/textures/texture.jpg");
	gfx::UniformBuffer* ubo = gfx::create_uniform_buffer(device, sizeof(UniformBufferObject));
	gfx::ResourceSetLayout* uboResourceLayout = gfx::create_resource_set_layout(device, {
		{.type = gfx::ResourceType::UniformBuffer, .stages = VK_SHADER_STAGE_VERTEX_BIT}
	});

	gfx::ResourceSetLayout* materialResourceLayout = gfx::create_resource_set_layout(device, {
		{.type = gfx::ResourceType::CombinedImageSampler, .stages = VK_SHADER_STAGE_FRAGMENT_BIT}
	});

	std::vector<gfx::ResourceSet> uboResources = gfx::create_resource_sets(device, uboResourceLayout, {
		{.type = gfx::ResourceType::UniformBuffer, .uniformBuffer = ubo}
	}, device->framesInFlight);

	gfx::ResourceSet materialResourceSet = gfx::create_resource_set(device, materialResourceLayout, {
		{.type = gfx::ResourceType::CombinedImageSampler, .texture = texture}
	});

	gfx::Pipeline* pipeline = gfx::create_graphics_pipeline(device, {
		.vertex_shader = load_shader("shaders/textured.vertex.spv"),
		.fragment_shader = load_shader("shaders/textured.fragment.spv"),
		.vertex_layout = &vertexLayout,
		.resource_set_layouts = {uboResourceLayout, materialResourceLayout}, // allow create directly in pipeline?
		.push_constants = {
			{ .size = sizeof(Material), .stages = VK_SHADER_STAGE_FRAGMENT_BIT}
		},
		.rasterizer = {
			.front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE
		}
	});

	// gfx::Mesh cube = gfx::create_mesh(device, {
	// 	.vertices {
	// 		.data = vertices.data(),
	// 		.size = sizeof(Vertex) * vertices.size(),
	// 		.stride = sizeof(Vertex)
	// 	},
	// 	.indices {
	// 		.data = indices.data(),
	// 		.size = sizeof(uint16_t) * indices.size(),
	// 		.stride = sizeof(uint16_t)
	// 	}
	// });

	Material mat{
		.tint = { 1.0f, 1.0f, 1.0f }
	};

	while (poll_window_events(window)) {
		const gfx::SwapchainFrame frame = gfx::acquire(device, window.vkWindow);
		updateUniformBuffer(ubo, frame);
		gfx::CommandBuffer commands = gfx::begin_commands(frame.window);
		gfx::begin_render_pass(device, commands, &frame);
		gfx::bind_pipeline(pipeline, commands, frame.dynamicState);
		gfx::bind_resource_sets(commands, pipeline, { uboResources[frame.index], materialResourceSet });
		gfx::push_constants(commands, pipeline, 0, &mat);
		gfx::draw_indexed(commands, &model, model.indexCount);
		gfx::end_render_pass(commands);
		gfx::end_commands(commands);
		gfx::submit_and_present(device, frame, commands);
	}

	gfx::wait_idle(device);

	gfx::destroy_mesh(device, &model);

	gfx::destroy_pipeline(device, pipeline);
	gfx::destroy_resource_set_layouts(device, {uboResourceLayout, materialResourceLayout});
	gfx::destroy_uniform_buffer(device, ubo);
	gfx::destroy_texture(device, texture);
	gfx::destroy_window(device, window.vkWindow);
	gfx::destroy_device(device);
	close_window(window);

	return EXIT_SUCCESS;
}
