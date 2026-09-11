#pragma once
#include <vector>
#include <vulkan/vulkan.h>

namespace gfx
{
	struct Device;
	struct Semaphore;

	struct SwapchainFrame
	{
		VkImage image;
		VkImageView imageView;
	};

	struct Swapchain
	{
		VkSwapchainKHR swapchain;
		VkExtent2D extent;
		VkFormat format;
		uint32_t imageIndex = 0;
		std::vector<SwapchainFrame> frames;
	};

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

		void create_swapchain_image_views(const std::vector<VkImage>& images);

	} // namespace detail


	[[nodiscard]] Swapchain* create_swapchain(Device* device, VkFormat swapchainFormat);
	void destroy_swapchain(Device* device, Swapchain* swapchain);
	[[nodiscard]] SwapchainFrame aquire(Device* device, Semaphore* semaphore);

} // namespace gfx
