#include "gfx/render_pass.h"
#include "gfx/device.h"
#include "gfx/swapchain.h"
#include "gfx/render_target.h"
#include "gfx/command_buffer.h"

namespace gfx
{
	RenderPass* create_render_pass(Device* device) {
		VkAttachmentDescription colorAttachment{
			.flags = 0,
			.format = device->swapchain->format,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		};

		VkAttachmentReference colorAttachmentRef{
			.attachment = 0,
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		};

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // TODO: support compute
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;

		VkRenderPass renderPass;

		VkResult result = vkCreateRenderPass(device->device, &renderPassInfo, nullptr, &renderPass);
		VK_ASSERT(result);

		RenderPass* pRenderPass = new RenderPass{
			.renderPass = renderPass
		};

		return pRenderPass;
	}

	void begin_render_pass(Device* device, CommandBuffer* commands) {
		VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};

		VkRenderPassBeginInfo renderPassInfo{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.pNext = nullptr,
			.renderPass = device->renderPass->renderPass,
			.framebuffer = device->renderTarget->framebuffers[0],
			.renderArea = {
				.offset = {0, 0},
				.extent = device->swapchain->extent
			},
			.clearValueCount = 1,
			.pClearValues = &clearColor
		};

		vkCmdBeginRenderPass(commands->commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	}

	void end_render_pass(CommandBuffer* commands) {
		vkCmdEndRenderPass(commands->commandBuffer);
	}

	void destroy_render_pass(Device* device, RenderPass* renderPass) {
		vkDestroyRenderPass(device->device, renderPass->renderPass, nullptr);
	}
} // namespace gfx
