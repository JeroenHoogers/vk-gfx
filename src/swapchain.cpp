#include "gfx/swapchain.h"
#include "gfx/device.h"
#include "gfx/render_target.h"
#include "gfx/sync.h"
#include "gfx/image.h"
#include "gfx/window.h"
#include <algorithm>
#include <limits>
#include <vulkan/vk_enum_string_helper.h>

namespace gfx
{
	namespace detail
	{
		SwapChainSupportDetails query_swapchain_support(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
		{
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

		VkPresentModeKHR choose_swap_present_mode(const std::vector<VkPresentModeKHR>& availableModes)
		{
			for (const auto& mode : availableModes) {
				if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
					return mode;
				}
			}

			return VK_PRESENT_MODE_FIFO_KHR;
		}

		VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height)
		{
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

		VkSurfaceFormatKHR choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& availableFormats, VkFormat desiredFormat)
		{
			for (const auto& availableFormat : availableFormats) {
				if (availableFormat.format == desiredFormat && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
					return availableFormat;
				}
			}

			return availableFormats[0];
		}

		std::vector<VkImageView> create_swap_image_views(Device* device, const std::vector<VkImage>& images, VkFormat imageFormat)
		{
			std::vector<VkImageView> imageViews(images.size());

			for (size_t i = 0; i < images.size(); i++) {
				imageViews[i] = create_image_view(device, images[i], imageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
			}
			return imageViews;
		}

	} // namespace detail

	Swapchain* create_swapchain(Device* device, Window* window, const SwapchainDesc& params)
	{
		auto swapchainSupport = detail::query_swapchain_support(device->physicalDevice, window->surface);
		VkPresentModeKHR presentMode = detail::choose_swap_present_mode(swapchainSupport.presentModes);
		window->callbacks.get_framebuffer_size(&window->width, &window->height, window->callbacks.user_data);

		const VkSurfaceCapabilitiesKHR& caps = swapchainSupport.capabilities;
		VkExtent2D extent = detail::choose_swap_extent(caps, window->width, window->height);
		VkSurfaceFormatKHR surfaceFormat = detail::choose_swap_surface_format(swapchainSupport.formats, params.format);

		uint32_t imageCount = caps.minImageCount + 1;
		if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount) {
			imageCount = caps.maxImageCount;
		}

		VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_FLAG_BITS_MAX_ENUM_KHR;
		if (caps.supportedCompositeAlpha & params.compositeAlpha) {
			compositeAlpha = params.compositeAlpha;
		} else if (caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) {
			compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		} else if (caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR) {
			compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
		}

		if (compositeAlpha != params.compositeAlpha) {
			if(compositeAlpha == VK_COMPOSITE_ALPHA_FLAG_BITS_MAX_ENUM_KHR) {
				printf("Unable to create swapchain. No suitable composite alpha mode found.\n");
				abort();
			}

			printf("The requested composite alpha mode: %s is not supported. Falling back to: %s.\n", string_VkCompositeAlphaFlagBitsKHR(params.compositeAlpha), string_VkCompositeAlphaFlagBitsKHR(compositeAlpha));
		}

		VkSwapchainCreateInfoKHR createInfo{
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.pNext = nullptr,
			.flags = 0,
			.surface = window->surface,
			.minImageCount = imageCount,
			.imageFormat = surfaceFormat.format,
			.imageColorSpace = surfaceFormat.colorSpace,
			.imageExtent = extent,
			.imageArrayLayers = 1,
			.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = 0,
			.pQueueFamilyIndices = nullptr,
			.preTransform = caps.currentTransform,
			.compositeAlpha = compositeAlpha,
			.presentMode = presentMode,
			.clipped = VK_TRUE,
			.oldSwapchain = VK_NULL_HANDLE
		};

		uint32_t queueFamilyIndices[] = {device->graphicsQueue.familyIndex, device->presentQueue.familyIndex};

		if (device->graphicsQueue.familyIndex != device->presentQueue.familyIndex) {
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

		std::vector<SwapchainImage> images(imageCount);
		for (std::uint32_t i = 0; i < images.size(); i++) {
			images[i] = {
				.image = swapchainImages[i],
				.imageView = swapchainImageViews[i],
				.renderFinished = create_semaphore(device)
			};
		}

		Swapchain* pSwapchain = new Swapchain{
			.swapchain = swapchain,
			.extent = extent,
			.format = surfaceFormat.format,
			.compositeAlpha = compositeAlpha,
			.images = std::move(images)
		};

		return pSwapchain;
	}

	void recreate_swapchain(Device* device, Window* window)
	{
		VkFormat format = window->swapchain->format;
		VkCompositeAlphaFlagBitsKHR compositeAlpha = window->swapchain->compositeAlpha;
		vkDeviceWaitIdle(device->device);

		destroy_render_target(device, window->renderTarget);
		destroy_swapchain(device, window->swapchain);

		window->swapchain = create_swapchain(device, window, {
			.format = format,
			.compositeAlpha = compositeAlpha
		});
		window->renderTarget = create_render_target(device, window, device->renderPass);
	}

	void destroy_swapchain(Device* device, Swapchain* swapchain)
	{
		for (auto& frame : swapchain->images) {
			vkDestroySemaphore(device->device, frame.renderFinished, nullptr);
			vkDestroyImageView(device->device, frame.imageView, nullptr);
		}

		vkDestroySwapchainKHR(device->device, swapchain->swapchain, nullptr);
		delete swapchain;
		swapchain = nullptr;
	}

	SwapchainFrame acquire(Device* device, Window* window)
	{
		// if no window specified use main window
		const Frame& frameInFlight = window->frames[window->currentFrame];

		vkWaitForFences(device->device, 1, &frameInFlight.inFlightFence, VK_TRUE, UINT64_MAX);
		VkResult result = vkAcquireNextImageKHR(device->device, window->swapchain->swapchain, UINT64_MAX, frameInFlight.imageAvailable, VK_NULL_HANDLE, &window->swapchain->imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreate_swapchain(device, window);
			return {};
		}
		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			std::fprintf(stderr, "Vulkan error aquiring next image: %d\n", static_cast<int>(result));
		}

		vkResetFences(device->device, 1, &frameInFlight.inFlightFence);

		// create dynamic state object
		VkViewport defaultViewport{
			.x = 0.0f,
			.y = 0.0f,
			.width = static_cast<float>(window->swapchain->extent.width),
			.height = static_cast<float>(window->swapchain->extent.height),
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};

		VkRect2D defaultScissor{
			.offset = {0, 0},
			.extent = window->swapchain->extent
		};

		DynamicState dynamicState{
			.viewport = defaultViewport,
			.scissor = defaultScissor,
		};

		const uint32_t imageIndex = window->swapchain->imageIndex;
		SwapchainFrame swapchainFrame {
			.image = window->swapchain->images[imageIndex],
			.window = window,
			.dynamicState = dynamicState,
			.frameBuffer = window->renderTarget->framebuffers[imageIndex],
			.renderFinished = window->swapchain->images[imageIndex].renderFinished,
			.extent = window->swapchain->extent,
			.index = window->currentFrame,
			.swapImageIndex = imageIndex,
		};

		return swapchainFrame;
	}
} // namespace gfx
