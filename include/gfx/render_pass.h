#pragma once
#include <vulkan/vulkan.h>

namespace gfx
{
	struct Device;
	struct CommandBuffer;
	struct SwapchainFrame;

	struct ColorAttachment
	{
		VkFormat format = VK_FORMAT_B8G8R8A8_SRGB;
		VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
	};

	struct DepthAttachment
	{
		VkFormat format = VK_FORMAT_D32_SFLOAT_S8_UINT;
		VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		VkClearValue clearColor = {{{1.0f, 0.0f}}};
	};

	struct RenderPass
	{
		VkRenderPass renderPass;
		ColorAttachment colors = {}; // TODO: allow more than one
		DepthAttachment depth = {};
	};

	struct RenderPassDesc
	{
		ColorAttachment colors = {}; // TODO: allow more than one
		DepthAttachment depth = {};
		// StencilAttachment stencil = {};
	};

	struct RenderPassInfo
	{
		VkImageView color;
		// std::vector<VkClearColorValue> clearValues;
	};

	[[nodiscard]] RenderPass* create_render_pass(Device* device, const RenderPassDesc& desc = {});

	void begin_render_pass(Device* device, CommandBuffer* commands, const SwapchainFrame* frame);
	void end_render_pass(CommandBuffer* commands);
	void destroy_render_pass(Device* device, RenderPass* renderPass);
} // namespace gfx
