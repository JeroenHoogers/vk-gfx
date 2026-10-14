// Copyright(c) 2026, Jeroen Hoogers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include <vector>
#include <vulkan/vulkan.h>

namespace gfx
{
	struct Device;
	struct Window;
	struct Swapchain;
	struct RenderPass;
	struct Image;

	namespace detail {
		Image create_depth_resources(Device* device, VkExtent2D extent, VkFormat format, VkImageUsageFlags = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
		Image create_color_resources(Device* device, VkExtent2D extent, VkFormat format, VkImageUsageFlags = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
	}

	struct RenderTargetDesc {
		VkExtent2D extent;
		VkFormat colorFormat;
		VkImageUsageFlags colorUsage;
	};

	struct RenderTarget
	{
		VkExtent2D extent;
		VkFormat colorFormat;
		std::vector<Image> colorImages; // used for MSAA
		std::vector<Image> depthImages;
		std::vector<VkFramebuffer> framebuffers;
	};

	[[nodiscard]] RenderTarget* create_render_target(Device* device, RenderPass* renderPass, Window* window);
	[[nodiscard]] RenderTarget* create_render_target(Device* device, RenderPass* renderPass, const RenderTargetDesc& params);
	void destroy_render_target(Device* device, RenderTarget* renderTarget);
} // namespace gfx
