// Copyright(c) 2026, Jeroen Hoogers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#include "common.h"
#include "glfw_window.h"
#include <chrono>
#include <cstdint>
#include <cstring>
#include <glm/gtc/matrix_transform.hpp>
#include <vk_gfx.h>
#include <draft-type.h>

void updateUniformBuffer(gfx::UniformBuffer* uniformBuffer, const gfx::SwapchainFrame& frame)
{
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	UniformBufferObject ubo{
		.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
		.view = glm::lookAt(glm::vec3(0.0f, 50.0f, -300.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
		.proj = glm::perspective(glm::radians(45.0f), frame.extent.width / (float)frame.extent.height, 0.1f, 1000.0f)
	};
	ubo.proj[1][1] *= -1;

	memcpy(uniformBuffer->mappedMemory[frame.index], &ubo, sizeof(ubo));
}

struct TextBuffer {
	gfx::Buffer glyphInstanceBuffer;
	gfx::Buffer indirectBuffer;
};

TextBuffer createTextBuffer(gfx::Device* device, const drafttype::HersheyFont& font, const drafttype::GPUFont& gpuFont, const std::string& text, glm::vec2 pos, drafttype::LayoutOptions opts) {
	auto glyphInstances = drafttype::layout(font, text, pos.x, pos.y, opts, true);

	// create indirect buffer
	std::vector<VkDrawIndexedIndirectCommand> commands;

	// generate one command per unique glyph in the text, recurring glyphs are instanced.
	uint32_t instanceCount = 1;
	uint32_t firstInstance = 0;
	for (uint32_t i = 0; i < glyphInstances.size(); i++) {
		uint32_t glyphIndex = glyphInstances[i].glyphIndex;
		if(i + 1 >= glyphInstances.size() || glyphInstances[i + 1].glyphIndex != glyphIndex) {
			commands.push_back({
				.indexCount = gpuFont.glyphs[glyphIndex].indexCount,
				.instanceCount = instanceCount,
				.firstIndex = gpuFont.glyphs[glyphIndex].indexOffset,
				.vertexOffset = 0,
				.firstInstance = firstInstance
			});
			instanceCount = 0;
			firstInstance = i+1;
		}
		instanceCount++;
	}

	TextBuffer textBuffer {
		.glyphInstanceBuffer = gfx::create_and_upload_buffer(device, glyphInstances.data(), {
			.size = glyphInstances.size() * sizeof(drafttype::ShapedGlyph),
			.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		}),
		.indirectBuffer = gfx::create_and_upload_buffer(device, commands.data(), {
			.size = commands.size() * sizeof(VkDrawIndexedIndirectCommand),
			.usage = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
			.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		})
	};

	printf("created text buffer with: %lu commands (%lu) and %lu instances (%lu)\n", commands.size(), textBuffer.indirectBuffer.size, glyphInstances.size(), textBuffer.glyphInstanceBuffer.size);

	return textBuffer;
}

int main()
{
	#ifdef DEBUG
		constexpr bool enableValidationLayers = true;
	#else
		constexpr bool enableValidationLayers = false;
	#endif

	constexpr std::uint32_t width = 800;
	constexpr std::uint32_t height = 600;

	std::string appName = "Instanced Text example";
	Window window = create_window(width, height, appName);

	constexpr uint32_t framesInFlight = 2;

	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.fillModeNonSolid = VK_TRUE;
	deviceFeatures.multiDrawIndirect = VK_TRUE;
	deviceFeatures.wideLines = VK_TRUE;
	deviceFeatures.drawIndirectFirstInstance = VK_TRUE;

	gfx::Device* device = gfx::create_device({
		.appname = appName,
		.apiVersion = VK_API_VERSION_1_1, // slang compilation of a vertex shader using SV_InstanceID doesn't compile to spirv_1_0 so we need to raise API version
		.deviceExtensions = { VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME }, // required by SV_InstanceID
		.framesInFlight = framesInFlight,
		.features = deviceFeatures,
		.msaaSamples = VK_SAMPLE_COUNT_8_BIT,
		.windows = {
			{ .window = window.vkWindow, .swapchain = { .format = VK_FORMAT_B8G8R8A8_SRGB }}
		},
		.resourcePool = {
			.sizes = {
				{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = framesInFlight},
				{.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1}
			},
			.max_sets = framesInFlight + 1,
		},
		.enableValidation = enableValidationLayers,
	});

	const std::string FONT_PATH = "assets/hershey-fonts/rowmans.jhf";
	drafttype::HersheyFont font(FONT_PATH);

	std::string text = " !\"#$%&'()*\n"
		"+,-./012345\n"
		"6789:;<=>?@\n"
		"ABCDEFGHIJK\n"
		"LMNOPQRSTUV\n"
		"WXYZ[\\]^_`\n"
		"abcdefghijk\n"
		"lmnopqrstuv\n"
		"wxyz{|}~\x7F";

	drafttype::LayoutOptions textLayoutOpts = {
		.letterSpacing = 5.0f,
		.scale = 0.5f,
		.horizontalAlign = drafttype::HorizontalAlign::Center,
		.verticalAlign = drafttype::VerticalAlign::Middle
	};

	constexpr float AXIS_SIZE = 10.0f;
	const std::vector<Vertex> vertices = {
		{{-AXIS_SIZE, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0, 0}},
		{{ AXIS_SIZE, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0, 0}},
		{{0.0f, -AXIS_SIZE, 0.0f}, {0.0f, 1.0f, 0.0f}, {0, 0}},
		{{0.0f,  AXIS_SIZE, 0.0f}, {0.0f, 1.0f, 0.0f}, {0, 0}},
		{{0.0f, 0.0f, -AXIS_SIZE}, {0.0f, 0.0f, 1.0f}, {0, 0}},
		{{0.0f, 0.0f,  AXIS_SIZE}, {0.0f, 0.0f, 1.0f}, {0, 0}},
	};

	const std::vector<uint16_t> indices = {
		0, 1, 2, 3, 4, 5
	};

	gfx::Mesh axesMesh = gfx::create_mesh(device, {
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

	gfx::VertexLayout axesVertexLayout = gfx::create_vertex_layout({{
		.stride = sizeof(Vertex),
		.attributes = {
			{.format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, pos)},
			{.format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, color)}
		}
	}});

	gfx::VertexLayout textVertexLayout = gfx::create_vertex_layout({{
		.stride = sizeof(drafttype::Vert),
		.attributes = {
			{.format = VK_FORMAT_R16G16_SINT, .offset = 0},
		}
	}});

	const auto gpuFont = font.generateGPUFont();
	gfx::Mesh fontMesh = gfx::create_mesh(device, {
		.vertices {
			.data = gpuFont.vertices.data(),
			.size = sizeof(drafttype::Vert) * gpuFont.vertices.size(),
			.stride = sizeof(drafttype::Vert)
		},
		.indices {
			.data = gpuFont.indices.data(),
			.size = sizeof(uint32_t) * gpuFont.indices.size(),
			.stride = sizeof(uint32_t)
		}
	});

	gfx::ResourceSetLayout uboResourceLayout = gfx::create_resource_set_layout(device, {
		{.type = gfx::ResourceType::UniformBuffer, .stages = VK_SHADER_STAGE_VERTEX_BIT}
	});

	gfx::ResourceSetLayout glyphInstancesResourceLayout = gfx::create_resource_set_layout(device, {
		{.type = gfx::ResourceType::StorageBuffer, .stages = VK_SHADER_STAGE_VERTEX_BIT}
	});

	gfx::Pipeline* textPipeline = gfx::create_graphics_pipeline(device, {
		.vertexShader = load_shader("shaders/text.vertex.spv"),
		.fragmentShader = load_shader("shaders/text.fragment.spv"),
		.layout = gfx::PipelineLayoutDesc{
			.resourceSetLayouts = {uboResourceLayout, glyphInstancesResourceLayout}
		},
		.vertexLayout = &textVertexLayout,
		.multisampling = {
			.enableAlphaToCoverage = VK_TRUE
		},
		.inputAssembly{
			.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST
		},
		.dynamicStates = gfx::DynamicStateFlags::Viewport | gfx::DynamicStateFlags::Scissor | gfx::DynamicStateFlags::LineWidth,
	});

	gfx::UniformBuffer* ubo = gfx::create_uniform_buffer(device, sizeof(UniformBufferObject));
	std::vector<gfx::ResourceSet> uboResources = gfx::create_resource_sets(device, uboResourceLayout, {
		{.type = gfx::ResourceType::UniformBuffer, .uniformBuffer = ubo}
	}, device->framesInFlight);

	const auto bounds = drafttype::measure(font, text, textLayoutOpts);
	TextBuffer textBuffers = createTextBuffer(device, font, gpuFont, text, glm::vec2(0.0f, -(bounds.bottom + bounds.top) * 0.5f), textLayoutOpts);
	gfx::ResourceSet glyphInstancesResourceSet = gfx::create_resource_set(device, glyphInstancesResourceLayout, {
		{.type = gfx::ResourceType::StorageBuffer, .storageBuffer = &textBuffers.glyphInstanceBuffer}
	});

	gfx::Pipeline* linePipeline = gfx::create_graphics_pipeline(device,
	{
		.vertexShader = load_shader("shaders/shader.vertex.spv"),
		.fragmentShader = load_shader("shaders/shader.fragment.spv"),
		.layout = gfx::PipelineLayoutDesc{
			.resourceSetLayouts = {uboResourceLayout}
		},
		.vertexLayout = &axesVertexLayout,
		.multisampling = {
			.enable_alpha_to_coverage = VK_TRUE
		},
		.inputAssembly{
 			.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST
		},
		.dynamicStates = gfx::DynamicStateFlags::Viewport | gfx::DynamicStateFlags::Scissor | gfx::DynamicStateFlags::LineWidth,
	});

	while (poll_window_events(window)) {
		const gfx::SwapchainFrame frame = gfx::acquire(device, window.vkWindow);
		updateUniformBuffer(ubo, frame);
		gfx::CommandBuffer commands = gfx::begin_commands(frame.window);
		gfx::begin_render_pass(device, commands, &frame);

		// draw text
		gfx::bind_pipeline(textPipeline, commands, frame.dynamicState);
		gfx::bind_resource_sets(commands, textPipeline, { uboResources[frame.index], glyphInstancesResourceSet });

		uint32_t count = textBuffers.indirectBuffer.size / sizeof(VkDrawIndexedIndirectCommand);
		gfx::draw_indexed_indirect(commands, &fontMesh, textBuffers.indirectBuffer, count, 0);

		// draw axes
		gfx::DynamicState dynamicState = frame.dynamicState;
		dynamicState.lineWidth = 2.0f;
		gfx::bind_pipeline(linePipeline, commands, dynamicState);

		gfx::bind_resource_set(commands, linePipeline, 0, uboResources[frame.index]);
		gfx::draw_indexed(commands, &axesMesh, axesMesh.indexCount);

		gfx::end_render_pass(commands);
		gfx::end_commands(commands);
		gfx::submit_and_present(device, frame, commands);
	}

	gfx::wait_idle(device);

	gfx::destroy_mesh(device, &fontMesh);
	gfx::destroy_mesh(device, &axesMesh);

	gfx::destroy_pipeline(device, textPipeline);
	gfx::destroy_pipeline(device, linePipeline);

	gfx::destroy_resource_set_layouts(device, {uboResourceLayout, glyphInstancesResourceLayout});
	gfx::destroy_uniform_buffer(device, ubo);

	gfx::destroy_buffer(device, textBuffers.glyphInstanceBuffer);
	gfx::destroy_buffer(device, textBuffers.indirectBuffer);

	gfx::destroy_window(device, window.vkWindow);
	gfx::destroy_device(device);
	close_window(window);

	return EXIT_SUCCESS;
}
