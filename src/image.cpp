#include "gfx/image.h"
#include "gfx/buffer.h"
#include "gfx/command_buffer.h"
#include "gfx/device.h"
#include <cstring>

namespace gfx
{
	namespace
	{
		void copy_buffer_to_image(Device* device, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
		{
			VkCommandBuffer commandBuffer = detail::begin_one_time_commands(device);
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
			detail::end_one_time_commands(device, commandBuffer);
		}

		void transition_image_layout(Device* device, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
		{
			VkCommandBuffer commandBuffer = detail::begin_one_time_commands(device);

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
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = 1
				},
			};

			VkPipelineStageFlags srcStage;
			VkPipelineStageFlags dstStage;

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
			} else {
				fprintf(stderr, "Unsupported layout transition!");
				std::abort();
			}

			vkCmdPipelineBarrier(
				commandBuffer,
				srcStage, dstStage,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);

			detail::end_one_time_commands(device, commandBuffer);
		}
	} // namespace

	Image* create_image(Device* device, void* pixels, const ImageDesc& params)
	{
		Buffer* staging = create_buffer(device, {.size = params.size, .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT, .properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT});

		void* data;
		vkMapMemory(device->device, staging->memory, 0, params.size, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(params.size));
		vkUnmapMemory(device->device, staging->memory);

		VkImageCreateInfo imageInfo{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = params.format,
			.extent = params.extent,
			.mipLevels = params.mipLevels,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = params.tiling,
			.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | params.usage,
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

		VkDeviceMemory imageMemory;
		result = vkAllocateMemory(device->device, &allocInfo, nullptr, &imageMemory);

		vkBindImageMemory(device->device, image, imageMemory, 0);

		// TODO OPTIMIZE THIS: put these all into a single command buffer and flush
		transition_image_layout(device, image, params.format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		copy_buffer_to_image(device, staging->buffer, image, params.extent.width, params.extent.height);
		transition_image_layout(device, image, params.format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		destroy_buffer(device, staging);

		return new Image{
			.image = image,
			.memory = imageMemory
		};
	}

	void destroy_image(Device* device, Image* image)
	{
		vkDestroyImage(device->device, image->image, nullptr);
		vkFreeMemory(device->device, image->memory, nullptr);
		delete image;
	}

} // namespace gfx
