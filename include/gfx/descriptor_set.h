#pragma once

#include <vector>
#include <vulkan/vulkan.h>

namespace gfx
{
	struct Device;
	struct CommandBuffer;
	struct Pipeline;
	struct UniformBuffer;
	struct Image;

	namespace detail
	{
		VkDescriptorBufferInfo get_uniform_buffer_descriptor_info(UniformBuffer* uniformBuffer);
		VkDescriptorImageInfo get_texture_sampler_descriptor_info(Image* image);

		void bind_descriptor_set(CommandBuffer* commandBuffer, Pipeline* pipeline, VkDescriptorSet descriptorSet);
		VkDescriptorSetLayout create_descriptor_set_layout(Device* device, const std::vector<VkDescriptorSetLayoutBinding>& layoutBindings);
	} // namespace detail

	// TODO: support more
	enum class ResourceType {
		CombinedImageSampler,
		UniformBuffer
		// StorageBuffer
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

	struct Resource
	{
		ResourceType type;
		union {
			void* ptr;
			Image* image;
			UniformBuffer* uniformBuffer;
			// TODO: add more (Sampler, StorageBuffer etc.)
		};
	};

	struct ResourceSet
	{
		std::vector<Resource> resources;
		VkDescriptorSet descriptorSet;
	};

	ResourceSet* create_resource_set(Device* device, ResourceSetLayout* layout, const std::vector<Resource>& resources);
	std::vector<ResourceSet*> create_resource_sets(Device* device, ResourceSetLayout* layout, const std::vector<Resource>& resources, uint32_t count = 1);
	void bind_resource_set(CommandBuffer* commandBuffer, Pipeline* pipeline, uint32_t index, ResourceSet* resourceSet);

	VkDescriptorPool create_descriptor_pool(Device* device);
	std::vector<VkDescriptorSet> create_descriptor_sets(Device* device, uint32_t count, VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout);
} // namespace gfx
