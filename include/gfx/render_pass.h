#pragma once
#include <vulkan/vulkan.h>

namespace gfx
{
	struct Device;
	struct CommandBuffer;

	struct RenderPass
	{
		VkRenderPass renderPass;
	};

	[[nodiscard]] RenderPass* create_render_pass(Device* device);

	void begin_render_pass(Device* device, CommandBuffer* commands);
	void end_render_pass(CommandBuffer* commands);
	void destroy_render_pass(Device* device, RenderPass* renderPass);
} // namespace gfx
