// Copyright(c) 2026, Jeroen Hoogers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#include "gfx/uniform_buffer.h"
#include "gfx/buffer.h"
#include "gfx/command_buffer.h"
#include "gfx/resource.h"
#include "gfx/device.h"
#include <assert.h>

namespace gfx
{
	UniformBuffer* create_uniform_buffer(Device* device, uint32_t size)
	{
		uint32_t count = device->framesInFlight;

		BufferDesc bufferDesc{
			.size = size,
			.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		};

		UniformBuffer* uniformBuffer = new UniformBuffer{
			.mappedMemory = std::vector<void*>(count),
			.buffers = std::vector<Buffer>(count),
		};

		for (size_t i = 0; i < count; i++) {
			uniformBuffer->buffers[i] = create_buffer(device, bufferDesc);

			VkResult result = vkMapMemory(device->device, uniformBuffer->buffers[i].memory, 0, uniformBuffer->buffers[i].size, 0, &uniformBuffer->mappedMemory[i]);
			VK_ASSERT(result);
		}

		return uniformBuffer;
	}

	void destroy_uniform_buffer(Device* device, UniformBuffer* uniformBuffer)
	{
		assert(uniformBuffer->buffers.size() == uniformBuffer->mappedMemory.size());

		for (uint32_t i = 0; i < uniformBuffer->buffers.size(); i++) {
			destroy_buffer(device, uniformBuffer->buffers[i]);
		}

		delete uniformBuffer;
	}

} // namespace gfx
