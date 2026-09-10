#pragma once
#include <vulkan/vulkan.h>

namespace gfx
{
	struct Device;

	struct RenderPass
	{
		VkRenderPass renderPass;
	};

	RenderPass* create_render_pass(Device* device);

	void begin_render_pass();
	void end_render_pass();
	void destroy_render_pass(Device* device, RenderPass* renderPass);
} // namespace gfx
