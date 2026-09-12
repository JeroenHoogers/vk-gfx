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

	struct ColorAttachment
	{
		VkImageView renderView = VK_NULL_HANDLE;
		VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
	};

	struct RenderPassDesc
	{
		ColorAttachment colors = {}; // TODO: allow more than one
		// DepthAttachment depth = {};
		// StencilAttachment stencil = {};
	};

	[[nodiscard]] RenderPass* create_render_pass(Device* device);

	void begin_render_pass(Device* device, CommandBuffer* commands, const RenderPassDesc& desc = {});
	void end_render_pass(CommandBuffer* commands);
	void destroy_render_pass(Device* device, RenderPass* renderPass);
} // namespace gfx
