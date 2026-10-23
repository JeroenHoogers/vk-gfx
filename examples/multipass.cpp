// Copyright(c) 2026, Jeroen Hoogers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#include <cstdint>
#include "glfw_window.h"
#include "common.h"
#include <vk_gfx.h>
#include <chrono>

struct Material {
	glm::vec3 tint;
};

void updateUniformBuffer(gfx::UniformBuffer uniformBuffer, const gfx::SwapchainFrame& frame)
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

	memcpy(uniformBuffer.mapped[frame.index], &ubo, sizeof(ubo));
}

int main() {
	#ifdef DEBUG
		constexpr bool enableValidationLayers = true;
	#else
		constexpr bool enableValidationLayers = false;
	#endif

	constexpr std::uint32_t width = 800;
	constexpr std::uint32_t height = 600;

	std::string appName = "multipass rendering example";
	Window window = create_window(width, height, appName);

	constexpr uint32_t framesInFlight = 2;

	gfx::Device* device = gfx::create_device({
		.appname = appName,
		.apiVersion = VK_API_VERSION_1_1, // slang compilation of a vertex shader using SV_VertexID doesn't compile to spirv_1_0 so we need to raise API version
		.deviceExtensions = { VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME }, // Required by SV_VertexID
		.framesInFlight = framesInFlight,
		.renderPass = {
			.enableDepth = true
		},
		.windows = {
			{ .window = window.vkWindow, .swapchain = { .format = VK_FORMAT_B8G8R8A8_SRGB }}
		},
		.resourcePool = {
			.sizes = {
				{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = static_cast<uint32_t>(10)},
				{.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, .descriptorCount = static_cast<uint32_t>(framesInFlight)},
				{.type = VK_DESCRIPTOR_TYPE_SAMPLER, .descriptorCount = static_cast<uint32_t>(framesInFlight)}},
			.max_sets = framesInFlight + 1,
		},
		.enableValidation = enableValidationLayers
	});

	gfx::Mesh mesh = create_cube(device);

	gfx::RenderPass* offscreenPass = gfx::create_render_pass(device, {
		.colors = { gfx::Attachment{
			.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
 			.clearColor = {{{ 0.025f, 0.025f, 0.025f, 1.0f}}} // add dark gray background to see cube
		}}
	});

	gfx::RenderTarget* offscreenTarget = gfx::create_render_target(device, offscreenPass, {
		.extent = { 256, 256 },
		.colorFormat = VK_FORMAT_B8G8R8A8_SRGB,
		.colorUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
	});

	gfx::ResourceSetLayout frameResourceLayout = gfx::create_resource_set_layout(device, {
		{.binding = 0, .type = gfx::ResourceType::UniformBuffer, .stages = VK_SHADER_STAGE_VERTEX_BIT},
		{.binding = 1, .type = gfx::ResourceType::SampledImages, .stages = VK_SHADER_STAGE_FRAGMENT_BIT},
		{.binding = 2, .type = gfx::ResourceType::Sampler, .stages = VK_SHADER_STAGE_FRAGMENT_BIT},
	});

	gfx::Sampler textureSampler = gfx::create_image_sampler(device);
	gfx::UniformBuffer ubo = gfx::create_uniform_buffer(device, sizeof(UniformBufferObject));

	std::vector<gfx::ResourceSet> frameResources = gfx::create_resource_sets(device, frameResourceLayout, {
		{ .uniformBuffer = &ubo },
		{ .images = &offscreenTarget->colorImages },
		{ .sampler = &textureSampler }
	}, device->framesInFlight);

	gfx::Pipeline* offscreenPipeline = gfx::create_graphics_pipeline(device, {
		.vertexSpirv = load_shader("shaders/triangle.vertex.spv"),
		.fragmentSpirv = load_shader("shaders/triangle.fragment.spv"),
		.renderPass = offscreenPass,
		.viewport = { .extent = offscreenTarget->extent },
		.rasterizer = { .frontFace = VK_FRONT_FACE_CLOCKWISE },
		.dynamicStates = gfx::DynamicStateFlags::None
	});

	gfx::VertexLayout vertexLayout = gfx::create_vertex_layout({{
		.stride = sizeof(Vertex),
		.attributes = {
			{.format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, pos)},
			{.format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, color)},
			{.format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(Vertex, uv)},
		}
	}});

	gfx::Pipeline* pipeline = gfx::create_graphics_pipeline(device, {
		.vertexSpirv = load_shader("shaders/multipass.vertex.spv"),
		.fragmentSpirv = load_shader("shaders/multipass.fragment.spv"),
		.layout = gfx::PipelineLayoutDesc{
			.resourceSetLayouts = { frameResourceLayout },
		},
		.vertexLayout = &vertexLayout,
	});

	while (poll_window_events(window)) {
		const gfx::SwapchainFrame frame = gfx::acquire(device, window.vkWindow);
		updateUniformBuffer(ubo, frame);
		gfx::CommandBuffer commands = gfx::begin_commands(frame.window);

		// pass 1: render the triangle into an offscreen image.
		gfx::begin_render_pass(offscreenPass, commands, frame.index, offscreenTarget);
		gfx::bind_pipeline(offscreenPipeline, commands);
		gfx::draw(commands, {}, 3);
		gfx::end_render_pass(commands);

		// transition output attachment to be readable in frag shader for next pass
		gfx::transition_image(commands, offscreenTarget->colorImages[frame.index],
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);

		// pass 2: display the rendered triangle as a texture on a cube
		gfx::begin_render_pass(device->renderPass, commands, frame);
		gfx::bind_pipeline(pipeline, commands, frame.dynamicState);
		gfx::bind_resource_sets(commands, pipeline, { frameResources[frame.index] });
		gfx::draw_indexed(commands, &mesh, mesh.indexCount);
		gfx::end_render_pass(commands);
		gfx::end_commands(commands);
		gfx::submit_and_present(device, frame, commands);
	}

	gfx::wait_idle(device);

	gfx::destroy_mesh(device, mesh);
	gfx::destroy_sampler(device, textureSampler);

	gfx::destroy_pipeline(device, offscreenPipeline);
	gfx::destroy_pipeline(device, pipeline);
	gfx::destroy_resource_set_layout(device, frameResourceLayout);
	gfx::destroy_uniform_buffer(device, ubo);

	gfx::destroy_render_target(device, offscreenTarget);
	gfx::destroy_render_pass(device, offscreenPass);

	gfx::destroy_window(device, window.vkWindow);
	gfx::destroy_device(device);
	close_window(window);

	return EXIT_SUCCESS;
}
