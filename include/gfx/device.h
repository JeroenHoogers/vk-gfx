#pragma once
#include "window.h"
#include <cstdint>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>
#include "descriptor_set.h"

namespace gfx
{
	inline void vk_assert_impl(VkResult result, const char* file, int line) {
		if (result != VK_SUCCESS) {
			std::fprintf(stderr, "Vulkan error: %d at %s:%d\n", static_cast<int>(result), file, line);

			std::abort();
		}
	}

	struct RenderPass;
	struct RenderTarget;
	struct CommandBuffer;
	struct ResourcePool;
	struct ResourcePoolDesc;
	struct Image;

	struct Queue {
		VkQueue handle = VK_NULL_HANDLE;
		uint32_t familyIndex = VK_QUEUE_FAMILY_IGNORED;
		uint32_t queueIndex = 0;
	};

	struct Device
	{
		VkInstance instance;
		VkPhysicalDevice physicalDevice;
		VkDevice device;
		VkDebugUtilsMessengerEXT debugMessenger;
		Queue graphicsQueue = {};
		Queue presentQueue = {};
		Queue transferQueue = {};
		VkCommandPool commandPool = VK_NULL_HANDLE;
		VkCommandPool transientPool = VK_NULL_HANDLE;
		ResourcePool* resourcePool = nullptr;
		VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
		bool enableDepth = false;
		uint32_t framesInFlight = 2;
		std::vector<Window*> windows = {}; // TODO: store windows on user side?
		RenderPass* renderPass = nullptr;
	};

	struct DeviceCreateParams
	{
		std::string appname = "vulkan app";
		std::vector<const char*> extensions{};
		std::vector<const char*> deviceExtensions{};
		std::vector<const char*> layers{};
		uint32_t framesInFlight = 2;
		VkFormat swapchainFormat = VK_FORMAT_UNDEFINED; // TODO: move vulkan out of public API?
		bool enableDepth = false;
		VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT; // 1 bit means no MSAA
		std::vector<WindowCallbacks*> windows {};
		ResourcePoolDesc resourcePool {};
		// WindowCallbacks* windowCallbacks = nullptr;
		bool enableValidation = false;
	};

	struct DeviceInit
	{
		Device* device;
		// TODO: add error handling
	};

	DeviceInit create_device(const DeviceCreateParams& init);

	void wait_idle(Device* device);

	void destroy_device(Device* device);
} // namespace gfx

#define VK_ASSERT(result) ::gfx::vk_assert_impl((result), __FILE__, __LINE__)
