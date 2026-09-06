#pragma once
#include "window.h"
#include <cstdint>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

namespace gfx
{
	inline void vk_assert_impl(VkResult result, const char* file, int line) {
		if (result != VK_SUCCESS) {
			std::fprintf(stderr, "Vulkan error: %d at %s:%d\n", static_cast<int>(result), file, line);

			std::abort();
		}
	}

	struct QueueFamilyIndices
	{
		std::uint32_t graphicsFamily;
		std::uint32_t presentFamily;

		std::uint32_t queueFamilyCount;
	};

	struct Device
	{
		VkInstance instance;
		VkPhysicalDevice physicalDevice;
		VkDevice device;
		VkSurfaceKHR surface; // TODO: store in window?
		VkDebugUtilsMessengerEXT debugMessenger;
		VkQueue graphicsQueue;
		VkQueue presentQueue;
	};

	struct DeviceCreateParams
	{
		std::string appname = "vulkan app";
		std::vector<const char*> extensions{};
		std::vector<const char*> deviceExtensions{};
		std::vector<const char*> layers{};
		WindowCallbacks* window = nullptr;
		bool enableValidation = false;
	};

	struct DeviceInit
	{
		Device* device;
	};

	DeviceInit create_device(DeviceCreateParams init);

	void destroy_device(Device* device);
} // namespace gfx

#define VK_ASSERT(result) ::gfx::vk_assert_impl((result), __FILE__, __LINE__)
