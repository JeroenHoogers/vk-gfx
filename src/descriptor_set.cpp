#include "gfx/descriptor_set.h"
#include "gfx/command_buffer.h"
#include "gfx/device.h"
#include "gfx/pipeline.h"
#include <array>

namespace gfx
{
	namespace detail
	{
		void bind_descriptor_set(CommandBuffer* commandBuffer, Pipeline* pipeline, VkDescriptorSet descriptorSet)
		{
			vkCmdBindDescriptorSets(commandBuffer->commandBuffer, pipeline->bindPoint, pipeline->pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
		}

		VkDescriptorSetLayout create_descriptor_set_layout(Device* device, const std::vector<DescriptorSetLayoutBinding>& layoutBindings)
		{
			std::vector<VkDescriptorSetLayoutBinding> bindings(layoutBindings.size());

			for (uint32_t i = 0; i < layoutBindings.size(); i++) {
				bindings[i] = VkDescriptorSetLayoutBinding{
					.binding = i,
					.descriptorType = layoutBindings[i].type,
					.descriptorCount = 1,
					.stageFlags = layoutBindings[i].stageFlags,
					.pImmutableSamplers = nullptr
				};
			}

			VkDescriptorSetLayoutCreateInfo layoutInfo{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.bindingCount = static_cast<uint32_t>(bindings.size()),
				.pBindings = bindings.data()
			};

			VkDescriptorSetLayout descriptorSetLayout;
			VkResult result = vkCreateDescriptorSetLayout(device->device, &layoutInfo, nullptr, &descriptorSetLayout);
			VK_ASSERT(result);

			return descriptorSetLayout;
		}
	} // namespace detail

	VkDescriptorPool create_descriptor_pool(Device* device)
	{
		// TODO: expose poolsizes as an argument
		std::array<VkDescriptorPoolSize, 2> poolSizes{
			VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = static_cast<uint32_t>(device->frames.size())},
			VkDescriptorPoolSize{.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = static_cast<uint32_t>(device->frames.size())}
		};

		VkDescriptorPoolCreateInfo poolInfo{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.maxSets = static_cast<uint32_t>(device->frames.size()),
			.poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
			.pPoolSizes = poolSizes.data()
		};

		VkDescriptorPool descriptorPool;
		VkResult result = vkCreateDescriptorPool(device->device, &poolInfo, nullptr, &descriptorPool);
		VK_ASSERT(result);

		return descriptorPool;
	}

	std::vector<VkDescriptorSet> create_descriptor_sets(Device* device, uint32_t count, VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout)
	{
		std::vector<VkDescriptorSetLayout> layouts(count, descriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.pNext = nullptr,
			.descriptorPool = descriptorPool,
			.descriptorSetCount = static_cast<uint32_t>(count),
			.pSetLayouts = layouts.data()
		};

		std::vector<VkDescriptorSet> descriptorSets(count);
		VkResult result = vkAllocateDescriptorSets(device->device, &allocInfo, descriptorSets.data());
		VK_ASSERT(result);
		return descriptorSets;
	}
} // namespace gfx
