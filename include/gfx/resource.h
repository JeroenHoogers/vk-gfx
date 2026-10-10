// Copyright(c) 2026, Jeroen Hoogers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include <vector>
#include <vulkan/vulkan.h>
#include <span>
#include "types.h" // Needed for CommandBuffer, ResourceSet

namespace gfx
{
	struct Device;
	struct Pipeline;
	struct UniformBuffer;
	struct Texture;
	struct Image;
	struct Buffer;
	struct MultiBuffer;

	namespace detail
	{
		VkDescriptorBufferInfo get_buffer_descriptor_info(const Buffer* uniformBuffer, uint32_t offset = 0, VkDeviceSize range = VK_WHOLE_SIZE);
		VkDescriptorImageInfo get_combined_image_sampler_descriptor_info(const Texture* image);
		VkDescriptorImageInfo get_sampled_image_descriptor_info(const Image* image);
		VkDescriptorImageInfo get_sampler_descriptor_info(const Sampler* sampler);

		void bind_descriptor_set(CommandBuffer* commandBuffer, Pipeline* pipeline, VkDescriptorSet descriptorSet);
		VkDescriptorSetLayout create_descriptor_set_layout(Device* device, const std::vector<VkDescriptorSetLayoutBinding>& layoutBindings);
	} // namespace detail

	// TODO: support more
	enum class ResourceType {
		CombinedImageSampler,
		SampledImage,
		Sampler,
		UniformBuffer,
		UniformBufferDynamic,
		StorageBuffer,
		StorageBufferDynamic,
		StorageBuffers
	};

	struct ResourceDesc
	{
		ResourceType type;
		VkShaderStageFlags stages;
	};

	[[nodiscard]] ResourceSetLayout create_resource_set_layout(Device* device, const std::vector<ResourceDesc>& layoutBindings);
	void destroy_resource_set_layout(Device* device, ResourceSetLayout layout);
	void destroy_resource_set_layouts(Device* device, const std::initializer_list<ResourceSetLayout>& layouts);
	void destroy_resource_set_layouts(Device* device, const std::span<ResourceSetLayout>& layouts);

	struct ResourcePoolDesc
	{
		std::vector<VkDescriptorPoolSize> sizes = {};
		uint32_t max_sets = 0;
		VkDescriptorPoolCreateFlags flags = 0;
	};

	struct Resource
	{
		ResourceType type;
		union {
			void* ptr;
			Texture* texture;
			Image* image;
			Sampler* sampler;
			UniformBuffer* uniformBuffer; // per frame in flight
			Buffer* storageBuffer;
			MultiBuffer* storageBuffers; // per frame in flight
		};
	};

	std::vector<ResourceSet> create_resource_sets(Device* device, const ResourceSetLayout layout, const std::vector<Resource>& resources, uint32_t count = 1);
	ResourceSet create_resource_set(Device* device, const ResourceSetLayout layout, const std::vector<Resource>& resources);
	void bind_resource_set(CommandBuffer commandBuffer, Pipeline* pipeline, uint32_t index, const ResourceSet& resourceSet);

	void bind_resource_sets(CommandBuffer commandBuffer, Pipeline* pipeline, std::initializer_list<ResourceSet> resourceSets);
	void bind_resource_sets(CommandBuffer commandBuffer, Pipeline* pipeline, std::span<const ResourceSet> resourceSets);

	[[nodiscard]] ResourcePool create_resource_pool(Device* device, const ResourcePoolDesc& params);
	void destroy_resource_pool(Device* device, ResourcePool pool);
	std::vector<VkDescriptorSet> create_descriptor_sets(Device* device, uint32_t count, VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout);
} // namespace gfx
