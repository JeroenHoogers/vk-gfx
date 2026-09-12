#include "gfx/descriptor_set.h"
#include "gfx/device.h"
#include "gfx/pipeline.h"
#include "gfx/command_buffer.h"

namespace gfx
{
	namespace detail
	{
		void bind_descriptor_set(CommandBuffer* commandBuffer, Pipeline* pipeline, VkDescriptorSet descriptorSet) {
			vkCmdBindDescriptorSets(commandBuffer->commandBuffer, pipeline->bindPoint, pipeline->pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
		}

		VkDescriptorSetLayout create_descriptor_set_layout(Device* device, VkDescriptorType type, VkShaderStageFlags stageFlags)
		{
			VkDescriptorSetLayoutBinding layoutBinding{
				.binding = 0,
				.descriptorType = type,
				.descriptorCount = 1,
				.stageFlags = stageFlags,
				.pImmutableSamplers = nullptr
			};

			VkDescriptorSetLayoutCreateInfo layoutInfo{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.bindingCount = 1,
				.pBindings = &layoutBinding,
			};

			VkDescriptorSetLayout descriptorSetLayout;
			VkResult result = vkCreateDescriptorSetLayout(device->device, &layoutInfo, nullptr, &descriptorSetLayout);
			VK_ASSERT(result);

			return descriptorSetLayout;
		}
	}

	VkDescriptorPool create_descriptor_pool(Device* device)
	{
		VkDescriptorPoolSize poolSize{
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = static_cast<uint32_t>(device->frames.size())
		};

		VkDescriptorPoolCreateInfo poolInfo{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.maxSets = static_cast<uint32_t>(device->frames.size()),
			.poolSizeCount = 1,
			.pPoolSizes = &poolSize
		};

		VkDescriptorPool descriptorPool;
		VkResult result = vkCreateDescriptorPool(device->device, &poolInfo, nullptr, &descriptorPool);
		VK_ASSERT(result);

		return descriptorPool;
	}

	std::vector<VkDescriptorSet> create_descriptor_sets(Device* device, VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout)
	{
		uint32_t frameCount = device->frames.size();
		std::vector<VkDescriptorSetLayout> layouts(frameCount, descriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.pNext = nullptr,
			.descriptorPool = descriptorPool,
			.descriptorSetCount = static_cast<uint32_t>(frameCount),
			.pSetLayouts = layouts.data()
		};

		std::vector<VkDescriptorSet> descriptorSets(frameCount);
		VkResult result = vkAllocateDescriptorSets(device->device, &allocInfo, descriptorSets.data());
		VK_ASSERT(result);
		return descriptorSets;
	}
} // namespace gfx
