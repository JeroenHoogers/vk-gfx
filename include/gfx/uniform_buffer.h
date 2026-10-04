#pragma once
#include <vulkan/vulkan.h>
#include <vector>

namespace gfx {
	struct Device;
	struct Window;
	struct Buffer;
	struct Pipeline;

	// turn into multibuffer?
	struct UniformBuffer{
		std::vector<void*> mappedMemory;
		std::vector<Buffer> buffers;
	};

	UniformBuffer* create_uniform_buffer(Device* device, uint32_t size);

	void destroy_uniform_buffer(Device* device, UniformBuffer* uniformBuffer);
}
