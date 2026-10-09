// Copyright(c) 2026, Jeroen Hoogers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

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
		// TODO: allow shared attachments (single depth / msaa image instead of framesInFlight images)
		// TODO: do not create additional framebuffers when depthImages and colorImages are both unused / empty
		std::vector<VkFramebuffer> create_framebuffers(Device* device, Window* window, std::vector<Image*> colorImages, std::vector<Image*> depthImages, RenderPass* renderPass) {
			const auto& swapchain = window->swapchain;
			std::vector<VkFramebuffer> framebuffers(swapchain->images.size() * device->framesInFlight);

			// create swapchainImages * framesInFlight framebuffers to allow overlap in attachments
			for (uint32_t i = 0; i < swapchain->images.size(); i++) {
				for (uint32_t j = 0; j < device->framesInFlight; j++) {
					std::vector<VkImageView> attachments(renderPass->attachmentSlots.size());

					for(uint32_t k = 0; k < renderPass->attachmentSlots.size(); k++) {
						const auto& slot = renderPass->attachmentSlots[k];
						switch (slot.type) {
							case AttachmentSlotType::Color:
								// TODO: support non-msaa non-swapchain image attachments
								attachments[k] = (slot.samples == VK_SAMPLE_COUNT_1_BIT) ? swapchain->images[i].imageView : colorImages[j]->imageView;
								break;
							case AttachmentSlotType::DepthStencil:
								attachments[k] = depthImages[j]->imageView;
								break;
							case AttachmentSlotType::Resolve:
								attachments[k] = swapchain->images[i].imageView;
								break;
						}
					}

					VkFramebufferCreateInfo framebufferInfo{
						.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
						.pNext = nullptr,
						.flags = 0,
						.renderPass = renderPass->renderPass,
						.attachmentCount = static_cast<uint32_t>(attachments.size()),
						.pAttachments = attachments.data(),
						.width = swapchain->extent.width,
						.height = swapchain->extent.height,
						.layers = 1
					};

					uint32_t framebufferIndex = i * device->framesInFlight + j;
					VkResult result = vkCreateFramebuffer(device->device, &framebufferInfo, nullptr, &framebuffers[framebufferIndex]);
					VK_ASSERT(result);
				}
			}

			return framebuffers;
		}
	} // namespace

	namespace detail {
		Image* create_depth_resources(Device* device, Swapchain* swapchain)
		{
			VkExtent2D swapchainExtent = swapchain->extent;
			Image* depthImage = create_image(device, ImageDesc{
				.format = VK_FORMAT_D32_SFLOAT_S8_UINT, // TODO: allow user to specify and add fallbacks
				.extent = {swapchainExtent.width, swapchainExtent.height, 1},
				.samples = device->msaaSamples,
				.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
				.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				.tiling = VK_IMAGE_TILING_OPTIMAL,
				.aspect = VK_IMAGE_ASPECT_DEPTH_BIT
			});

			detail::transition_image_layout(device, depthImage->image, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

			return depthImage;
		}

		Image* create_color_resources(Device* device, Swapchain* swapchain)
		{
			VkExtent2D swapchainExtent = swapchain->extent;
			VkFormat colorFormat = swapchain->format;

			Image* colorImage = create_image(device, ImageDesc{
				.format = colorFormat,
				.extent = {swapchainExtent.width, swapchainExtent.height, 1},
				.samples = device->msaaSamples,
				.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
				.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				.tiling = VK_IMAGE_TILING_OPTIMAL,
				.aspect = VK_IMAGE_ASPECT_COLOR_BIT
			});

			return colorImage;
		}
	}

	RenderTarget* create_render_target(Device* device, Window* window, RenderPass* renderPass) {
		uint32_t colorCount = 0;
		uint32_t depthStencilCount = 0;
		// uint32_t resolveCount = 0; // TODO: needed for headless

		for (const auto& slot : renderPass->attachmentSlots) {
			switch (slot.type) {
				case AttachmentSlotType::Color:
					// only create resources when MSAA is enabled
					if (slot.samples != VK_SAMPLE_COUNT_1_BIT) {
						colorCount++;
					}
					break;
				case gfx::AttachmentSlotType::DepthStencil:
					depthStencilCount++;
					break;
				case gfx::AttachmentSlotType::Resolve:
					// resolveCount++;
					break;
			}
		}

		std::vector<Image*> depthImages(device->framesInFlight * depthStencilCount);
		std::vector<Image*> colorImages(device->framesInFlight * colorCount);

		for (uint32_t i = 0; i < device->framesInFlight; i++) {
			for(uint32_t j = 0; j < colorCount; j++) {
				uint32_t idx = i * colorCount + j;
				colorImages[idx] = detail::create_color_resources(device, window->swapchain);
			}
			for(uint32_t j = 0; j < depthStencilCount; j++) {
				uint32_t idx = i * depthStencilCount + j;
				depthImages[idx] = detail::create_depth_resources(device, window->swapchain);
			}
		}

		std::vector<VkFramebuffer> framebuffers = create_framebuffers(device, window, colorImages, depthImages, renderPass);

		return new RenderTarget{
			.colorImages = std::move(colorImages),
			.depthImages = std::move(depthImages),
			.framebuffers = std::move(framebuffers),
		};
	}

	void destroy_render_target(Device* device, RenderTarget* renderTarget) {
		for (auto* colorImage : renderTarget->colorImages) {
			destroy_image(device, colorImage);
		}

		for (auto* depthImage : renderTarget->depthImages) {
			destroy_image(device, depthImage);
		}

		for (auto framebuffer : renderTarget->framebuffers) {
            vkDestroyFramebuffer(device->device, framebuffer, nullptr);
        }

        delete renderTarget;
        renderTarget = nullptr;
	}
} // namespace gfx
