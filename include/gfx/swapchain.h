#pragma once
#include <vector>
#include <vulkan/vulkan.h>
#include "dynamic_state.h"
#include "types.h"

namespace gfx
{
	struct Device;
	struct Window;

	struct SwapchainImage {
		VkImage image;
		VkImageView imageView;

		VkSemaphore renderFinished;
	};

	struct SwapchainFrame
	{
		SwapchainImage image;
		Window* window;
		DynamicState dynamicState;
		VkFramebuffer frameBuffer;
		Semaphore renderFinished;
		VkExtent2D extent;
		// CommandBuffer* commandBuffer; // TODO: could be added here
		uint32_t index = 0;
		uint32_t swapImageIndex = 0;
	};

	struct Swapchain
	{
		bool resized = false;
		VkSwapchainKHR swapchain;
		VkExtent2D extent;
		VkFormat format;
		VkCompositeAlphaFlagBitsKHR compositeAlpha;
		uint32_t imageIndex = 0;
		std::vector<SwapchainImage> images;
	};

	struct SwapchainDesc
	{
		VkFormat format;
		VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
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

	[[nodiscard]] Swapchain* create_swapchain(Device* device, Window* window, const SwapchainDesc& params);
	void recreate_swapchain(Device* device, Window* window);
	void destroy_swapchain(Device* device, Swapchain* swapchain);
	[[nodiscard]] SwapchainFrame acquire(Device* device, Window* window);

} // namespace gfx
