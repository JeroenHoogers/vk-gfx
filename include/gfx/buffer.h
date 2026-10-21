// Copyright(c) 2026, Jeroen Hoogers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include "types.h" // needed for CommandBuffer

namespace gfx {
	struct Device;

	namespace detail {
		uint32_t find_memory_type(Device* device, uint32_t typeFilter, VkMemoryPropertyFlags properties);
	}

	struct Buffer {
		VkBuffer buffer = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;
		VkDeviceSize size = 0;
		// VkDeviceSize count;
		// uint32_t stride;
	};

	struct MappedBuffer {
		Buffer buffer;
		void* mapped = nullptr;
	};

	typedef std::vector<Buffer> MultiBuffer;

	struct MappedMultiBuffer {
		MultiBuffer buffers;
		std::vector<void*> mapped = {};
	};

	struct BufferDesc{
		uint64_t size;
		// uint64_t count;
		VkBufferUsageFlags usage;
		VkMemoryPropertyFlags properties;
		VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	};

	[[nodiscard]] Buffer create_buffer(Device* device, const BufferDesc& params);
	[[nodiscard]] Buffer create_and_upload_buffer(Device* device, const void* data, const BufferDesc& params);

	[[nodiscard]] MappedBuffer create_mapped_buffer(Device* device, const BufferDesc& params);
	[[nodiscard]] MappedMultiBuffer create_mapped_buffers(Device* device, const BufferDesc& params, uint32_t count);

	[[nodiscard]] MultiBuffer create_buffers(Device* device, const BufferDesc& params, uint32_t count);
	[[nodiscard]] MultiBuffer create_and_upload_buffers(Device* device, const void* data, const BufferDesc& params, uint32_t count);

	void copy_buffer(CommandBuffer commands, const Buffer& src, const Buffer& dst);

	void destroy_buffer(Device* device, const Buffer& buffer);
	void destroy_buffer(Device* device, const MappedBuffer& buffer);
	void destroy_buffers(Device* device, const MultiBuffer& buffer);
}
