#pragma once
#include <vulkan/vulkan.h>
#include <vector>

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

	struct MultiBuffer {
		std::vector<Buffer> buffers;
	};

	struct BufferDesc{
		uint64_t size;
		// uint64_t count;
		VkBufferUsageFlags usage;
		VkMemoryPropertyFlags properties;
		VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	};

	Buffer create_buffer(Device* device, const BufferDesc& params);
	Buffer create_and_upload_buffer(Device* device, const void* data, const BufferDesc& params);

	MultiBuffer create_buffers(Device* device, const BufferDesc& params, uint32_t count);
	MultiBuffer create_and_upload_buffers(Device* device, const void* data, const BufferDesc& params, uint32_t count);

	void copy_buffer(Device* device, const Buffer& src, const Buffer& dst);

	void destroy_buffer(Device* device, const Buffer& buffer);
	void destroy_buffer(Device* device, const MultiBuffer& buffer);
}
