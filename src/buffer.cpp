// Copyright(c) 2026, Jeroen Hoogers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

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

	Buffer create_buffer(Device* device, const BufferDesc& params)
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


		VmaAllocationCreateInfo allocInfo{};
		allocInfo.flags = params.allocFlags;
		allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
		allocInfo.requiredFlags = params.requiredProperties;
		allocInfo.preferredFlags = params.preferredProperties;

		VkBuffer buffer;
		VmaAllocation allocation;
		VkResult result = vmaCreateBuffer(device->vmaAllocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr);
		VK_ASSERT(result);

		return Buffer{.buffer = buffer, .memory = allocation, .size = params.size};
	}

	MappedBuffer create_mapped_buffer(Device* device, const BufferDesc& params) {
		MappedBuffer mappedBuffer {
			.buffer = create_buffer(device, params)
		};

		VkResult result = vmaMapMemory(device->vmaAllocator, mappedBuffer.buffer.memory, &mappedBuffer.mapped);
		VK_ASSERT(result);

		return mappedBuffer;
	}

	MappedMultiBuffer create_mapped_buffers(Device* device, const BufferDesc& params, uint32_t count) {
		MappedMultiBuffer mappedBuffers {
			.buffers = create_buffers(device, params, count),
			.mapped = std::vector<void*>(count)
		};

		for (uint32_t i = 0; i < count; i++) {
			auto& buffer = mappedBuffers.buffers[i];
			VkResult result = vmaMapMemory(device->vmaAllocator, buffer.memory, &mappedBuffers.mapped[i]);
			VK_ASSERT(result);
		}

		return mappedBuffers;
	}

	Buffer create_and_upload_buffer(Device* device, const void* data, const BufferDesc& params)
	{
		// TODO: cache staging buffer? (user provided)
		// TODO: re-use command buffer
		Buffer staging = create_buffer(device, {
			.size = params.size,
			.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			.requiredProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			.preferredProperties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			.allocFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
		});

		void* mapped;
		VkResult result = vmaMapMemory(device->vmaAllocator, staging.memory, &mapped);
		VK_ASSERT(result);
		memcpy(mapped, data, static_cast<size_t>(staging.size));
		vmaFlushAllocation(device->vmaAllocator, staging.memory, 0, staging.size); // only needed if not HOST_COHERENT
		vmaUnmapMemory(device->vmaAllocator, staging.memory);

		Buffer buffer = create_buffer(device, {
			.size = params.size,
			.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | params.usage,
			.requiredProperties = params.requiredProperties,
			.preferredProperties = params.preferredProperties
		});

		VkCommandBuffer commandBuffer = detail::begin_one_time_commands(device);
		copy_buffer(commandBuffer, staging, buffer);
		detail::end_one_time_commands(device, commandBuffer);

		destroy_buffer(device, staging);

		return buffer;
	}

	MultiBuffer create_buffers(Device* device, const BufferDesc& params, uint32_t count)
	{
		MultiBuffer multiBuffer(count);

		for (size_t i = 0; i < count; i++) {
			multiBuffer[i] = create_buffer(device, {
				.size = params.size,
				.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | params.usage,
				.requiredProperties = params.requiredProperties,
				.preferredProperties = params.preferredProperties,
				.allocFlags = params.allocFlags
			});
		}

		return multiBuffer;
	}

	MultiBuffer create_and_upload_buffers(Device* device, const void* data, const BufferDesc& params, uint32_t count)
	{
		// TODO: cache staging buffer? (user provided)
		// TODO: re-use command buffer
		Buffer staging = create_buffer(device, {
			.size = params.size,
			.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			.requiredProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, .preferredProperties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			.allocFlags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
		});

		VkCommandBuffer commandBuffer = detail::begin_one_time_commands(device);
		void* mapped;
		VkResult result = vmaMapMemory(device->vmaAllocator, staging.memory, &mapped);
		VK_ASSERT(result);
		memcpy(mapped, data, static_cast<size_t>(staging.size));
		vmaUnmapMemory(device->vmaAllocator, staging.memory);

		MultiBuffer multiBuffer(count);

		for (size_t i = 0; i < count; i++) {
			multiBuffer[i] = create_buffer(device, {
				.size = params.size,
				.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | params.usage,
				.requiredProperties = params.requiredProperties,
				.preferredProperties = params.preferredProperties
			});
			copy_buffer(commandBuffer, staging, multiBuffer[i]);
		}

		detail::end_one_time_commands(device, commandBuffer);
		destroy_buffer(device, staging);

		return multiBuffer;
	}

	void copy_buffer(CommandBuffer commandBuffer, const Buffer& src, const Buffer& dst)
	{
		VkBufferCopy copyRegion{
			.srcOffset = 0,
			.dstOffset = 0,
			.size = src.size
		};

		vkCmdCopyBuffer(commandBuffer, src.buffer, dst.buffer, 1, &copyRegion);
	}

	void destroy_buffer(Device* device, const Buffer& buffer)
	{
		vmaDestroyBuffer(device->vmaAllocator, buffer.buffer, buffer.memory);
	}

	void destroy_buffers(Device* device, const MultiBuffer& multiBuffer)
	{
		for (size_t i = 0; i < multiBuffer.size(); i++) {
			destroy_buffer(device, multiBuffer[i]);
		}
	}

} // namespace gfx
