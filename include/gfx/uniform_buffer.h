// Copyright(c) 2026, Jeroen Hoogers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include <vulkan/vulkan.h>
#include "buffer.h"

namespace gfx {
	struct Device;
	struct Buffer;

	typedef MappedMultiBuffer UniformBuffer;

	[[nodiscard]] UniformBuffer create_uniform_buffer(Device* device, uint32_t size);

	void destroy_uniform_buffer(Device* device, UniformBuffer& uniformBuffer);
}
