#pragma once
#include <vector>
#include <vulkan/vulkan.h>

namespace gfx
{
	struct Device;

	namespace detail
	{
		struct SwapChainSupportDetails
		{
			VkSurfaceCapabilitiesKHR capabilities;
			std::vector<VkSurfaceFormatKHR> formats;
			std::vector<VkPresentModeKHR> presentModes;
		};

		SwapChainSupportDetails query_swapchain_support(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

		VkPresentModeKHR choose_swap_present_mode(const std::vector<VkPresentModeKHR>& availableModes);
	} // namespace detail

	struct Swapchain
	{
		VkSwapchainKHR swapchain;
		std::vector<VkImage> images;
	};

	struct SwapchainFrame
	{
	};

	[[nodiscard]] Swapchain* create_swapchain(Device* device, VkFormat swapchainFormat);
	[[nodiscard]] SwapchainFrame* aquire(Device* device);

} // namespace gfx
