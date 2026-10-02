#include <cstdint>
#include "glfw_window.h"
#include "common.h"
#include <vk_gfx.h>
#include <cstring>
#include <random>

struct Particle {
	glm::vec2 position;
	glm::vec2 velocity;
	glm::vec4 color;
};

struct UBO {
	float deltaTime;
};

constexpr uint32_t PARTICLE_COUNT = 1000;
constexpr std::uint32_t WIDTH = 800;
constexpr std::uint32_t HEIGHT = 600;

static float lastFrameTime = 0.0f;

gfx::Buffer* createParticleBuffer(gfx::Device* device) {
	// Initialize particles
    std::default_random_engine rndEngine((unsigned)time(nullptr));
    std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);

    // Initial particle positions on a circle
    std::vector<Particle> particles(PARTICLE_COUNT);
    for (auto& particle : particles) {
        float r = 0.25f * sqrt(rndDist(rndEngine));
        float theta = rndDist(rndEngine) * 2 * 3.14159265358979323846;
        float x = r * cos(theta) * HEIGHT / WIDTH;
        float y = r * sin(theta);
        particle.position = glm::vec2(x, y);
        particle.velocity = glm::normalize(glm::vec2(x,y)) * 0.00025f;
        particle.color = glm::vec4(rndDist(rndEngine), rndDist(rndEngine), rndDist(rndEngine), 1.0f);
    }

	gfx::Buffer* particleBuffer = gfx::create_and_upload_buffer(device, particles.data(), {
		.size = particles.size() * sizeof(Particle),
		.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	});

	return particleBuffer;
}

void updateUniformBuffer(gfx::UniformBuffer* uniformBuffer,  const gfx::SwapchainFrame& frame) {
	UBO ubo{};
	ubo.deltaTime = lastFrameTime * 2.0f;

	memcpy(uniformBuffer->mappedMemory[frame.index], &ubo, sizeof(ubo));
}

int main()
{
	constexpr bool enableValidationLayers = true;

	std::string appName = "compute example";
	Window window = create_window(WIDTH, HEIGHT, appName);

 	gfx::VertexLayout vertexLayout = gfx::create_vertex_layout({{
		.stride = sizeof(Particle),
		.attributes = {
			{.format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(Particle, position)},
			{.format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Particle, color)},
		}
	}});

	constexpr uint32_t framesInFlight = 2;

	gfx::DeviceInit deviceInit = gfx::create_device({
		.appname = appName,
		.extensions = {},
		.queues = {gfx::QueueRequest{ .flags = gfx::QueueFlags::Graphics | gfx::QueueFlags::Present | gfx::QueueFlags::Compute }},
		.framesInFlight = framesInFlight,
		.windows = {
			{ .window = window.vkWindow, .swapchain = { .format = VK_FORMAT_B8G8R8A8_SRGB }}
		},
		.resourcePool = {
			.sizes = {
				{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = framesInFlight},
				{.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = framesInFlight * 2}
			},
			.max_sets = framesInFlight,
		},
		.enableValidation = enableValidationLayers
	});

	gfx::Device* device = deviceInit.device;

	// if () { // TODO: error handling
		// gfx::destroy_device(device);
		// close_window(window);
	// }
	gfx::Buffer* lastParticles = createParticleBuffer(device);
	gfx::Buffer* particles = gfx::create_buffer(device, {
		.size = lastParticles->size,
		.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	});

	gfx::UniformBuffer* ubo = gfx::create_uniform_buffer(device, sizeof(UBO));

	gfx::ResourceSetLayout* resourceLayout = gfx::create_resource_set_layout(device, {
		{.type = gfx::ResourceType::UniformBuffer, .stages = VK_SHADER_STAGE_COMPUTE_BIT},
		{.type = gfx::ResourceType::StorageBuffer, .stages = VK_SHADER_STAGE_COMPUTE_BIT},
		{.type = gfx::ResourceType::StorageBuffer, .stages = VK_SHADER_STAGE_COMPUTE_BIT},
	});

	std::vector<gfx::ResourceSet> resources = gfx::create_resource_sets(device, resourceLayout, {
		{.type = gfx::ResourceType::UniformBuffer, .uniformBuffer = ubo},
		{.type = gfx::ResourceType::StorageBuffer, .storageBuffer = lastParticles},
		{.type = gfx::ResourceType::StorageBuffer, .storageBuffer = particles},
	}, device->framesInFlight);

	gfx::Pipeline* gfxPipeline = gfx::create_graphics_pipeline(device, {
		.vertex_shader = load_shader("shaders/compute.vertex.spv"),
		.fragment_shader = load_shader("shaders/compute.fragment.spv"),
		.vertex_layout = &vertexLayout,
	});

	gfx::Pipeline* computePipeline = gfx::create_compute_pipeline(device, {
		.compute_shader = load_shader("shaders/compute.compute.spv"),
		.resource_set_layouts = { resourceLayout }
	});

	while (poll_window_events(window)) {
		const gfx::SwapchainFrame frame = gfx::acquire(device, window.vkWindow);
		updateUniformBuffer(ubo, frame);

		// // compute
		// gfx::CommandBuffer* commands = gfx::begin_commands(frame.window);
		// gfx::bind_pipeline(computePipeline, commands);
		// gfx::end_commands(commands);
		// gfx::submit(device, commands);

		// // TODO: sync
		// // drawing
		// commands = gfx::begin_commands(frame.window);
		// gfx::begin_render_pass(device, commands, &frame);
		// gfx::dispatch(commands, PARTICLE_COUNT / 256);
		// gfx::bind_pipeline(gfxPipeline, commands, frame.dynamicState);
		// gfx::draw(commands, {}, 3);
		// gfx::end_render_pass(commands);
		// gfx::end_commands(commands);
		// gfx::submit_and_present(device, frame.window, commands);
	}

	gfx::wait_idle(device);

	gfx::destroy_pipeline(device, computePipeline);
	gfx::destroy_pipeline(device, gfxPipeline);
	gfx::destroy_window(device, window.vkWindow);
	gfx::destroy_device(device);
	close_window(window);

	return EXIT_SUCCESS;
}
