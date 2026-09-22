#include "gfx/buffer.h"
#include "gfx/device.h"
#include "gfx/command_buffer.h"
#include <cstring>

namespace gfx
{
	namespace detail
	{
		uint32_t find_memory_type(Device* device, uint32_t typeFilter, VkMemoryPropertyFlags properties)
		{
			VkPhysicalDeviceMemoryProperties memProperties;
			vkGetPhysicalDeviceMemoryProperties(device->physicalDevice, &memProperties);
			for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
				auto flags = memProperties.memoryTypes[i].propertyFlags;
				if ((typeFilter & (1 << i)) && (flags & properties) == properties) {
					return i;
				}
			}

			fprintf(stderr, "failed to find suitable memory type!");
			std::abort();
		}
	} // namespace

	Buffer* create_buffer(Device* device, const BufferDesc& params)
	{
		VkBufferCreateInfo bufferInfo{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.size = params.size,
			.usage = params.usage,
			.sharingMode = params.sharingMode,
			.queueFamilyIndexCount = 0,
			.pQueueFamilyIndices = nullptr
		};

		VkBuffer buffer;
		VkResult result = vkCreateBuffer(device->device, &bufferInfo, nullptr, &buffer);
		VK_ASSERT(result);

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device->device, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.pNext = nullptr,
			.allocationSize = memRequirements.size,
			.memoryTypeIndex = detail::find_memory_type(device, memRequirements.memoryTypeBits, params.properties),
		};

		VkDeviceMemory deviceMemory;
		result = vkAllocateMemory(device->device, &allocInfo, nullptr, &deviceMemory);
		VK_ASSERT(result);

		result = vkBindBufferMemory(device->device, buffer, deviceMemory, 0);
		VK_ASSERT(result);

		return new Buffer{.buffer = buffer, .memory = deviceMemory, .size = params.size};
	}

	Buffer* create_and_upload_buffer(Device* device, const void* data, const BufferDesc& params)
	{
		Buffer* staging = create_buffer(device, {.size = params.size, .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT, .properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT});

		void* mapped;
		VkResult result = vkMapMemory(device->device, staging->memory, 0, staging->size, 0, &mapped);
		VK_ASSERT(result);
		memcpy(mapped, data, static_cast<size_t>(staging->size));
		vkUnmapMemory(device->device, staging->memory);

		Buffer* buffer = create_buffer(device, {.size = params.size, .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | params.usage, .properties = params.properties});

		copy_buffer(device, staging, buffer);

		destroy_buffer(device, staging);

		return buffer;
	}

	void copy_buffer(Device* device, Buffer* src, Buffer* dst)
	{
		VkCommandBuffer commands = detail::begin_one_time_commands(device);

		VkBufferCopy copyRegion{
			.srcOffset = 0,
			.dstOffset = 0,
			.size = src->size
		};

		vkCmdCopyBuffer(commands, src->buffer, dst->buffer, 1, &copyRegion);
		detail::end_one_time_commands(device, commands);
	}

	void destroy_buffer(Device* device, Buffer* buffer)
	{
		vkDestroyBuffer(device->device, buffer->buffer, nullptr);
		vkFreeMemory(device->device, buffer->memory, nullptr);

		delete buffer;
		buffer = nullptr;
	}

} // namespace gfx
