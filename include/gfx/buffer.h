#pragma once
#include "vulkan/vulkan.h"

namespace gfx {
	struct Device;

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
		VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	};

	Buffer* create_buffer(Device* device, const BufferDesc& params);

	void destroy_buffer(Device* device, Buffer* buffer);
}
