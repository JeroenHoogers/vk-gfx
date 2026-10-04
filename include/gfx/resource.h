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
	struct Buffer;
	struct MultiBuffer;

	namespace detail
	{
		VkDescriptorBufferInfo get_uniform_buffer_descriptor_info(UniformBuffer* uniformBuffer);
		VkDescriptorImageInfo get_texture_sampler_descriptor_info(Texture* image);

		void bind_descriptor_set(CommandBuffer* commandBuffer, Pipeline* pipeline, VkDescriptorSet descriptorSet);
		VkDescriptorSetLayout create_descriptor_set_layout(Device* device, const std::vector<VkDescriptorSetLayoutBinding>& layoutBindings);
	} // namespace detail

	// TODO: support more
	enum class ResourceType {
		CombinedImageSampler,
		UniformBuffer,
		StorageBuffer,
		StorageBuffers
	};

	struct ResourceDesc
	{
		ResourceType type;
		VkShaderStageFlags stages;
	};

	struct ResourceSetLayout
	{
		std::vector<ResourceDesc> bindings;
		VkDescriptorSetLayout descriptorSetLayout;
	};

	ResourceSetLayout* create_resource_set_layout(Device* device, const std::vector<ResourceDesc>& layoutBindings);
	void destroy_resource_set_layout(Device* device, ResourceSetLayout* layout);
	void destroy_resource_set_layouts(Device* device, const std::vector<ResourceSetLayout*>& layouts);

	struct ResourcePoolDesc
	{
		std::vector<VkDescriptorPoolSize> sizes = {};
		uint32_t max_sets = 0;
		VkDescriptorPoolCreateFlags flags = 0;
	};

	struct ResourcePool
	{
		VkDescriptorPool descriptorPool;
	};

	struct Resource
	{
		ResourceType type;
		union {
			void* ptr;
			Texture* texture;
			UniformBuffer* uniformBuffer; // per frame in flight
			Buffer* storageBuffer;
			MultiBuffer* storageBuffers;
			// TODO: add more (Sampler, StorageBuffer etc.)
		};
	};

	std::vector<ResourceSet> create_resource_sets(Device* device, ResourceSetLayout* layout, const std::vector<Resource>& resources, uint32_t count = 1);
	ResourceSet create_resource_set(Device* device, ResourceSetLayout* layout, const std::vector<Resource>& resources);
	void bind_resource_set(CommandBuffer commandBuffer, Pipeline* pipeline, uint32_t index, const ResourceSet& resourceSet);

	void bind_resource_sets(CommandBuffer commandBuffer, Pipeline* pipeline, std::initializer_list<ResourceSet> resourceSets);
	void bind_resource_sets(CommandBuffer commandBuffer, Pipeline* pipeline, std::span<const ResourceSet> resourceSets);

	ResourcePool* create_resource_pool(Device* device, const ResourcePoolDesc& params);
	void destroy_resource_pool(Device* device, ResourcePool* pool);
	std::vector<VkDescriptorSet> create_descriptor_sets(Device* device, uint32_t count, VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout);
} // namespace gfx
