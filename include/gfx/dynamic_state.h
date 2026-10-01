#pragma once
#include <cstdint>
#include <vulkan/vulkan.h>
#include "detail/bitflags.h"

namespace gfx
{
	// TODO: how to handle user defined dynamic states?
	enum class DynamicStateFlags : uint32_t {
		None = 0,
		Viewport = 1 << 0,
		Scissor = 1 << 1,
		CullMode = 1 << 2,
		FrontFace = 1 << 3,
		PrimitiveTopology = 1 << 4,
		LineWidth = 1 << 5
	};

	template <>
	struct detail::enable_bitmask_operators<DynamicStateFlags> : std::true_type {};

	struct DynamicState
	{
		VkViewport viewport{}; // TODO: support multiple viewports
		VkRect2D scissor{}; // TODO: support multiple scissors
		VkCullModeFlags cullMode{};
		VkFrontFace frontFace{};
		VkPrimitiveTopology primitiveTopology{};
		float lineWidth = 1.0f;
	};
} // namespace gfx
