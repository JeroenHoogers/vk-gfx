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
		// .model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
		.model = glm::mat4(1.0),
		.view = glm::lookAt(glm::vec3(-50.0f, 0.0f, -300.0f), glm::vec3(-50.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
		.proj = glm::perspective(glm::radians(45.0f), frame.extent.width / (float)frame.extent.height, 0.1f, 1000.0f)
	};
	// ubo.proj[1][1] *= -1;

	memcpy(uniformBuffer->mappedMemory[frame.index], &ubo, sizeof(ubo));
}

struct TextBuffer {
	gfx::Buffer* glyphInstanceBuffer;
	gfx::Buffer* indirectBuffer;
};

TextBuffer createTextBuffer(gfx::Device* device, const drafttype::HersheyFont& font, const drafttype::GPUFont& gpuFont, const std::string& text, glm::vec2 pos, drafttype::LayoutOptions opts) {
	auto glyphInstances = drafttype::layout(font, text, pos.x, pos.y, opts, true);
	printf("created instances: %lu\n", glyphInstances.size());

	// create indirect buffer
	std::vector<VkDrawIndexedIndirectCommand> commands;

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

	TextBuffer textBuffer {};

	VkDeviceSize instanceSize = glyphInstances.size() * sizeof(drafttype::ShapedGlyph);
	VkDeviceSize indirectSize = commands.size() * sizeof(VkDrawIndexedIndirectCommand);

	printf("created text buffer with: %lu commands (%lu) and %lu instances (%lu)\n", commands.size(), indirectSize, glyphInstances.size(), instanceSize);

	textBuffer.glyphInstanceBuffer = gfx::create_and_upload_buffer(device, glyphInstances.data(), {
		.size = instanceSize,
		.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	});

	textBuffer.indirectBuffer = gfx::create_and_upload_buffer(device, commands.data(), {
		.size = indirectSize,
		.usage = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
		.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	});

	return textBuffer;
}

int main()
{
	constexpr std::uint32_t width = 800;
	constexpr std::uint32_t height = 600;
	constexpr bool enableValidationLayers = true;
	std::string appName = "Instanced text example";
	Window window = create_window(width, height, appName);

	constexpr uint32_t framesInFlight = 2;

	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.fillModeNonSolid = VK_TRUE;
	deviceFeatures.multiDrawIndirect = VK_TRUE;
	deviceFeatures.wideLines = VK_TRUE;

	gfx::DeviceInit deviceInit = gfx::create_device({
		.appname = appName,
		.extensions = {},
		.framesInFlight = framesInFlight,
		.swapchainFormat = VK_FORMAT_B8G8R8A8_SRGB,
		.features = deviceFeatures,
		.enableDepth = false,
		.msaaSamples = VK_SAMPLE_COUNT_8_BIT,
		.windows = {&window.callbacks},
		.resourcePool = {
			.sizes = {
				{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = framesInFlight},
				{.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1}
			},
			.max_sets = framesInFlight + 1,
		},
		.enableValidation = enableValidationLayers,
	});

	const std::string FONT_PATH = "assets/hershey-fonts/timesg.jhf";
	drafttype::HersheyFont font(FONT_PATH);

	const auto gpuFont = font.generateGPUFont();

	std::string text = " !\"#$%&'()*\n"
		"+,-./012345\n"
		"6789:;<=>?@\n"
		"ABCDEFGHIJK\n"
		"LMNOPQRSTUV\n"
		"WXYZ[\\]^_`\n"
		"abcdefghijk\n"
		"lmnopqrstuv\n"
		"wxyz{|}~\x7F";

	// text = "aabbdas";

	drafttype::LayoutOptions opts = {
		.letterSpacing = 20.0f,
		.lineSpacing = 0.0f,
		.scale = 1.0f,
		.horizontalAlign = drafttype::HorizontalAlign::Center,
		// .verticalAlign = drafttype::VerticalAlign::Middle
	};

	gfx::Device* device = deviceInit.device;

	// if () { // TODO: error handling
	// gfx::destroy_device(device);
	// close_window(window);
	// }
	gfx::Window* mainWindow = device->windows[0];
	glfwSetWindowUserPointer(window.window, mainWindow);

	TextBuffer textBuffers = createTextBuffer(device, font, gpuFont, text, glm::vec2(0.0f, -100.0f), opts);

	gfx::VertexLayout vertexLayout = gfx::create_vertex_layout({{
		.stride = sizeof(drafttype::Vert),
		.attributes = {
			{.format = VK_FORMAT_R16G16_SINT, .offset = 0},
		}
	}});

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

	gfx::UniformBuffer* ubo = gfx::create_uniform_buffer(device, sizeof(UniformBufferObject));
	gfx::ResourceSetLayout* uboResourceLayout = gfx::create_resource_set_layout(device, {
		{.type = gfx::ResourceType::UniformBuffer, .stages = VK_SHADER_STAGE_VERTEX_BIT}
	});

	gfx::ResourceSetLayout* glyphInstancesResourceLayout = gfx::create_resource_set_layout(device, {
		{.type = gfx::ResourceType::StorageBuffer, .stages = VK_SHADER_STAGE_VERTEX_BIT}
	});

	std::vector<gfx::ResourceSet*> uboResources = gfx::create_resource_sets(device, uboResourceLayout, {
		{.type = gfx::ResourceType::UniformBuffer, .uniformBuffer = ubo}
	}, device->framesInFlight);

	gfx::ResourceSet* glyphInstancesResourceSet = gfx::create_resource_set(device, glyphInstancesResourceLayout, {
		{.type = gfx::ResourceType::StorageBuffer, .storageBuffer = textBuffers.glyphInstanceBuffer}
	});

	gfx::Pipeline* pipeline = gfx::create_graphics_pipeline(device, {
		.vertex_shader = load_shader("shaders/text.vert.spv"),
		.fragment_shader = load_shader("shaders/text.frag.spv"),
		.vertex_layout = &vertexLayout,
		.resource_set_layouts = {uboResourceLayout, glyphInstancesResourceLayout},
		// .rasterizer = {
		// 	.polygon_mode = VK_POLYGON_MODE_LINE,
		// },
		.multisampling = {
			.enable_alpha_to_coverage = VK_TRUE
		},
		.primitiveTopology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
		.dynamic_states = gfx::DynamicStateFlags::Viewport | gfx::DynamicStateFlags::Scissor | gfx::DynamicStateFlags::LineWidth,
	});

	while (poll_window_events(window)) {
		const gfx::SwapchainFrame frame = gfx::acquire(device, mainWindow);
		updateUniformBuffer(ubo, frame);
		gfx::CommandBuffer* commands = gfx::begin_commands(frame.window);
		gfx::begin_render_pass(device, commands, &frame);
		gfx::DynamicState dynamicState = frame.dynamicState;
		dynamicState.lineWidth = 2.0f;
		gfx::bind_pipeline(pipeline, commands, dynamicState);

		// TODO: create helper to bind more than 1 set at once?
		gfx::bind_resource_set(commands, pipeline, 0, uboResources[frame.index]);
		gfx::bind_resource_set(commands, pipeline, 1, glyphInstancesResourceSet);

		uint32_t count = textBuffers.indirectBuffer->size / sizeof(VkDrawIndexedIndirectCommand);
		gfx::draw_indirect(commands, &fontMesh, textBuffers.indirectBuffer, count);
		gfx::end_render_pass(commands);
		gfx::end_commands(commands);
		gfx::submit_and_present(device, frame.window, commands);
	}

	gfx::wait_idle(device);

	gfx::destroy_mesh(device, &fontMesh);

	gfx::destroy_pipeline(device, pipeline);
	gfx::destroy_resource_set_layouts(device, {uboResourceLayout, glyphInstancesResourceLayout});
	gfx::destroy_uniform_buffer(device, ubo);

	gfx::destroy_buffer(device, textBuffers.glyphInstanceBuffer);
	gfx::destroy_buffer(device, textBuffers.indirectBuffer);

	gfx::destroy_device(device);
	close_window(window);

	return EXIT_SUCCESS;
}
