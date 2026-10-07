// Copyright(c) 2026, Jeroen Hoogers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#include "gfx/resource.h"
#include "gfx/buffer.h"
#include "gfx/command_buffer.h"
#include "gfx/device.h"
#include "gfx/image.h"
#include "gfx/pipeline.h"
#include "gfx/uniform_buffer.h"
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
			case ResourceType::StorageBuffers:
				return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			}

			assert(false && "Invalid resource type value");
			return VK_DESCRIPTOR_TYPE_MAX_ENUM;
		}

		void bind_descriptor_set(CommandBuffer commandBuffer, Pipeline* pipeline, VkDescriptorSet descriptorSet)
		{
			vkCmdBindDescriptorSets(commandBuffer, pipeline->bindPoint, pipeline->layout.layout, 0, 1, &descriptorSet, 0, nullptr);
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

		VkDescriptorBufferInfo get_buffer_descriptor_info(const Buffer* buffer)
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

	ResourcePool create_resource_pool(Device* device, const ResourcePoolDesc& params)
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

		return descriptorPool;
	}

	void destroy_resource_pool(Device* device, ResourcePool pool)
	{
		vkDestroyDescriptorPool(device->device, pool, nullptr);
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

	ResourceSetLayout create_resource_set_layout(Device* device, const std::vector<ResourceDesc>& layoutBindings)
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

		return layout;
	}

	void destroy_resource_set_layout(Device* device, ResourceSetLayout layout)
	{
		vkDestroyDescriptorSetLayout(device->device, layout, nullptr);
	}

	void destroy_resource_set_layouts(Device* device, const std::initializer_list<ResourceSetLayout>& layouts) {
		for (auto& layout : layouts) {
			vkDestroyDescriptorSetLayout(device->device, layout, nullptr);
		}
	}

	void destroy_resource_set_layouts(Device* device, const std::span<ResourceSetLayout>& layouts) {
		for (auto& layout : layouts) {
			vkDestroyDescriptorSetLayout(device->device, layout, nullptr);
		}
	}

	std::vector<ResourceSet> create_resource_sets(Device* device, ResourceSetLayout layout, const std::vector<Resource>& resources, uint32_t count)
	{
		std::vector<VkDescriptorSetLayout> layouts(count, layout);
		VkDescriptorSetAllocateInfo allocInfo{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.pNext = nullptr,
			.descriptorPool = device->resourcePool,
			.descriptorSetCount = static_cast<uint32_t>(count),
			.pSetLayouts = layouts.data()
		};

		std::vector<VkWriteDescriptorSet> writes(count * resources.size());

		// we need to allocate these outside the loop so they don't invalidate before we call vkUpdateDescriptorSets,
		// note that only one of the 2 will be used the other will be left empty
		std::vector<VkDescriptorBufferInfo> bufferInfos(writes.size());
		std::vector<VkDescriptorImageInfo> imageInfos(writes.size());

		std::vector<ResourceSet> descriptorSets(count);
		VkResult result = vkAllocateDescriptorSets(device->device, &allocInfo, descriptorSets.data());
		VK_ASSERT(result);

		for (uint32_t i = 0; i < count; i++) {
			for (uint32_t j = 0; j < resources.size(); j++) {
				const uint32_t index = i * resources.size() + j;

				auto& write = writes[index];
				write = VkWriteDescriptorSet{
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
					bufferInfos[index] = detail::get_buffer_descriptor_info(&resources[j].uniformBuffer->buffers[i]);
					write.pBufferInfo = &bufferInfos[index];
					break;
				case ResourceType::CombinedImageSampler:
					imageInfos[index] = detail::get_texture_sampler_descriptor_info(resources[j].texture);
					write.pImageInfo = &imageInfos[index];
					break;
				case ResourceType::StorageBuffer:
					bufferInfos[index] = detail::get_buffer_descriptor_info(resources[j].storageBuffer);
					write.pBufferInfo = &bufferInfos[index];
					break;
				case ResourceType::StorageBuffers:
					bufferInfos[index] = detail::get_buffer_descriptor_info(&resources[j].storageBuffers->buffers[i]);
					write.pBufferInfo = &bufferInfos[index];
					break;
				}
			}
		}

		vkUpdateDescriptorSets(device->device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

		std::vector<ResourceSet> resourceSets(count, {VK_NULL_HANDLE});

		return descriptorSets;
	}

	ResourceSet create_resource_set(Device* device, const ResourceSetLayout layout, const std::vector<Resource>& resources)
	{
		return create_resource_sets(device, layout, resources)[0];
	}

	void bind_resource_set(CommandBuffer commandBuffer, Pipeline* pipeline, uint32_t index, const ResourceSet& resourceSet)
	{
		vkCmdBindDescriptorSets(commandBuffer, pipeline->bindPoint, pipeline->layout.layout, index, 1, &resourceSet, 0, nullptr);
	}

	void bind_resource_sets(CommandBuffer commandBuffer, Pipeline* pipeline, std::initializer_list<ResourceSet> resourceSets) {
		// too restrictive?
		VkDescriptorSet handles[8];
	    assert(resourceSets.size() <= std::size(handles));

	    size_t i = 0;
	    for (const ResourceSet& resourceSet : resourceSets) {
			handles[i++] = resourceSet;
		}

		vkCmdBindDescriptorSets(commandBuffer, pipeline->bindPoint, pipeline->layout.layout, 0, static_cast<uint32_t>(resourceSets.size()), handles, 0, nullptr);
	}

	void bind_resource_sets(CommandBuffer commandBuffer, Pipeline* pipeline, std::span<const ResourceSet> resourceSets) {
		vkCmdBindDescriptorSets(commandBuffer, pipeline->bindPoint, pipeline->layout.layout, 0, static_cast<uint32_t>(resourceSets.size()), resourceSets.data(), 0, nullptr);
	}

} // namespace gfx
