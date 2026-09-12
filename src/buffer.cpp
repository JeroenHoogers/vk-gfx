#include "gfx/buffer.h"
#include "gfx/device.h"

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
			.memoryTypeIndex = find_memory_type(device, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
		};

		VkDeviceMemory deviceMemory;
		result = vkAllocateMemory(device->device, &allocInfo, nullptr, &deviceMemory);
		VK_ASSERT(result);

		result = vkBindBufferMemory(device->device, buffer, deviceMemory, 0);
		VK_ASSERT(result);

		return new Buffer{.buffer = buffer, .memory = deviceMemory, .size = params.size};
	}

	void destroy_buffer(Device* device, Buffer* buffer)
	{
		vkDestroyBuffer(device->device, buffer->buffer, nullptr);
		vkFreeMemory(device->device, buffer->memory, nullptr);

		delete buffer;
		buffer = nullptr;
	}

} // namespace gfx
