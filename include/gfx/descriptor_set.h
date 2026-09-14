#pragma once

#include <vector>
#include <vulkan/vulkan.h>

namespace gfx
{
	struct Device;
	struct CommandBuffer;
	struct Pipeline;

	namespace detail
	{
		struct DescriptorSetLayoutBinding
		{
			VkDescriptorType type;
			VkShaderStageFlags stageFlags;
		};

		void bind_descriptor_set(CommandBuffer* commandBuffer, Pipeline* pipeline, VkDescriptorSet descriptorSet);
		VkDescriptorSetLayout create_descriptor_set_layout(Device* device, const std::vector<DescriptorSetLayoutBinding>& layoutBindings);
	} // namespace detail

	VkDescriptorPool create_descriptor_pool(Device* device);
	std::vector<VkDescriptorSet> create_descriptor_sets(Device* device, uint32_t count, VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout);

	void destroy_descriptor_sets(Device* device);
} // namespace gfx
