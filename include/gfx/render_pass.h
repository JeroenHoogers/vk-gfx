// Copyright(c) 2026, Jeroen Hoogers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include "types.h"
#include <vector>
#include <vulkan/vulkan.h>

namespace gfx
{
	struct Device;
	struct SwapchainFrame;

	struct Attachment
	{
		VkFormat format = VK_FORMAT_B8G8R8A8_SRGB;
		VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE;

		VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkImageLayout finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		// default clear color, only used when it is not specified in begin_render_pass()
		VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
	};

	enum class AttachmentSlotType {
		Color,	 // msaa source, or direct output when samples == 1
		Resolve, // generated single-sample output
		DepthStencil
	};

	struct AttachmentSlot
	{
		AttachmentSlotType type;
		uint32_t index;
		VkFormat format;
		VkSampleCountFlagBits samples;
	};

	struct RenderPass
	{
		VkRenderPass renderPass;
		std::vector<Attachment> colors;
		Attachment depthStencil;
		const bool useDepth = false;
		std::vector<AttachmentSlot> attachmentSlots;
		std::vector<VkClearValue> clearValues;
	};

	struct RenderPassDesc
	{
		std::vector<Attachment> colors = {{
			.format = VK_FORMAT_B8G8R8A8_SRGB,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			.clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}}
		}};
		Attachment depthStencil = {
			.format = VK_FORMAT_D32_SFLOAT_S8_UINT,
			.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			.clearColor = {{{1.0f, 0.0f}}}
		};
		bool enableDepth = false;
	};

	[[nodiscard]] RenderPass* create_render_pass(Device* device, const RenderPassDesc& desc = {});

	void begin_render_pass(Device* device, CommandBuffer commands, const SwapchainFrame* frame);
	void end_render_pass(CommandBuffer commands);
	void destroy_render_pass(Device* device, RenderPass* renderPass);
} // namespace gfx
