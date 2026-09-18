#include "gfx/render_pass.h"
#include "gfx/command_buffer.h"
#include "gfx/device.h"
#include "gfx/render_target.h"
#include "gfx/swapchain.h"
#include <array>

namespace gfx
{
	RenderPass* create_render_pass(Device* device, const RenderPassDesc& desc)
	{
		VkAttachmentDescription colorAttachment{
			.flags = 0,
			.format = desc.colors.format,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = desc.colors.loadOp,
			.storeOp = desc.colors.storeOp,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		};

		VkAttachmentDescription depthStencilAttachment{
			.flags = 0,
			.format = desc.depth.format,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = desc.depth.loadOp,
			.storeOp = desc.depth.storeOp,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		};

		VkAttachmentReference colorAttachmentRef{
			.attachment = 0,
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		};

		VkAttachmentReference depthAttachmentRef{
			.attachment = 1,
			.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		};

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // TODO: support compute
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		VkSubpassDependency dependency{
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dependencyFlags = 0
		};

		// std::array<VkAttachmentDescription, 1> attachments = {colorAttachment};
		std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthStencilAttachment};

		VkRenderPassCreateInfo renderPassInfo{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.attachmentCount = static_cast<uint32_t>(attachments.size()),
			.pAttachments = attachments.data(),
			.subpassCount = 1,
			.pSubpasses = &subpass,
			.dependencyCount = 1,
			.pDependencies = &dependency
		};

		VkRenderPass renderPass;

		VkResult result = vkCreateRenderPass(device->device, &renderPassInfo, nullptr, &renderPass);
		VK_ASSERT(result);

		RenderPass* pRenderPass = new RenderPass{
			.renderPass = renderPass
		};

		return pRenderPass;
	}

	void begin_render_pass(Device* device, CommandBuffer* commands, const SwapchainFrame* frame)
	{
		// TODO: provide these?
		std::vector<VkClearValue> clearValues{
			VkClearValue {.color = {{0.0f, 0.0f, 0.0f, 1.0f}}},
			VkClearValue {.depthStencil = {1.0f, 0}}
		};

		VkRenderPassBeginInfo renderPassInfo{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.pNext = nullptr,
			.renderPass = device->renderPass->renderPass,
			.framebuffer = frame->frameBuffer,
			.renderArea = {
				.offset = {0, 0},
				.extent = frame->window->swapchain->extent
			},
			.clearValueCount = static_cast<uint32_t>(clearValues.size()),
			.pClearValues = clearValues.data()
		};

		vkCmdBeginRenderPass(commands->commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	}

	void end_render_pass(CommandBuffer* commands)
	{
		vkCmdEndRenderPass(commands->commandBuffer);
	}

	void destroy_render_pass(Device* device, RenderPass* renderPass)
	{
		vkDestroyRenderPass(device->device, renderPass->renderPass, nullptr);
	}
} // namespace gfx
