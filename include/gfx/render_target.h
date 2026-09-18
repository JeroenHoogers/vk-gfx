#pragma once
#include <vector>
#include <vulkan/vulkan.h>

namespace gfx
{
	struct Device;
	struct Swapchain;
	struct RenderPass;
	struct Image;

	namespace detail {
		Image* create_depth_resources(Device* device, Swapchain* swapchain);
	}

	struct RenderTarget
	{
		// RenderPass* renderPass;
		// TODO: add depth?
		// std::vector<VkImage> depthImages;
		// std::vector<VkImageView> depthViews;
		std::vector<VkFramebuffer> framebuffers;
	    // RenderPass* renderPass; // weak reference?
	};

	RenderTarget* create_render_target(Device* device, Swapchain* swapchain, RenderPass* renderPass);
	void destroy_render_target(Device* device, RenderTarget* renderTarget);
} // namespace gfx
