#include "gfx/buffer.h"
#include "gfx/device.h"
#include "gfx/command_buffer.h"

namespace gfx
{
	namespace
	{
		uint32_t find_memory_type(Device* device, uint32_t typeFilter, VkMemoryPropertyFlags properties)
		{
			VkPhysicalDeviceMemoryProperties memProperties;
			vkGetPhysicalDeviceMemoryProperties(device->physicalDevice, &memProperties);
			for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
				if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
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
			.memoryTypeIndex = find_memory_type(device, memRequirements.memoryTypeBits, params.properties),
		};

		VkDeviceMemory deviceMemory;
		result = vkAllocateMemory(device->device, &allocInfo, nullptr, &deviceMemory);
		VK_ASSERT(result);

		result = vkBindBufferMemory(device->device, buffer, deviceMemory, 0);
		VK_ASSERT(result);

		return new Buffer{.buffer = buffer, .memory = deviceMemory, .size = params.size};
	}

	void copy_buffer(Device* device, Buffer* src, Buffer* dst)
	{
		VkCommandBuffer commands = detail::create_command_buffer(device, device->transientPool);

		VkBufferCopy copyRegion{
			.srcOffset = 0,
			.dstOffset = 0,
			.size = src->size
		};

		detail::begin_commands(commands);
		vkCmdCopyBuffer(commands, src->buffer, dst->buffer, 1, &copyRegion);
		vkEndCommandBuffer(commands);
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commands;

		// TODO: create transferqueue?
		vkQueueSubmit(device->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(device->graphicsQueue);

		vkFreeCommandBuffers(device->device, device->transientPool, 1, &commands);
	}

	void destroy_buffer(Device* device, Buffer* buffer)
	{
		vkDestroyBuffer(device->device, buffer->buffer, nullptr);
		vkFreeMemory(device->device, buffer->memory, nullptr);

		delete buffer;
		buffer = nullptr;
	}

} // namespace gfx
