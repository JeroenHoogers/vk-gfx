#include "gfx/swapchain.h"
#include "gfx/device.h"
#include <algorithm>
#include <limits>

namespace gfx
{
	namespace detail
	{
		SwapChainSupportDetails query_swapchain_support(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
			SwapChainSupportDetails details{};

			// capabilities
			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

			// formats
			uint32_t formatCount;
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);

			if (formatCount != 0) {
				details.formats.resize(formatCount);
				vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data());
			}

			// present modes
			uint32_t presentModeCount;
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);

			if (presentModeCount != 0) {
				details.presentModes.resize(presentModeCount);
				vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, details.presentModes.data());
			}

			return details;
		}

		VkPresentModeKHR choose_swap_present_mode(const std::vector<VkPresentModeKHR>& availableModes) {
			for (const auto& mode : availableModes) {
				if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
					return mode;
				}
			}

			return VK_PRESENT_MODE_FIFO_KHR;
		}

		VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height) {
			if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
				return capabilities.currentExtent;
			} else {
				VkExtent2D actualExtent = {
					static_cast<uint32_t>(width),
					static_cast<uint32_t>(height)
				};

				actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
				actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

				return actualExtent;
			}
		}

		VkSurfaceFormatKHR choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& availableFormats, VkFormat desiredFormat) {
			for (const auto& availableFormat : availableFormats) {
				if (availableFormat.format == desiredFormat && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
					return availableFormat;
				}
			}

			return availableFormats[0];
		}

		std::vector<VkImageView> create_swap_image_views(Device* device, const std::vector<VkImage>& images, VkFormat imageFormat) {
			std::vector<VkImageView> imageViews(images.size());

			for (size_t i = 0; i < images.size(); i++) {
				VkImageViewCreateInfo createInfo{
					.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.image = images[i],
					.viewType = VK_IMAGE_VIEW_TYPE_2D,
					.format = imageFormat,
					.components = {
						.r = VK_COMPONENT_SWIZZLE_IDENTITY,
						.g = VK_COMPONENT_SWIZZLE_IDENTITY,
						.b = VK_COMPONENT_SWIZZLE_IDENTITY,
						.a = VK_COMPONENT_SWIZZLE_IDENTITY
					},
					.subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1}
				};

				VkResult result = vkCreateImageView(device->device, &createInfo, nullptr, &imageViews[i]);
				VK_ASSERT(result);
			}
			return imageViews;
		}

		void destroy_swapchain(Device* device) {
			for (auto& frame : device->swapchain->frames) {
				vkDestroyImageView(device->device, frame.imageView, nullptr);
			}

			vkDestroySwapchainKHR(device->device, device->swapchain->swapchain, nullptr);

			delete device->swapchain;
			device->swapchain = nullptr;
		}

	} // namespace detail

	Swapchain* create_swapchain(Device* device, VkFormat desiredFormat) {
		auto swapchainSupport = detail::query_swapchain_support(device->physicalDevice, device->surface);
		VkPresentModeKHR presentMode = detail::choose_swap_present_mode(swapchainSupport.presentModes);
		VkExtent2D extent = detail::choose_swap_extent(swapchainSupport.capabilities, 0, 0); // TODO: get width / height
		VkSurfaceFormatKHR surfaceFormat = detail::choose_swap_surface_format(swapchainSupport.formats, desiredFormat);

		printf("extent %d, %d\n", extent.width, extent.height);

		uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
		if (swapchainSupport.capabilities.maxImageCount > 0 && imageCount > swapchainSupport.capabilities.maxImageCount) {
			imageCount = swapchainSupport.capabilities.maxImageCount;
		}
		printf("Swapchain image count: %d\n", imageCount);

		VkSwapchainCreateInfoKHR createInfo{
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.pNext = nullptr,
			.flags = 0,
			.surface = device->surface,
			.minImageCount = imageCount,
			.imageFormat = surfaceFormat.format,
			.imageColorSpace = surfaceFormat.colorSpace,
			.imageExtent = extent,
			.imageArrayLayers = 1,
			.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = 0,
			.pQueueFamilyIndices = nullptr,
			.preTransform = swapchainSupport.capabilities.currentTransform,
			.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			.presentMode = presentMode,
			.clipped = VK_TRUE,
			.oldSwapchain = VK_NULL_HANDLE
		};

		detail::QueueFamilyIndices indices;
		if (!find_queue_families(device->physicalDevice, device->surface, indices)) {
			fprintf(stderr, "failed to find required queue families!");
		}
		uint32_t queueFamilyIndices[] = {indices.graphicsFamily, indices.presentFamily};

		if (indices.graphicsFamily != indices.presentFamily) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}

		VkSwapchainKHR swapchain;
		VkResult result = vkCreateSwapchainKHR(device->device, &createInfo, nullptr, &swapchain);
		VK_ASSERT(result);

		std::vector<VkImage> swapchainImages;
		result = vkGetSwapchainImagesKHR(device->device, swapchain, &imageCount, nullptr);
		VK_ASSERT(result);
		swapchainImages.resize(imageCount);
		result = vkGetSwapchainImagesKHR(device->device, swapchain, &imageCount, swapchainImages.data());
		VK_ASSERT(result);

		std::vector<VkImageView> swapchainImageViews = detail::create_swap_image_views(device, swapchainImages, surfaceFormat.format);

		std::vector<SwapchainFrame> frames(imageCount);
		for (std::uint32_t i = 0; i < frames.size(); i++) {
			frames[i] = {
				.image = swapchainImages[i],
				.imageView = swapchainImageViews[i]
			};
		}

		Swapchain* pSwapchain = new Swapchain{
			.swapchain = swapchain,
			.extent = extent,
			.frames = frames
		};

		return pSwapchain;
	}

	SwapchainFrame* aquire(Device* device) {
		return {};
	}
} // namespace gfx
