// Copyright(c) 2026, Jeroen Hoogers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#include "gfx/uniform_buffer.h"
#include "gfx/device.h"
#include <assert.h>

namespace gfx
{
	UniformBuffer create_uniform_buffer(Device* device, uint32_t size)
	{
		uint32_t count = device->framesInFlight;

		BufferDesc bufferDesc{
			.size = size,
			.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		};

		UniformBuffer uniformBuffer = create_mapped_buffers(device, bufferDesc, count);
		return uniformBuffer;
	}

	void destroy_uniform_buffer(Device* device, UniformBuffer& uniformBuffer)
	{
		assert(uniformBuffer.buffers.size() == uniformBuffer.mapped.size());

		for (uint32_t i = 0; i < uniformBuffer.buffers.size(); i++) {
			destroy_buffer(device, uniformBuffer.buffers[i]);
		}
	}
} // namespace gfx
