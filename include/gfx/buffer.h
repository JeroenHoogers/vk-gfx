#pragma once
#include <vulkan/vulkan.h>

namespace gfx {
	struct Device;

	namespace detail {
		uint32_t find_memory_type(Device* device, uint32_t typeFilter, VkMemoryPropertyFlags properties);
	}

	struct Buffer {
		VkBuffer buffer;
		VkDeviceMemory memory;
		VkDeviceSize size;
		// VkDeviceSize count;
		// uint32_t stride;
	};

	struct BufferDesc{
		uint64_t size;
		// uint64_t count;
		VkBufferUsageFlags usage;
		VkMemoryPropertyFlags properties;
		VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	};

	Buffer* create_buffer(Device* device, const BufferDesc& params);

	void copy_buffer(Device* device, Buffer* src, Buffer* dst);

	void destroy_buffer(Device* device, Buffer* buffer);
}
