#pragma once
#include <cstdint>
#include <vulkan/vulkan.h>

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

	constexpr DynamicStateFlags operator&(DynamicStateFlags lhs, DynamicStateFlags rhs)
	{
		return static_cast<DynamicStateFlags>(
			static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs)
		);
	}

	constexpr DynamicStateFlags operator|(DynamicStateFlags lhs, DynamicStateFlags rhs)
	{
		return static_cast<DynamicStateFlags>(
			static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs)
		);
	}

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
