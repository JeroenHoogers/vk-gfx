#include "gfx/uniform_buffer.h"
#include "gfx/buffer.h"
#include "gfx/command_buffer.h"
#include "gfx/descriptor_set.h"
#include "gfx/device.h"
#include <assert.h>

namespace gfx
{
	UniformBuffer* create_uniform_buffer(Device* device, const UniformBufferDesc& params)
	{
		uint32_t frameCount = device->frames.size();
		VkDescriptorSetLayout descriptorSetLayout = detail::create_descriptor_set_layout(device, {
			detail::DescriptorSetLayoutBinding{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .stageFlags = VK_SHADER_STAGE_VERTEX_BIT},
			detail::DescriptorSetLayoutBinding{.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT}
		});

		BufferDesc bufferDesc{
			.size = params.size,
			.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		};

		std::vector<VkDescriptorSet> descriptorSets = create_descriptor_sets(device, frameCount, device->descriptorPool, descriptorSetLayout);

		UniformBuffer* uniformBuffer = new UniformBuffer{
			.descriptorSetLayout = descriptorSetLayout,
			.mappedMemory = std::vector<void*>(frameCount),
			.buffers = std::vector<Buffer*>(frameCount),
			.descriptorSets = std::move(descriptorSets)
		};

		for (size_t i = 0; i < frameCount; i++) {
			uniformBuffer->buffers[i] = create_buffer(device, bufferDesc);

			VkResult result = vkMapMemory(device->device, uniformBuffer->buffers[i]->memory, 0, uniformBuffer->buffers[i]->size, 0, &uniformBuffer->mappedMemory[i]);
			VK_ASSERT(result);

			VkDescriptorBufferInfo bufferInfo{
				.buffer = uniformBuffer->buffers[i]->buffer,
				.offset = 0,
				.range = params.size
			};

			VkWriteDescriptorSet descriptorWrite{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.pNext = nullptr,
				.dstSet = uniformBuffer->descriptorSets[i],
				.dstBinding = 0,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.pImageInfo = nullptr,
				.pBufferInfo = &bufferInfo,
				.pTexelBufferView = nullptr
			};

			vkUpdateDescriptorSets(device->device, 1, &descriptorWrite, 0, nullptr);
		}

		return uniformBuffer;
	}

	void bind_uniform_buffer(Device* device, Pipeline* pipeline, CommandBuffer* commands, UniformBuffer* uniformBuffer)
	{
		detail::bind_descriptor_set(commands, pipeline, uniformBuffer->descriptorSets[device->currentFrame]);
	}

	void destroy_uniform_buffer(Device* device, UniformBuffer* uniformBuffer)
	{
		assert(uniformBuffer->buffers.size() == uniformBuffer->mappedMemory.size());

		for (uint32_t i = 0; i < uniformBuffer->buffers.size(); i++) {
			destroy_buffer(device, uniformBuffer->buffers[i]);
		}

		vkDestroyDescriptorSetLayout(device->device, uniformBuffer->descriptorSetLayout, nullptr);
		delete uniformBuffer;
	}

} // namespace gfx
