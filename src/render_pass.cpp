// Copyright(c) 2026, Jeroen Hoogers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#include "gfx/render_pass.h"
#include "gfx/command_buffer.h"
#include "gfx/device.h"
#include "gfx/render_target.h"
#include "gfx/swapchain.h"

namespace gfx
{
	RenderPass* create_render_pass(Device* device, const RenderPassDesc& params)
	{
		std::vector<VkAttachmentDescription> attachments;
		std::vector<VkAttachmentReference> colorRefs;
		std::vector<VkAttachmentReference> resolveRefs;

		std::vector<AttachmentSlot> slots;
		std::vector<VkClearValue> clearValues;

		attachments.reserve(params.colors.size());
		colorRefs.reserve(params.colors.size());

		bool enableMsaa = device->msaaSamples != VK_SAMPLE_COUNT_1_BIT;

		for (const Attachment& color : params.colors) {
			VkAttachmentDescription colorAttachment{
				.flags = 0,
				.format = color.format,
				.samples = device->msaaSamples,
				.loadOp = color.loadOp,
				.storeOp = enableMsaa ? VK_ATTACHMENT_STORE_OP_DONT_CARE : color.storeOp,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = color.initialLayout,
				.finalLayout = enableMsaa ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : color.finalLayout
			};

			VkAttachmentReference colorRef = {
				.attachment = static_cast<uint32_t>(attachments.size()),
				.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			};

			colorRefs.push_back(colorRef);

			slots.push_back({
				.type = AttachmentSlotType::Color,
				.index = colorRef.attachment,
				.format = colorAttachment.format,
				.samples = colorAttachment.samples
			});

			clearValues.push_back(color.clearColor); // probably not needed

			attachments.push_back(colorAttachment);

			// create resolve
			if (enableMsaa) {
				VkAttachmentDescription colorAttachmentResolve{
					.flags = 0,
					.format = color.format,
					.samples = VK_SAMPLE_COUNT_1_BIT,
					.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
					.storeOp = color.storeOp,
					.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
					.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
					.initialLayout = color.initialLayout,
					.finalLayout = color.finalLayout
				};

				VkAttachmentReference resolveRef = {
					.attachment = static_cast<uint32_t>(attachments.size()),
					.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
				};

				resolveRefs.push_back(resolveRef);
				attachments.push_back(colorAttachmentResolve);

				slots.push_back({
					.type = AttachmentSlotType::Resolve,
					.index = resolveRef.attachment,
					.format = colorAttachmentResolve.format,
					.samples = colorAttachmentResolve.samples
				});

				clearValues.push_back(color.clearColor);
			}
		}

		VkAttachmentReference depthAttachmentRef{};

		if (params.enableDepth) {
			VkAttachmentDescription depthStencilAttachment{
				.flags = 0,
				.format = params.depthStencil.format,
				.samples = device->msaaSamples,
				.loadOp = params.depthStencil.loadOp,
				.storeOp = params.depthStencil.storeOp,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
			};

			depthAttachmentRef = VkAttachmentReference{
				.attachment = static_cast<uint32_t>(attachments.size()),
				.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
			};

			attachments.push_back(depthStencilAttachment);
			slots.push_back({
				.type = AttachmentSlotType::DepthStencil,
				.index = depthAttachmentRef.attachment,
				.format = depthStencilAttachment.format,
				.samples = depthStencilAttachment.samples
			});
			clearValues.push_back(params.depthStencil.clearColor);
		}

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // TODO: support compute
		subpass.colorAttachmentCount = static_cast<uint32_t>(colorRefs.size());
		subpass.pColorAttachments = colorRefs.data();
		subpass.pResolveAttachments = resolveRefs.empty() ? nullptr : resolveRefs.data();
		subpass.pDepthStencilAttachment = params.enableDepth ? &depthAttachmentRef : nullptr;

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

		return new RenderPass{
			.renderPass = renderPass,
			.colors = params.colors,
			.depthStencil = params.depthStencil,
			.useDepth = params.enableDepth,
			.attachmentSlots = std::move(slots),
			.clearValues = std::move(clearValues)
		};
	}

	void begin_render_pass(Device* device, CommandBuffer commands, const SwapchainFrame* frame)
	{
		// TODO: allow renderpass to be supplied, otherwise take the default renderpass from device
		RenderPass* renderPass = device->renderPass;
		// TODO: allow clearvalues to be supplied as arguments

		VkRenderPassBeginInfo renderPassInfo{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.pNext = nullptr,
			.renderPass = device->renderPass->renderPass,
			.framebuffer = frame->frameBuffer,
			.renderArea = {
				.offset = {0, 0},
				.extent = frame->window->swapchain->extent
			},
			.clearValueCount = static_cast<uint32_t>(renderPass->clearValues.size()),
			.pClearValues = renderPass->clearValues.data()
		};

		vkCmdBeginRenderPass(commands, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	}

	void end_render_pass(CommandBuffer commands)
	{
		vkCmdEndRenderPass(commands);
	}

	void destroy_render_pass(Device* device, RenderPass* renderPass)
	{
		vkDestroyRenderPass(device->device, renderPass->renderPass, nullptr);
	}
} // namespace gfx
