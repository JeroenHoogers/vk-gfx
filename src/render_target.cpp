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
		std::vector<VkFramebuffer> create_framebuffers(Device* device, Window* window, std::vector<Image*> colorImages, std::vector<Image*> depthImages, RenderPass* renderPass) {
			const auto& swapchain = window->swapchain;
			std::vector<VkFramebuffer> framebuffers(swapchain->images.size());
			for (size_t i = 0; i < swapchain->images.size(); i++) {
				std::vector<VkImageView> attachments = {};

				if(device->msaaSamples == VK_SAMPLE_COUNT_1_BIT) { // no msaa
					attachments.push_back(swapchain->images[i].imageView);
					if(device->enableDepth) {
						attachments.push_back(depthImages[window->currentFrame]->imageView);// TODO: check if indexing is correct
					}
				} else {
					// msaa enabled
					attachments.push_back(colorImages[window->currentFrame]->imageView); // TODO: check if indexing is correct
					if(device->enableDepth) { // no msaa + depth
						attachments.push_back(depthImages[window->currentFrame]->imageView);// TODO: check if indexing is correct
					}
					attachments.push_back(swapchain->images[i].imageView);
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

				VkResult result = vkCreateFramebuffer(device->device, &framebufferInfo, nullptr, &framebuffers[i]);
				VK_ASSERT(result);
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
		std::vector<Image*> depthImages(device->framesInFlight);
		std::vector<Image*> colorImages(device->framesInFlight);

		for (uint32_t i = 0; i < device->framesInFlight; i++) {
			colorImages[i] = detail::create_color_resources(device, window->swapchain);
			depthImages[i] = detail::create_depth_resources(device, window->swapchain);
		}

		std::vector<VkFramebuffer> framebuffers = create_framebuffers(device, window, colorImages, depthImages, renderPass);

		RenderTarget* pRenderTarget = new RenderTarget{
			// .renderPass = renderPass,
			.colorImages = std::move(colorImages),
			.depthImages = std::move(depthImages),
			.framebuffers = std::move(framebuffers),
		};

		return pRenderTarget;
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
