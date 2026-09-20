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
		Image* create_depth_resources(Device* device, Swapchain* swapchain);
	}

	struct RenderTarget
	{
		// RenderPass* renderPass;
		std::vector<Image*> colorImages; // used for MSAA
		std::vector<Image*> depthImages;
		std::vector<VkFramebuffer> framebuffers;
	};

	RenderTarget* create_render_target(Device* device, Window* window, RenderPass* renderPass);
	void destroy_render_target(Device* device, RenderTarget* renderTarget);
} // namespace gfx
