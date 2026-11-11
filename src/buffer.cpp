// Copyright(c) 2026, Jeroen Hoogers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#include "gfx/buffer.h"
#include "gfx/device.h"
#include "gfx/command_buffer.h"
#include <cstring>
#include <cassert>

namespace gfx
{
	namespace detail
	{
		Buffer create_buffer_impl(Device* device, const BufferDesc& params, VmaAllocationInfo* allocationInfo)
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
			VkResult result = vmaCreateBuffer(device->vmaAllocator, &bufferInfo, &allocInfo, &buffer, &allocation, allocationInfo);
			VK_ASSERT(result);

			return Buffer{.handle = buffer, .allocation = allocation, .size = params.size};
		}
	} // namespace detail

	Buffer create_buffer(Device* device, const BufferDesc& params)
	{
		return detail::create_buffer_impl(device, params, nullptr);
	}

	MappedBuffer create_mapped_buffer(Device* device, const BufferDesc& params)
	{
		assert((params.allocFlags & VMA_ALLOCATION_CREATE_MAPPED_BIT) > 0);

		VmaAllocationInfo allocationInfo;
		Buffer buffer = detail::create_buffer_impl(device, params, &allocationInfo);

		return MappedBuffer {
			.buffer = buffer,
			.mapped = allocationInfo.pMappedData
		};
	}

	MappedMultiBuffer create_mapped_buffers(Device* device, const BufferDesc& params, uint32_t count) {
		MappedMultiBuffer mappedBuffers {
			.buffers = create_buffers(device, params, count),
			.mapped = std::vector<void*>(count)
		};

		for (uint32_t i = 0; i < count; i++) {
			auto& buffer = mappedBuffers.buffers[i];
			VkResult result = vmaMapMemory(device->vmaAllocator, buffer.allocation, &mappedBuffers.mapped[i]);
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
		VkResult result = vmaMapMemory(device->vmaAllocator, staging.allocation, &mapped);
		VK_ASSERT(result);
		memcpy(mapped, data, static_cast<size_t>(staging.size));
		vmaFlushAllocation(device->vmaAllocator, staging.allocation, 0, staging.size); // only needed if not HOST_COHERENT
		vmaUnmapMemory(device->vmaAllocator, staging.allocation);

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
		VkResult result = vmaMapMemory(device->vmaAllocator, staging.allocation, &mapped);
		VK_ASSERT(result);
		memcpy(mapped, data, static_cast<size_t>(staging.size));
		vmaUnmapMemory(device->vmaAllocator, staging.allocation);

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

		vkCmdCopyBuffer(commandBuffer, src.handle, dst.handle, 1, &copyRegion);
	}

	void destroy_buffer(Device* device, MappedBuffer& mappedBuffer)
	{
		destroy_buffer(device, mappedBuffer.buffer);
		mappedBuffer.mapped = nullptr;
	}

	void destroy_buffer(Device* device, Buffer& buffer)
	{
		vmaDestroyBuffer(device->vmaAllocator, buffer.handle, buffer.allocation);
		buffer.handle = VK_NULL_HANDLE;
		buffer.allocation = nullptr;
	}

	void destroy_buffers(Device* device, MultiBuffer& multiBuffer)
	{
		for (size_t i = 0; i < multiBuffer.size(); i++) {
			destroy_buffer(device, multiBuffer[i]);
		}
	}

} // namespace gfx
