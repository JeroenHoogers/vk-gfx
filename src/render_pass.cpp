#include "gfx/render_pass.h"
#include "gfx/command_buffer.h"
#include "gfx/device.h"
#include "gfx/render_target.h"
#include "gfx/swapchain.h"

namespace gfx
{
	RenderPass* create_render_pass(Device* device, const RenderPassDesc& desc)
	{
		bool enableMsaa = device->msaaSamples != VK_SAMPLE_COUNT_1_BIT;

		VkAttachmentDescription colorAttachment{
			.flags = 0,
			.format = desc.colors.format,
			.samples = device->msaaSamples,
			.loadOp = desc.colors.loadOp,
			.storeOp = desc.colors.storeOp,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = enableMsaa ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
			// .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR  // No MSAA
		};

		std::vector<VkAttachmentDescription> attachments = { colorAttachment };

		VkAttachmentReference colorAttachmentRef{
			.attachment = 0,
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		};

		VkAttachmentReference depthAttachmentRef{
			.attachment = 1,
			.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		};

		VkAttachmentReference colorAttachmentResolveRef{
			.attachment = 2,
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		};

		if (device->enableDepth) {
			VkAttachmentDescription depthStencilAttachment{
				.flags = 0,
				.format = desc.depth.format,
				.samples = device->msaaSamples,
				.loadOp = desc.depth.loadOp,
				.storeOp = desc.depth.storeOp,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
			};

			depthAttachmentRef.attachment = attachments.size();
			attachments.push_back(depthStencilAttachment);
		}

		if(enableMsaa) {
			VkAttachmentDescription colorAttachmentResolve{
				.flags = 0,
				.format = desc.colors.format,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.storeOp = desc.colors.storeOp,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
			};

			colorAttachmentResolveRef.attachment = attachments.size();
			attachments.push_back(colorAttachmentResolve);
		}

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // TODO: support compute
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pResolveAttachments = enableMsaa ? &colorAttachmentResolveRef : nullptr;
		subpass.pDepthStencilAttachment = device->enableDepth ? &depthAttachmentRef : nullptr;

		VkSubpassDependency dependency{
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.dependencyFlags = 0
		};

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
			.renderPass = renderPass,
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
