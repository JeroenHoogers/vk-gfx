#pragma once
#include <vector>
#include <vulkan/vulkan.h>

namespace gfx
{
	struct Device;
	struct RenderPass;

	struct RenderTarget
	{
		// TODO: add depth?
		// std::vector<VkImage> depthImages;
		// std::vector<VkImageView> depthViews;
		std::vector<VkFramebuffer> framebuffers;
	    // RenderPass* renderPass; // weak reference?
	};

	RenderTarget* create_render_target(Device* device);
	void destroy_render_target(Device* device, RenderTarget* renderTarget);
} // namespace gfx
