#include <cstdint>
#include "glfw_window.h"
#include "common.h"
#include <vk_gfx.h>
#include <cstring>
#include <random>
#include <algorithm>

struct Particle {
	glm::vec2 position;
	glm::vec2 velocity;
	glm::vec4 color;
};

struct UBO {
	float deltaTime;
};

constexpr uint32_t PARTICLE_COUNT = 8192;
constexpr std::uint32_t WIDTH = 800;
constexpr std::uint32_t HEIGHT = 600;

constexpr uint32_t FRAMES_IN_FLIGHT = 2;

float lastFrameTime = 0.0f;
double lastTime = 0.0f;

gfx::MultiBuffer createParticleBuffer(gfx::Device* device) {
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

	gfx::MultiBuffer particleBuffers = gfx::create_and_upload_buffers(device, particles.data(), {
		.size = particles.size() * sizeof(Particle),
		.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	}, FRAMES_IN_FLIGHT);

	return particleBuffers;
}

void updateUniformBuffer(gfx::UniformBuffer* uniformBuffer, uint32_t frameIndex) {
	UBO ubo{};
	ubo.deltaTime = lastFrameTime * 2.0f;

	memcpy(uniformBuffer->mappedMemory[frameIndex], &ubo, sizeof(ubo));
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

	VkPhysicalDeviceFeatures features{};
	features.largePoints = VK_TRUE;

	gfx::DeviceInit deviceInit = gfx::create_device({
		.appname = appName,
		.extensions = {},
		.queues = {gfx::QueueRequest{ .flags = gfx::QueueFlags::Graphics | gfx::QueueFlags::Present | gfx::QueueFlags::Compute }},
		.framesInFlight = FRAMES_IN_FLIGHT,
		.features = features,
		.windows = {
			{ .window = window.vkWindow, .swapchain = { .format = VK_FORMAT_B8G8R8A8_SRGB }}
		},
		.resourcePool = {
			.sizes = {
				{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = FRAMES_IN_FLIGHT},
				{.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = FRAMES_IN_FLIGHT * 2}
			},
			.max_sets = FRAMES_IN_FLIGHT,
		},
		.enableValidation = enableValidationLayers
	});

	gfx::Device* device = deviceInit.device;

	// if () { // TODO: error handling
		// gfx::destroy_device(device);
		// close_window(window);
	// }

	gfx::MultiBuffer particles = createParticleBuffer(device);
	gfx::MultiBuffer lastParticles = particles;
	std::rotate(lastParticles.buffers.begin(), lastParticles.buffers.end() - 1, lastParticles.buffers.end()); // rotate buffers right by 1 to get last frame

	gfx::UniformBuffer* ubo = gfx::create_uniform_buffer(device, sizeof(UBO));

	gfx::ResourceSetLayout* resourceLayout = gfx::create_resource_set_layout(device, {
		{.type = gfx::ResourceType::UniformBuffer, .stages = VK_SHADER_STAGE_COMPUTE_BIT},
		{.type = gfx::ResourceType::StorageBuffer, .stages = VK_SHADER_STAGE_COMPUTE_BIT}, // last (in)
		{.type = gfx::ResourceType::StorageBuffer, .stages = VK_SHADER_STAGE_COMPUTE_BIT}, // curr (out)
	});

	std::vector<gfx::ResourceSet> resources = gfx::create_resource_sets(device, resourceLayout, {
		{.type = gfx::ResourceType::UniformBuffer, .uniformBuffer = ubo},
		{.type = gfx::ResourceType::StorageBuffers, .storageBuffers = &lastParticles}, // last (in)
		{.type = gfx::ResourceType::StorageBuffers, .storageBuffers = &particles}, // curr (out)
	}, device->framesInFlight);

	gfx::Pipeline* computePipeline = gfx::create_compute_pipeline(device, {
		.compute_shader = load_shader("shaders/compute.compute.spv"),
		.resource_set_layouts = { resourceLayout }
	});

	gfx::Pipeline* gfxPipeline = gfx::create_graphics_pipeline(device, {
		.vertex_shader = load_shader("shaders/compute.vertex.spv"),
		.fragment_shader = load_shader("shaders/compute.fragment.spv"),
		.vertex_layout = &vertexLayout,
		.inputAssembly {
			.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST
		},
		.blending = {
			.enable_blend = VK_TRUE
		}
	});

	// create compute sync objects and commandbuffers
	std::vector<gfx::Semaphore> computeFinishedSemaphores(FRAMES_IN_FLIGHT);
	std::vector<gfx::Fence> computeInFlightFences(FRAMES_IN_FLIGHT);

	for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
		computeFinishedSemaphores[i] = gfx::create_semaphore(device);
		computeInFlightFences[i] = gfx::create_fence(device);
	}

	std::vector<gfx::CommandBuffer> computeCommandBuffers = gfx::create_command_buffers(device, FRAMES_IN_FLIGHT);

	while (poll_window_events(window)) {
		uint32_t frameIndex = window.vkWindow->currentFrame;
		gfx::wait_for_fence(device, computeInFlightFences[frameIndex]);
		updateUniformBuffer(ubo, frameIndex);
		gfx::reset_fence(device, computeInFlightFences[frameIndex]);

		// compute
		gfx::CommandBuffer commands = gfx::begin_commands(computeCommandBuffers[frameIndex]);
		gfx::bind_pipeline(computePipeline, commands);
		gfx::bind_resource_set(commands, computePipeline, 0, resources[frameIndex]);
		gfx::dispatch(commands, PARTICLE_COUNT / 256);
		gfx::end_commands(commands);
		gfx::submit(commands, {
			.queue = device->computeQueue,
			.signalSemaphores = {computeFinishedSemaphores[frameIndex]},
			.completedFence = computeInFlightFences[frameIndex]
		});

		// TODO: sync
		// drawing
		// gfx::CommandBuffer* commands = gfx::begin_commands(frame.window);
		const gfx::SwapchainFrame frame = gfx::acquire(device, window.vkWindow);
		commands = gfx::begin_commands(frame.window);
		gfx::bind_pipeline(gfxPipeline, commands, frame.dynamicState);
		gfx::begin_render_pass(device, commands, &frame);

		gfx::Mesh mesh {
			.vertexBuffer = particles.buffers[frame.index],
			.vertexCount = PARTICLE_COUNT
		};

		gfx::draw(commands, &mesh, PARTICLE_COUNT);
		gfx::end_render_pass(commands);
		gfx::end_commands(commands);
		gfx::Frame& frameInFlight = frame.window->frames[frame.index];
		gfx::submit_and_present(device, frame, commands, {
			.queue = device->graphicsQueue,
			.waitSemaphores = { computeFinishedSemaphores[frame.index], frameInFlight.imageAvailable },
			.waitStages = { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT },
			.signalSemaphores = { frame.renderFinished },
			.completedFence = frameInFlight.inFlightFence
		});

        double currentTime = glfwGetTime();
        lastFrameTime = (currentTime - lastTime) * 1000.0;
        lastTime = currentTime;
	}

	gfx::wait_idle(device);

	gfx::destroy_uniform_buffer(device, ubo);
	gfx::destroy_buffer(device, particles);

	gfx::destroy_resource_set_layout(device, resourceLayout);
	gfx::destroy_pipeline(device, computePipeline);
	gfx::destroy_pipeline(device, gfxPipeline);

	for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
		gfx::destroy_semaphore(device, computeFinishedSemaphores[i]);
		gfx::destroy_fence(device, computeInFlightFences[i]);
	}

	gfx::destroy_window(device, window.vkWindow);
	gfx::destroy_device(device);
	close_window(window);

	return EXIT_SUCCESS;
}
