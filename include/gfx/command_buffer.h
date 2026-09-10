#pragma once
#include <vulkan/vulkan.h>
#include <cstdint>

namespace gfx
{
	struct Device;

	struct CommandBuffer
	{
		VkCommandBuffer commandBuffer;
	};

	struct Mesh {
		float aa;
	};

	[[nodiscard]] CommandBuffer* begin_commands(Device* device);
	void end_commands(CommandBuffer* commands);

	void draw(CommandBuffer* commands, void* mesh, uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t firstInstance = 0);

} // namespace gfx
