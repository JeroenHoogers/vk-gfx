#pragma once
#include "window.h"
#include <cstdint>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include "resource.h" // required for ResourcePoolDesc
#include "swapchain.h" // required for SwapchainDesc
#include "detail/bitflags.h"
#include "render_pass.h" // required for RenderPassDesc

namespace gfx
{
	inline void vk_assert_impl(VkResult result, const char* file, int line)
	{
		if (result != VK_SUCCESS) {
			std::fprintf(stderr, "Vulkan error: %d: %s at  %s:%d\n", static_cast<int>(result), string_VkResult(result), file, line);

			std::abort();
		}
	}

	struct RenderPass;
	struct RenderTarget;
	struct CommandBuffer;
	struct Image;

	// consider moving queue logic to a separate file?
	struct Queue {
		VkQueue handle = VK_NULL_HANDLE;
		uint32_t familyIndex = VK_QUEUE_FAMILY_IGNORED;
		uint32_t queueIndex = 0;
	};

	enum class QueueFlags : uint8_t {
		None     = 0,
		Graphics = 1 << 0,
		Present  = 1 << 1,
		Compute  = 1 << 2,
		Transfer = 1 << 3,
	};

	template <>
	struct detail::enable_bitmask_operators<QueueFlags> : std::true_type {};

	struct QueueRequest {
		QueueFlags flags;
		bool preferDedicated = false;
	};

	struct Device
	{
		VkInstance instance;
		VkPhysicalDevice physicalDevice;
		VkDevice device;
		VkDebugUtilsMessengerEXT debugMessenger;
		// TODO: consider creating a list of queues instead matching the queue requests?
		Queue graphicsQueue = {};
		Queue presentQueue = {};
		Queue transferQueue = {};
		Queue computeQueue = {};
		VkCommandPool commandPool = VK_NULL_HANDLE;
		VkCommandPool transientPool = VK_NULL_HANDLE;
		ResourcePool* resourcePool = nullptr;
		VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
		bool enableDepth = false;
		uint32_t framesInFlight = 2;
		RenderPass* renderPass = nullptr;
	};

	struct WindowDesc {
		Window* window;
		SwapchainDesc swapchain;
	};

	struct DeviceCreateParams
	{
		std::string appname = "vulkan app";
		uint32_t apiVersion = VK_API_VERSION_1_0;
		std::vector<const char*> extensions{};
		std::vector<const char*> deviceExtensions{};
		std::vector<const char*> layers{};
		std::vector<QueueRequest> queues{ QueueRequest{ .flags = QueueFlags::Graphics | QueueFlags::Present }};
		uint32_t framesInFlight = 2;
		VkPhysicalDeviceFeatures features{};
		RenderPassDesc renderPass{};
		bool enableDepth = false;
		VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT; // 1 bit means no MSAA
		std::vector<WindowDesc> windows {};
		ResourcePoolDesc resourcePool {};
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
