#include "gfx/descriptor_set.h"
#include "gfx/buffer.h"
#include "gfx/command_buffer.h"
#include "gfx/device.h"
#include "gfx/image.h"
#include "gfx/pipeline.h"
#include "gfx/uniform_buffer.h"
#include <array>
#include <assert.h>

namespace gfx
{

	namespace detail
	{
		constexpr VkDescriptorType to_descriptor_type(ResourceType resourceType) noexcept
		{
			switch (resourceType) {
			case ResourceType::UniformBuffer:
				return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			case ResourceType::CombinedImageSampler:
				return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			case ResourceType::StorageBuffer:
				return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			}

			assert(false && "Invalid resource type value");
			return VK_DESCRIPTOR_TYPE_MAX_ENUM;
		}

		void bind_descriptor_set(CommandBuffer* commandBuffer, Pipeline* pipeline, VkDescriptorSet descriptorSet)
		{
			vkCmdBindDescriptorSets(commandBuffer->commandBuffer, pipeline->bindPoint, pipeline->pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
		}

		VkDescriptorSetLayout create_descriptor_set_layout(Device* device, const std::vector<VkDescriptorSetLayoutBinding>& bindings)
		{
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

		VkDescriptorBufferInfo get_buffer_descriptor_info(Buffer* buffer)
		{
			return VkDescriptorBufferInfo{
				.buffer = buffer->buffer,
				.offset = 0,
				.range = buffer->size,
			};
		}

		VkDescriptorImageInfo get_texture_sampler_descriptor_info(Texture* texture)
		{
			return VkDescriptorImageInfo{
				.sampler = texture->sampler,
				.imageView = texture->image->imageView,
				.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			};
		}
	} // namespace detail

	ResourcePool* create_resource_pool(Device* device, const ResourcePoolDesc& params)
	{
		VkDescriptorPoolCreateInfo poolInfo{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.pNext = nullptr,
			.flags = params.flags,
			.maxSets = params.max_sets,
			.poolSizeCount = static_cast<uint32_t>(params.sizes.size()),
			.pPoolSizes = params.sizes.data()
		};

		VkDescriptorPool descriptorPool;
		VkResult result = vkCreateDescriptorPool(device->device, &poolInfo, nullptr, &descriptorPool);
		VK_ASSERT(result);

		return new ResourcePool{
			.descriptorPool = descriptorPool
		};
	}

	void destroy_resource_pool(Device* device, ResourcePool* pool)
	{
		vkDestroyDescriptorPool(device->device, pool->descriptorPool, nullptr);
		delete pool;
		pool = nullptr;
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

	ResourceSetLayout* create_resource_set_layout(Device* device, const std::vector<ResourceDesc>& layoutBindings)
	{
		std::vector<VkDescriptorSetLayoutBinding> bindings(layoutBindings.size());

		for (uint32_t i = 0; i < layoutBindings.size(); i++) {
			bindings[i] = VkDescriptorSetLayoutBinding{
				.binding = i,
				.descriptorType = detail::to_descriptor_type(layoutBindings[i].type),
				.descriptorCount = 1,
				.stageFlags = layoutBindings[i].stages,
				.pImmutableSamplers = nullptr
			};
		}

		VkDescriptorSetLayout layout = detail::create_descriptor_set_layout(device, bindings);

		return new ResourceSetLayout{
			.bindings = layoutBindings,
			.descriptorSetLayout = layout
		};
	}

	void destroy_resource_set_layouts(Device* device, const std::vector<ResourceSetLayout*>& layouts)
	{
		for (auto& layout : layouts) {
			vkDestroyDescriptorSetLayout(device->device, layout->descriptorSetLayout, nullptr);
		}
	}

	void destroy_resource_set_layout(Device* device, ResourceSetLayout* layout)
	{
		vkDestroyDescriptorSetLayout(device->device, layout->descriptorSetLayout, nullptr);
	}

	std::vector<ResourceSet*> create_resource_sets(Device* device, ResourceSetLayout* layout, const std::vector<Resource>& resources, uint32_t count)
	{
		std::vector<VkDescriptorSetLayout> layouts(count, layout->descriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.pNext = nullptr,
			.descriptorPool = device->resourcePool->descriptorPool,
			.descriptorSetCount = static_cast<uint32_t>(count),
			.pSetLayouts = layouts.data()
		};

		std::vector<VkDescriptorSet> descriptorSets(count);
		VkResult result = vkAllocateDescriptorSets(device->device, &allocInfo, descriptorSets.data());
		VK_ASSERT(result);

		for (uint32_t i = 0; i < count; i++) {
			std::vector<VkWriteDescriptorSet> writes(resources.size());
			for (uint32_t j = 0; j < resources.size(); j++) {
				// TODO: maybe create these inside the switch to avoid unnesessary initialization
				VkDescriptorBufferInfo bufferInfo{};
				VkDescriptorImageInfo imageInfo{};

				writes[j] = VkWriteDescriptorSet{
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.pNext = nullptr,
					.dstSet = descriptorSets[i],
					.dstBinding = j,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = detail::to_descriptor_type(resources[j].type),
					.pImageInfo = nullptr,
					.pBufferInfo = nullptr,
					.pTexelBufferView = nullptr,
				};

				switch (resources[j].type) {
				case ResourceType::UniformBuffer:
					bufferInfo = detail::get_buffer_descriptor_info(resources[j].uniformBuffer->buffers[i]);
					writes[j].pBufferInfo = &bufferInfo;
					break;
				case ResourceType::CombinedImageSampler:
					imageInfo = detail::get_texture_sampler_descriptor_info(resources[j].texture);
					writes[j].pImageInfo = &imageInfo;
					break;
				case ResourceType::StorageBuffer:
					bufferInfo = detail::get_buffer_descriptor_info(resources[j].storageBuffer);
					writes[j].pBufferInfo = &bufferInfo;
					break;
				}
			}
			vkUpdateDescriptorSets(device->device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
		}

		std::vector<ResourceSet*> resourceSets(count, nullptr);
		for (uint32_t i = 0; i < descriptorSets.size(); i++) {
			resourceSets[i] = new ResourceSet{
				.resources = resources,
				.descriptorSet = descriptorSets[i]
			};
		}

		return resourceSets;
	}

	ResourceSet* create_resource_set(Device* device, ResourceSetLayout* layout, const std::vector<Resource>& resources)
	{
		return create_resource_sets(device, layout, resources)[0];
	}

	void bind_resource_set(CommandBuffer* commandBuffer, Pipeline* pipeline, uint32_t index, ResourceSet* resourceSet)
	{
		vkCmdBindDescriptorSets(commandBuffer->commandBuffer, pipeline->bindPoint, pipeline->pipelineLayout, index, 1, &resourceSet->descriptorSet, 0, nullptr);
	}
} // namespace gfx
