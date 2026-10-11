// Copyright(c) 2026, Jeroen Hoogers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#include "gfx/device.h"
#include "gfx/render_target.h"
#include "gfx/render_pass.h"
#include "gfx/swapchain.h"
#include "gfx/image.h"
#include "gfx/window.h"
#include "gfx/command_buffer.h"

namespace gfx
{
	namespace
	{
		VkFramebuffer create_framebuffer(Device* device, RenderPass* renderPass, VkExtent2D extent, const std::vector<VkImageView>& attachments)
		{
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

			VkFramebuffer framebuffer;
			VkResult result = vkCreateFramebuffer(device->device, &framebufferInfo, nullptr, &framebuffer);
			VK_ASSERT(result);

			return framebuffer;
		}

		// TODO: allow shared attachments (single depth / msaa image instead of framesInFlight images)
		// TODO: do not create additional framebuffers when depthImages and colorImages are both unused / empty
		std::vector<VkFramebuffer> create_framebuffers(Device* device, RenderPass* renderPass, Window* window, std::vector<Image> colorImages, std::vector<Image> depthImages) {
			const auto& swapchain = window->swapchain;
			std::vector<VkFramebuffer> framebuffers(swapchain->images.size() * device->framesInFlight);

			// Create #SwapchainImages * #FramesInFlight framebuffers. 3 Swapchain Images and 2 Frames in flight 3*2 = 6 Framebuffers:
			//
			// | FrameBuffer     | FB [0]    | FB [1]    | FB [2]    | FB [3]    | FB [4]    | FB [5]    |
			// |-----------------|-----------|-----------|-----------|-----------|-----------|-----------|
			// | Swapchain Image | SI [0]    | SI [1]    | SI [2]    | SI [0]    | SI [1]    | SI [2]    |
			// | Frame In Flight | F  [0]    | F  [1]    | F  [0]    | F  [1]    | F  [0]    | F  [1]    |

			for (uint32_t i = 0; i < swapchain->images.size(); i++) {
				for (uint32_t j = 0; j < device->framesInFlight; j++) {
					std::vector<VkImageView> attachments(renderPass->attachmentSlots.size());

					for(uint32_t k = 0; k < renderPass->attachmentSlots.size(); k++) {
						const auto& slot = renderPass->attachmentSlots[k];
						switch (slot.type) {
							case AttachmentSlotType::Color:
								// TODO: support non-msaa non-swapchain image attachments
								attachments[k] = (slot.samples == VK_SAMPLE_COUNT_1_BIT) ? swapchain->images[i].imageView : colorImages[j].imageView;
								break;
							case AttachmentSlotType::DepthStencil:
								attachments[k] = depthImages[j].imageView;
								break;
							case AttachmentSlotType::Resolve:
								attachments[k] = swapchain->images[i].imageView;
								break;
						}
					}

					uint32_t framebufferIndex = i * device->framesInFlight + j;
					framebuffers[framebufferIndex] = create_framebuffer(device, renderPass, swapchain->extent, attachments);
				}
			}

			return framebuffers;
		}


		std::vector<VkFramebuffer> create_framebuffers(Device* device, RenderPass* renderPass, VkExtent2D extent, std::vector<Image> colorImages, std::vector<Image> depthImages) {
			std::vector<VkFramebuffer> framebuffers(device->framesInFlight);

			// Create #FramesInFlight framebuffers. 2 Frames in flight:
			//
			// | FrameBuffer     | FB [0]    | FB [1]    |
			// |-----------------|-----------|-----------|
			// | Frame In Flight | F  [0]    | F  [1]    |

			for (uint32_t i = 0; i < device->framesInFlight; i++) {
				std::vector<VkImageView> attachments(renderPass->attachmentSlots.size());

				for(uint32_t k = 0; k < renderPass->attachmentSlots.size(); k++) {
					const auto& slot = renderPass->attachmentSlots[k];
					switch (slot.type) {
						case AttachmentSlotType::Color:
						case AttachmentSlotType::Resolve:
							// TODO: support non-msaa non-swapchain image attachments
							attachments[k] = colorImages[i].imageView;
							break;
						case AttachmentSlotType::DepthStencil:
							attachments[k] = depthImages[i].imageView;
							break;
					}
				}

				framebuffers[i] = create_framebuffer(device, renderPass, extent, attachments);
			}

			return framebuffers;
		}

	} // namespace

	namespace detail {
		Image create_depth_resources(Device* device, VkExtent2D extent, VkFormat format)
		{
			Image depthImage = create_image(device, ImageDesc{
				.format = format, // TODO: allow user to specify and add fallbacks
				.extent = {extent.width, extent.height, 1},
				.samples = device->msaaSamples,
				.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
				.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				.tiling = VK_IMAGE_TILING_OPTIMAL,
				.aspect = VK_IMAGE_ASPECT_DEPTH_BIT
			});

			VkCommandBuffer commandBuffer = detail::begin_one_time_commands(device);
			detail::transition_image_layout(commandBuffer, depthImage.image, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
			detail::end_one_time_commands(device, commandBuffer);

			return depthImage;
		}

		Image create_color_resources(Device* device, VkExtent2D extent, VkFormat format)
		{
			Image colorImage = create_image(device, ImageDesc{
				.format = format,
				.extent = {extent.width, extent.height, 1},
				.samples = device->msaaSamples,
				.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
				.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				.tiling = VK_IMAGE_TILING_OPTIMAL,
				.aspect = VK_IMAGE_ASPECT_COLOR_BIT
			});

			return colorImage;
		}
	}

	RenderTarget* create_render_target(Device* device, RenderPass* renderPass, Window* window) {
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

		std::vector<Image> depthImages(device->framesInFlight * depthStencilCount);
		std::vector<Image> colorImages(device->framesInFlight * colorCount);

		for (uint32_t i = 0; i < device->framesInFlight; i++) {
			for(uint32_t j = 0; j < colorCount; j++) {
				uint32_t idx = i * colorCount + j;
				colorImages[idx] = detail::create_color_resources(device, window->swapchain->extent, window->swapchain->format);
			}
			for(uint32_t j = 0; j < depthStencilCount; j++) {
				uint32_t idx = i * depthStencilCount + j;
				depthImages[idx] = detail::create_depth_resources(device, window->swapchain->extent, VK_FORMAT_D32_SFLOAT_S8_UINT);
			}
		}

		std::vector<VkFramebuffer> framebuffers = create_framebuffers(device, renderPass, window, colorImages, depthImages);

		return new RenderTarget{
			.extent = window->swapchain->extent,
			.colorFormat = window->swapchain->format,
			.colorImages = std::move(colorImages),
			.depthImages = std::move(depthImages),
			.framebuffers = std::move(framebuffers),
		};
	}

	RenderTarget* create_render_target(Device* device, RenderPass* renderPass, const RenderTargetDesc& params)
	{
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

		std::vector<Image> depthImages(device->framesInFlight * depthStencilCount);
		std::vector<Image> colorImages(device->framesInFlight * colorCount);

		for (uint32_t i = 0; i < device->framesInFlight; i++) {
			for(uint32_t j = 0; j < colorCount; j++) {
				uint32_t idx = i * colorCount + j;
				colorImages[idx] = detail::create_color_resources(device, params.extent, renderPass->colors[j].format);
			}
			for(uint32_t j = 0; j < depthStencilCount; j++) {
				uint32_t idx = i * depthStencilCount + j;
				depthImages[idx] = detail::create_depth_resources(device, params.extent, renderPass->depthStencil.format);
			}
		}

		std::vector<VkFramebuffer> framebuffers = create_framebuffers(device, renderPass, params.extent, colorImages, depthImages);

		return new RenderTarget{
			.extent = params.extent,
			.colorFormat = params.colorFormat,
			.colorImages = std::move(colorImages),
			.depthImages = std::move(depthImages),
			.framebuffers = std::move(framebuffers),
		};
	}

	void destroy_render_target(Device* device, RenderTarget* renderTarget) {
		for (auto& colorImage : renderTarget->colorImages) {
			destroy_image(device, colorImage);
		}

		for (auto& depthImage : renderTarget->depthImages) {
			destroy_image(device, depthImage);
		}

		for (auto framebuffer : renderTarget->framebuffers) {
            vkDestroyFramebuffer(device->device, framebuffer, nullptr);
        }

        delete renderTarget;
        renderTarget = nullptr;
	}
} // namespace gfx
