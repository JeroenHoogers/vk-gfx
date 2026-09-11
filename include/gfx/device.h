#pragma once
#include "window.h"
#include <cstdint>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>
#include "sync.h"

namespace gfx
{
	inline void vk_assert_impl(VkResult result, const char* file, int line) {
		if (result != VK_SUCCESS) {
			std::fprintf(stderr, "Vulkan error: %d at %s:%d\n", static_cast<int>(result), file, line);

			std::abort();
		}
	}

	// TODO: probably don't want this in the public API

	namespace detail
	{
		struct QueueFamilyIndices
		{
			std::uint32_t graphicsFamily;
			std::uint32_t presentFamily;

			std::uint32_t queueFamilyCount;
		};

		bool find_queue_families(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, QueueFamilyIndices& indices);
	} // namespace detail

	struct Swapchain;
	struct Pipeline;
	struct RenderPass;
	struct RenderTarget;
	struct CommandBuffer;

	struct Frame {
		CommandBuffer* commands;
		Fence fence;
		Semaphore imageAvailable;
	};

	struct Device
	{
		VkInstance instance;
		VkPhysicalDevice physicalDevice;
		VkDevice device;
		VkSurfaceKHR surface; // TODO: store in window?
		VkDebugUtilsMessengerEXT debugMessenger;
		VkQueue graphicsQueue = VK_NULL_HANDLE;
		VkQueue presentQueue = VK_NULL_HANDLE;
		VkCommandPool commandPool = VK_NULL_HANDLE;
		std::vector<Frame> frames = {};
		uint32_t currentFrame = 0;
		WindowCallbacks* window;
		Swapchain* swapchain = nullptr;
		RenderPass* renderPass = nullptr;
		RenderTarget* renderTarget = nullptr;
	};

	struct DeviceCreateParams
	{
		std::string appname = "vulkan app";
		std::vector<const char*> extensions{};
		std::vector<const char*> deviceExtensions{};
		std::vector<const char*> layers{};
		VkFormat swapchainFormat = VK_FORMAT_UNDEFINED; // TODO: move vulkan out of public API?
		WindowCallbacks* window = nullptr;
		bool enableValidation = false;
		uint32_t framesInFlight = 2;
	};

	struct DeviceInit
	{
		Device* device;
	};

	DeviceInit create_device(const DeviceCreateParams& init);

	void wait_idle(Device* device);

	void destroy_device(Device* device);
} // namespace gfx

#define VK_ASSERT(result) ::gfx::vk_assert_impl((result), __FILE__, __LINE__)
