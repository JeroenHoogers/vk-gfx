// Copyright(c) 2026, Jeroen Hoogers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#include "gfx/image.h"
#include "gfx/buffer.h"
#include "gfx/command_buffer.h"
#include "gfx/device.h"
#include <cstring>
#include <algorithm>

namespace gfx
{
	namespace
	{
		void copy_buffer_to_image(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
		{
			VkBufferImageCopy region{
				.bufferOffset = 0,
				.bufferRowLength = 0,
				.bufferImageHeight = 0,
				.imageSubresource = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.mipLevel = 0,
					.baseArrayLayer = 0,
					.layerCount = 1
				},
				.imageOffset = {0, 0, 0},
				.imageExtent = {width, height, 1}
			};

			vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
		}


		bool has_stencil_component(VkFormat format)
		{
			return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
		}

		void generate_mipmaps(Device* device, VkCommandBuffer commandBuffer, VkImage image, VkFormat format, int32_t width, int32_t height, uint32_t mipLevels)
		{
			// check if image format supports linear blitting
			VkFormatProperties formatProperties;
			vkGetPhysicalDeviceFormatProperties(device->physicalDevice, format, &formatProperties);
			if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
				printf("Unable to generate mipmaps! Texture image format does not support linear blitting!");
			}

			VkImageMemoryBarrier barrier{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.pNext = nullptr,
				.srcAccessMask = 0,
				.dstAccessMask = 0,
				.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.newLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.image = image,
				.subresourceRange = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 0,
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = 1
				}
			};

			int32_t mipWidth = width;
			int32_t mipHeight = height;

			for (uint32_t i = 1; i < mipLevels; i++) {
				barrier.subresourceRange.baseMipLevel = i - 1;
				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

				vkCmdPipelineBarrier(
					commandBuffer,
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
					0, nullptr,
					0, nullptr,
					1, &barrier
				);

				VkImageBlit blit{
					.srcSubresource = {
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						.mipLevel = i - 1,
						.baseArrayLayer = 0,
						.layerCount = 1
					},
					.srcOffsets = {
						{0, 0, 0},
						{mipWidth, mipHeight, 1}
					},
					.dstSubresource = {
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						.mipLevel = i,
						.baseArrayLayer = 0,
						.layerCount = 1
					},
					.dstOffsets = {
						{0, 0, 0},
						{mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1}
					}
				};

				vkCmdBlitImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				vkCmdPipelineBarrier(
					commandBuffer,
					VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
					0, nullptr,
					0, nullptr,
					1, &barrier
				);

				if (mipWidth > 1)
					mipWidth /= 2;
				if (mipHeight > 1)
					mipHeight /= 2;
			}

			barrier.subresourceRange.baseMipLevel = mipLevels - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
		}
	} // namespace

	namespace detail
	{
		VkImage create_image(Device* device, const ImageDesc& params, VkDeviceMemory& imageMemory)
		{
			VkImageCreateInfo imageInfo{
				.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.imageType = VK_IMAGE_TYPE_2D,
				.format = params.format,
				.extent = params.extent,
				.mipLevels = params.mipLevels,
				.arrayLayers = 1,
				.samples = params.samples,
				.tiling = params.tiling,
				.usage = params.usage,
				.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
				.queueFamilyIndexCount = 0,
				.pQueueFamilyIndices = nullptr,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			};

			VkImage image;
			VkResult result = vkCreateImage(device->device, &imageInfo, nullptr, &image);
			VK_ASSERT(result);

			VkMemoryRequirements memRequirements;
			vkGetImageMemoryRequirements(device->device, image, &memRequirements);

			VkMemoryAllocateInfo allocInfo{
				.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
				.pNext = nullptr,
				.allocationSize = memRequirements.size,
				.memoryTypeIndex = detail::find_memory_type(device, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
			};

			result = vkAllocateMemory(device->device, &allocInfo, nullptr, &imageMemory);
			VK_ASSERT(result);

			result = vkBindImageMemory(device->device, image, imageMemory, 0);
			VK_ASSERT(result);

			return image;
		}

		VkImageView create_image_view(Device* device, VkImage image, VkFormat format, VkImageAspectFlags aspect, uint32_t mipLevels)
		{
			VkImageViewCreateInfo viewInfo{
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.image = image,
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = format,
				.components = {
					.r = VK_COMPONENT_SWIZZLE_IDENTITY,
					.g = VK_COMPONENT_SWIZZLE_IDENTITY,
					.b = VK_COMPONENT_SWIZZLE_IDENTITY,
					.a = VK_COMPONENT_SWIZZLE_IDENTITY
				},
				.subresourceRange = {.aspectMask = aspect, .baseMipLevel = 0, .levelCount = mipLevels, .baseArrayLayer = 0, .layerCount = 1}
			};

			VkImageView imageView;
			VkResult result = vkCreateImageView(device->device, &viewInfo, nullptr, &imageView);
			VK_ASSERT(result);
			return imageView;
		}

		void transition_image_layout(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels)
		{
			VkImageMemoryBarrier barrier{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.pNext = nullptr,
				.srcAccessMask = 0,
				.dstAccessMask = 0,
				.oldLayout = oldLayout,
				.newLayout = newLayout,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.image = image,
				.subresourceRange = {
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 0,
					.levelCount = mipLevels,
					.baseArrayLayer = 0,
					.layerCount = 1
				},
			};

			VkPipelineStageFlags srcStage;
			VkPipelineStageFlags dstStage;

			if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
				barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
				if (has_stencil_component(format)) {
					barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
				}
			}

			if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			} else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
				barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			} else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			} else {
				fprintf(stderr, "Unsupported layout transition!");
				std::abort();
			}

			vkCmdPipelineBarrier(
				commandBuffer,
				srcStage, dstStage, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);
		}
	} // namespace detail

	Sampler create_image_sampler(Device* device, const SamplerDesc& params)
	{
		const bool useAnisotropy = params.enableAnisotrophy && device->features.samplerAnisotropy;
		float maxAnisotropy = 1.0f;

		if (useAnisotropy) {
			// TODO: cache these properties in device?
			VkPhysicalDeviceProperties properties{};
			vkGetPhysicalDeviceProperties(device->physicalDevice, &properties);

			maxAnisotropy = std::clamp(params.maxAnisotropy, 1.0f, properties.limits.maxSamplerAnisotropy);
		}

		VkSamplerCreateInfo samplerInfo{
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.magFilter = params.filter,
			.minFilter = params.filter,
			.mipmapMode = params.mipmapMode,
			.addressModeU = params.addressMode,
			.addressModeV = params.addressMode,
			.addressModeW = params.addressMode,
			.mipLodBias = 0,
			.anisotropyEnable = useAnisotropy ? VK_TRUE : VK_FALSE,
			.maxAnisotropy = maxAnisotropy,
			.compareEnable = VK_FALSE,
			.compareOp = VK_COMPARE_OP_ALWAYS,
			.minLod = 0.0f,
			.maxLod = VK_LOD_CLAMP_NONE,
			.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
			.unnormalizedCoordinates = VK_FALSE
		};

		VkSampler sampler;
		VkResult result = vkCreateSampler(device->device, &samplerInfo, nullptr, &sampler);
		VK_ASSERT(result);
		return sampler;
	}

	void transition_image(CommandBuffer commands, const Image& image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) {
		detail::transition_image_layout(commands, image.image, image.format, oldLayout, newLayout, mipLevels);
	}

	Image create_image(Device* device, const ImageDesc& params)
	{
		VkDeviceMemory imageMemory;
		VkImage image = detail::create_image(device, params, imageMemory);
		VkImageView imageView = detail::create_image_view(device, image, params.format, params.aspect, params.mipLevels);

		return Image{
			.image = image,
			.imageView = imageView,
			.memory = imageMemory,
			.format = params.format
		};
	}

	Image create_image(Device* device, void* pixels, uint64_t size, const ImageDesc& params)
	{
		Buffer staging = create_buffer(device, {
			.size = size,
			.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		});

		void* data;
		vkMapMemory(device->device, staging.memory, 0, size, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(size));
		vkUnmapMemory(device->device, staging.memory);

		VkDeviceMemory imageMemory;
		VkImage image = detail::create_image(device, params, imageMemory);

		VkCommandBuffer commandBuffer = detail::begin_one_time_commands(device);

		detail::transition_image_layout(commandBuffer, image, params.format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, params.mipLevels);
		copy_buffer_to_image(commandBuffer, staging.buffer, image, params.extent.width, params.extent.height);

		// generate_mipmaps also transfers layout to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		// TODO: also support loading mipmaps instead of generating them at runtime
		generate_mipmaps(device, commandBuffer, image, params.format, params.extent.width, params.extent.height, params.mipLevels);
		detail::end_one_time_commands(device, commandBuffer);

		destroy_buffer(device, staging);

		VkImageView imageView = detail::create_image_view(device, image, params.format, params.aspect, params.mipLevels);

		return Image{
			.image = image,
			.imageView = imageView,
			.memory = imageMemory,
			.format = params.format
		};
	}

	Texture create_texture(Device* device, void* pixels, uint64_t size, const ImageDesc& params)
	{
		Image image = create_image(device, pixels, size, params);
		VkSampler sampler = create_image_sampler(device);

		return Texture{
			.image = image,
			.sampler = sampler
		};
	}

	TextureResource create_texture_resource(Device* device, void* pixels, uint64_t size, const ImageDesc& params, const ResourceSetLayout& resourceLayout)
	{
		Texture texture = create_texture(device, pixels, size, params);
		ResourceSet resourceSet = create_resource_set(device, resourceLayout, {Resource{.texture = &texture}});
		return TextureResource{
			.texture = texture,
			.resourceSet = resourceSet
		};
	}

	void destroy_texture(Device* device, Texture& texture)
	{
		destroy_sampler(device, texture.sampler);
		destroy_image(device, texture.image);
	}

	void destroy_sampler(Device* device, Sampler& sampler)
	{
		vkDestroySampler(device->device, sampler, nullptr);
	}

	void destroy_image(Device* device, Image& image)
	{
		vkDestroyImageView(device->device, image.imageView, nullptr);
		vkDestroyImage(device->device, image.image, nullptr);
		vkFreeMemory(device->device, image.memory, nullptr);
	}

} // namespace gfx
