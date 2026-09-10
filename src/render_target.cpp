#include "gfx/device.h"
#include "gfx/render_target.h"
#include "gfx/render_pass.h"
#include "gfx/swapchain.h"

namespace gfx
{
	namespace
	{
		std::vector<VkFramebuffer> create_framebuffers(Device* device, VkExtent2D extent) {
			const auto& swapchainFrames = device->swapchain->frames;
			std::vector<VkFramebuffer> framebuffers(swapchainFrames.size());
			for (size_t i = 0; i < swapchainFrames.size(); i++) {
				VkImageView attachments[] = {
					swapchainFrames[i].imageView
				};

				VkFramebufferCreateInfo framebufferInfo{
					.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.renderPass = device->renderPass->renderPass,
					.attachmentCount = 1,
					.pAttachments = attachments,
					.width = extent.width,
					.height = extent.height,
					.layers = 1
				};

				VkResult result = vkCreateFramebuffer(device->device, &framebufferInfo, nullptr, &framebuffers[i]);
				VK_ASSERT(result);
			}

			return framebuffers;
		}
	} // namespace

	RenderTarget* create_render_target(Device* device) {
		std::vector<VkFramebuffer> framebuffers = create_framebuffers(device, device->swapchain->extent);

		RenderTarget* pRenderTarget = new RenderTarget{
			.framebuffers = std::move(framebuffers)
		};

		return pRenderTarget;
	}

	void destroy_render_target(Device* device, RenderTarget* renderTarget) {
		for (auto framebuffer : renderTarget->framebuffers) {
            vkDestroyFramebuffer(device->device, framebuffer, nullptr);
        }
	}
} // namespace gfx
