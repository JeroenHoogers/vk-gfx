#pragma once
#include <vulkan/vulkan.h>
#include <vector>

namespace gfx {
	struct Device;
	struct Window;
	struct Buffer;
	struct Pipeline;
	struct CommandBuffer;

	struct UniformBuffer{
		std::vector<void*> mappedMemory;
		std::vector<Buffer*> buffers;
	};

	UniformBuffer* create_uniform_buffer(Device* device, uint32_t size);
	void bind_uniform_buffer(Device* device, Pipeline* pipeline, CommandBuffer* commands, UniformBuffer* uniformBuffer);

	void destroy_uniform_buffer(Device* device, UniformBuffer* uniformBuffer);
}
