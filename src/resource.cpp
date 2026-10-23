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
			case ResourceType::UniformBufferDynamic:
				return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			case ResourceType::SampledImage:
			case ResourceType::SampledImages:
				return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			case ResourceType::Sampler:
				return VK_DESCRIPTOR_TYPE_SAMPLER;
			case ResourceType::CombinedImageSampler:
				return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			case ResourceType::StorageBufferDynamic:
				return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
			case ResourceType::StorageBuffer:
			case ResourceType::StorageBuffers:
				return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			}

			assert(false && "Invalid resource type value");
			return VK_DESCRIPTOR_TYPE_MAX_ENUM;
		}

		void bind_descriptor_set(CommandBuffer commandBuffer, Pipeline* pipeline, VkDescriptorSet descriptorSet)
		{
			vkCmdBindDescriptorSets(commandBuffer, pipeline->bindPoint, pipeline->layout.handle, 0, 1, &descriptorSet, 0, nullptr);
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

		VkDescriptorBufferInfo get_buffer_descriptor_info(const Buffer* buffer, uint32_t offset, VkDeviceSize range)
		{
			return VkDescriptorBufferInfo{
				.buffer = buffer->buffer,
				.offset = offset,
				.range = range,
			};
		}

		VkDescriptorImageInfo get_sampler_descriptor_info(const Sampler* sampler)
		{
			return VkDescriptorImageInfo{
				.sampler = *sampler,
				.imageView = VK_NULL_HANDLE,
				.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED
			};
		}

		VkDescriptorImageInfo get_sampled_image_descriptor_info(const Image* image)
		{
			return VkDescriptorImageInfo{
				.sampler = VK_NULL_HANDLE,
				.imageView = image->imageView,
				.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			};
		}

		VkDescriptorImageInfo get_combined_image_sampler_descriptor_info(const Texture* texture)
		{
			return VkDescriptorImageInfo{
				.sampler = texture->sampler,
				.imageView = texture->image.imageView,
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

	ResourceSetLayout create_resource_set_layout(Device* device, const std::vector<ResourceBindingDesc>& layoutBindings)
	{
		std::vector<VkDescriptorSetLayoutBinding> bindings(layoutBindings.size());

		for (uint32_t i = 0; i < layoutBindings.size(); i++) {
			bindings[i] = VkDescriptorSetLayoutBinding{
				.binding = i,
				.descriptorType = detail::to_descriptor_type(layoutBindings[i].type),
				.descriptorCount = layoutBindings[i].count,
				.stageFlags = layoutBindings[i].stages,
				.pImmutableSamplers = nullptr
			};
		}

		return ResourceSetLayout {
			.handle = detail::create_descriptor_set_layout(device, bindings),
			.bindings = layoutBindings,
		};
	}

	void destroy_resource_set_layout(Device* device, ResourceSetLayout layout)
	{
		vkDestroyDescriptorSetLayout(device->device, layout.handle, nullptr);
	}

	void destroy_resource_set_layouts(Device* device, const std::initializer_list<ResourceSetLayout>& layouts) {
		for (auto& layout : layouts) {
			vkDestroyDescriptorSetLayout(device->device, layout.handle, nullptr);
		}
	}

	void destroy_resource_set_layouts(Device* device, const std::span<ResourceSetLayout>& layouts) {
		for (auto& layout : layouts) {
			vkDestroyDescriptorSetLayout(device->device, layout.handle, nullptr);
		}
	}

	std::vector<ResourceSet> allocate_resource_sets(Device* device, const ResourceSetLayout& layout, uint32_t count)
	{
		std::vector<VkDescriptorSetLayout> layouts(count, layout.handle);

		VkDescriptorSetAllocateInfo allocInfo{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.pNext = nullptr,
			.descriptorPool = device->resourcePool,
			.descriptorSetCount = static_cast<uint32_t>(count),
			.pSetLayouts = layouts.data()
		};

		std::vector<VkDescriptorSet> descriptorSets(count);
		VkResult result = vkAllocateDescriptorSets(device->device, &allocInfo, descriptorSets.data());
		VK_ASSERT(result);

		return descriptorSets;
	}

	ResourceSet allocate_resource_set(Device* device, const ResourceSetLayout& layout)
	{
		VkDescriptorSetAllocateInfo allocInfo{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.pNext = nullptr,
			.descriptorPool = device->resourcePool,
			.descriptorSetCount = 1,
			.pSetLayouts = &layout.handle
		};

		VkDescriptorSet descriptorSet;
		VkResult result = vkAllocateDescriptorSets(device->device, &allocInfo, &descriptorSet);
		VK_ASSERT(result);

		return descriptorSet;
	}

	std::vector<ResourceSet> create_resource_sets(Device* device, const ResourceSetLayout& layout, const std::vector<Resource>& resources, uint32_t count)
	{
		std::vector<ResourceSet> resourceSets = allocate_resource_sets(device, layout, count);
		write_resource_sets(device, layout, resourceSets, resources);

		return resourceSets;
	}

	ResourceSet create_resource_set(Device* device, const ResourceSetLayout& layout, const std::vector<Resource>& resources)
	{
		ResourceSet resourceSet = allocate_resource_set(device, layout);
		write_resource_set(device, layout, resourceSet, resources);
		return resourceSet;
	}

	void write_resource_sets(Device* device, const ResourceSetLayout& layout, const std::vector<ResourceSet>& sets, const std::vector<Resource>& resources) {
		assert(resources.size() == layout.bindings.size());

		const size_t writeCount = sets.size() * resources.size();
		std::vector<VkWriteDescriptorSet> writes(writeCount);

		// we need to allocate these outside the loop so they don't invalidate before we call vkUpdateDescriptorSets,
		// note that only one of the 2 will be used the other will be left empty
		std::vector<VkDescriptorBufferInfo> bufferInfos(writeCount);
		std::vector<VkDescriptorImageInfo> imageInfos(writeCount);

		for (uint32_t i = 0; i < sets.size(); i++) {
			for (uint32_t j = 0; j < resources.size(); j++) {
				const uint32_t index = i * resources.size() + j;

				auto& write = writes[index];
				write = VkWriteDescriptorSet{
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.pNext = nullptr,
					.dstSet = sets[i],
					.dstBinding = layout.bindings[j].binding,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = detail::to_descriptor_type(layout.bindings[j].type),
					.pImageInfo = nullptr,
					.pBufferInfo = nullptr,
					.pTexelBufferView = nullptr,
				};

				switch (layout.bindings[j].type) {
				case ResourceType::UniformBufferDynamic:
				// TODO: allow subrange
				case ResourceType::UniformBuffer:
					bufferInfos[index] = detail::get_buffer_descriptor_info(&resources[j].uniformBuffer->buffers[i]);
					write.pBufferInfo = &bufferInfos[index];
					break;
				case ResourceType::Sampler:
					imageInfos[index] = detail::get_sampler_descriptor_info(resources[j].sampler);
					write.pImageInfo = &imageInfos[index];
					break;
				case ResourceType::SampledImage:
					imageInfos[index] = detail::get_sampled_image_descriptor_info(resources[j].image);
					write.pImageInfo = &imageInfos[index];
					break;
				case ResourceType::SampledImages:
					imageInfos[index] = detail::get_sampled_image_descriptor_info(&resources[j].images->at(i));
					write.pImageInfo = &imageInfos[index];
					break;
				case ResourceType::CombinedImageSampler:
					imageInfos[index] = detail::get_combined_image_sampler_descriptor_info(resources[j].texture);
					write.pImageInfo = &imageInfos[index];
					break;
				case ResourceType::StorageBuffer:
					bufferInfos[index] = detail::get_buffer_descriptor_info(resources[j].storageBuffer);
					write.pBufferInfo = &bufferInfos[index];
					break;
				case ResourceType::StorageBufferDynamic:
					// TODO: allow subrange
				case ResourceType::StorageBuffers:
					bufferInfos[index] = detail::get_buffer_descriptor_info(&resources[j].storageBuffers->at(i));
					write.pBufferInfo = &bufferInfos[index];
					break;
				}
			}
		}

		vkUpdateDescriptorSets(device->device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
	}

	void write_resource_set(Device* device, const ResourceSetLayout& layout, const ResourceSet& resourceSet, const std::vector<Resource>& resources) {
		write_resource_sets(device, layout, {resourceSet}, resources);
	}

	void bind_resource_set(CommandBuffer commandBuffer, Pipeline* pipeline, uint32_t index, const ResourceSet& resourceSet)
	{
		vkCmdBindDescriptorSets(commandBuffer, pipeline->bindPoint, pipeline->layout.handle, index, 1, &resourceSet, 0, nullptr);
	}

	void bind_resource_sets(CommandBuffer commandBuffer, Pipeline* pipeline, std::initializer_list<ResourceSet> resourceSets) {
		// too restrictive?
		VkDescriptorSet handles[8];
	    assert(resourceSets.size() <= std::size(handles));

	    size_t i = 0;
	    for (const ResourceSet& resourceSet : resourceSets) {
			handles[i++] = resourceSet;
		}

		vkCmdBindDescriptorSets(commandBuffer, pipeline->bindPoint, pipeline->layout.handle, 0, static_cast<uint32_t>(resourceSets.size()), handles, 0, nullptr);
	}

	void bind_resource_sets(CommandBuffer commandBuffer, Pipeline* pipeline, std::span<const ResourceSet> resourceSets) {
		vkCmdBindDescriptorSets(commandBuffer, pipeline->bindPoint, pipeline->layout.handle, 0, static_cast<uint32_t>(resourceSets.size()), resourceSets.data(), 0, nullptr);
	}

} // namespace gfx
