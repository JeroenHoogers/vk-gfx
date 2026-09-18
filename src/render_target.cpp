#include "gfx/device.h"
#include "gfx/render_target.h"
#include "gfx/render_pass.h"
#include "gfx/swapchain.h"
#include "gfx/image.h"
#include "gfx/window.h"

namespace gfx
{
	namespace
	{
		std::vector<VkFramebuffer> create_framebuffers(Device* device, Swapchain* swapchain, RenderPass* renderPass, VkExtent2D extent) {
			const auto& swapchainFrames = swapchain->images;
			std::vector<VkFramebuffer> framebuffers(swapchainFrames.size());
			for (size_t i = 0; i < swapchainFrames.size(); i++) {
				std::vector<VkImageView> attachments = {
					swapchainFrames[i].imageView,
					device->frames[0].depthImage->imageView // TODO: also store per window
				};

				VkFramebufferCreateInfo framebufferInfo{
					.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.renderPass = renderPass->renderPass,
					.attachmentCount = static_cast<uint32_t>(attachments.size()),
					.pAttachments = attachments.data(),
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

	namespace detail {
		Image* create_depth_resources(Device* device, Swapchain* swapchain) {
			VkExtent2D swapchainExtent = swapchain->extent;
			Image* depthImage = create_image(device, ImageDesc{
				.format = VK_FORMAT_D32_SFLOAT_S8_UINT, // TODO: allow user to specify and add fallbacks
				.extent = {swapchainExtent.width, swapchainExtent.height, 1},
				.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
				.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				.tiling = VK_IMAGE_TILING_OPTIMAL,
				.aspect = VK_IMAGE_ASPECT_DEPTH_BIT
			});

			detail::transition_image_layout(device, depthImage->image, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

			return depthImage;
		}
	}

	RenderTarget* create_render_target(Device* device, Swapchain* swapchain, RenderPass* renderPass) {
		for (auto& frame : device->frames) {
			frame.depthImage = detail::create_depth_resources(device, swapchain);
		}

		std::vector<VkFramebuffer> framebuffers = create_framebuffers(device, swapchain, renderPass, swapchain->extent);

		RenderTarget* pRenderTarget = new RenderTarget{
			// .renderPass = renderPass,
			.framebuffers = std::move(framebuffers)
		};

		return pRenderTarget;
	}

	void destroy_render_target(Device* device, RenderTarget* renderTarget) {
		for (auto& frame : device->frames) {
			destroy_image(device, frame.depthImage);
		}

		for (auto framebuffer : renderTarget->framebuffers) {
            vkDestroyFramebuffer(device->device, framebuffer, nullptr);
        }

        delete renderTarget;
        renderTarget = nullptr;
	}
} // namespace gfx
