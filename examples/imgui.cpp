// Copyright(c) 2026, Jeroen Hoogers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#include <cstdint>
#include "glfw_window.h"
#include "common.h"
#include <vk_gfx.h>
#include <imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>

static void check_vk_result(VkResult err)
{
    if (err == 0)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
}

void init_imgui(gfx::Device* device, GLFWwindow* window) {
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForVulkan(window, true);
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = device->instance;
	init_info.PhysicalDevice = device->physicalDevice;
	init_info.Device = device->device;
	init_info.QueueFamily = device->graphicsQueue.familyIndex;
	init_info.Queue = device->graphicsQueue.handle;
	init_info.PipelineCache = VK_NULL_HANDLE;
	init_info.DescriptorPoolSize = IMGUI_IMPL_VULKAN_MINIMUM_SAMPLED_IMAGE_POOL_SIZE,
	init_info.DescriptorPool = VK_NULL_HANDLE;
	init_info.MinImageCount = 2;
	init_info.ImageCount = 2;
	init_info.Allocator = nullptr;
	init_info.PipelineInfoMain.RenderPass = device->renderPass->renderPass;
	init_info.PipelineInfoMain.Subpass = 0;
	init_info.PipelineInfoMain.MSAASamples = device->msaaSamples;
	init_info.CheckVkResultFn = check_vk_result;
	ImGui_ImplVulkan_Init(&init_info);
}

int main() {
	#ifdef DEBUG
		constexpr bool enableValidationLayers = true;
	#else
		constexpr bool enableValidationLayers = false;
	#endif

	constexpr std::uint32_t width = 800;
	constexpr std::uint32_t height = 600;

	std::string appName = "imgui example";
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

	init_imgui(device, window.glfwWindow);

	gfx::Pipeline* pipeline = gfx::create_graphics_pipeline(device, {
		.vertexShader = load_shader("shaders/triangle.vertex.spv"),
		.fragmentShader = load_shader("shaders/triangle.fragment.spv")
	});

	while (poll_window_events(window)) {
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGui::ShowDemoWindow();

		const gfx::SwapchainFrame frame = gfx::acquire(device, window.vkWindow);
		gfx::CommandBuffer commands = gfx::begin_commands(frame.window);
		gfx::begin_render_pass(device, commands, &frame);
		gfx::bind_pipeline(pipeline, commands, frame.dynamicState);
		gfx::draw(commands, {}, 3);

		ImGui::Render();
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commands);

		gfx::end_render_pass(commands);
		gfx::end_commands(commands);
		gfx::submit_and_present(device, frame, commands);
	}

	gfx::wait_idle(device);

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	gfx::destroy_pipeline(device, pipeline);
	gfx::destroy_window(device, window.vkWindow);
	gfx::destroy_device(device);
	close_window(window);

	return EXIT_SUCCESS;
}
