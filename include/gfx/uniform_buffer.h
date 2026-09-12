#pragma once
#include <vulkan/vulkan.h>
#include <vector>

namespace gfx {
	struct Device;
	struct Buffer;
	struct Pipeline;
	struct CommandBuffer;

	struct UniformBufferDesc{
		uint32_t size;
		VkShaderStageFlags stageFlags;
	};

	struct UniformBuffer{
		VkDescriptorSetLayout descriptorSetLayout;

		std::vector<void*> mappedMemory;
		std::vector<Buffer*> buffers;
		std::vector<VkDescriptorSet> descriptorSets;
	};

	UniformBuffer* create_uniform_buffer(Device* device, const UniformBufferDesc& params);
	void bind_uniform_buffer(Device* device, Pipeline* pipeline, CommandBuffer* commands, UniformBuffer* uniformBuffer);

	void destroy_uniform_buffer(Device* device, UniformBuffer* uniformBuffer);
}
