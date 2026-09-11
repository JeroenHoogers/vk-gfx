#pragma once
#include <vulkan/vulkan.h>
#include <cstdint>

namespace gfx
{
	struct Device;
	struct Fence;
	struct Semaphore;

	struct CommandBuffer
	{
		VkCommandBuffer commandBuffer;
	};

	namespace detail {
		VkCommandBuffer create_command_buffer(Device* device);
	}


	[[nodiscard]] CommandBuffer* begin_commands(Device* device);
	void end_commands(CommandBuffer* commands);

	void draw(CommandBuffer* commands, void* mesh, uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t firstInstance = 0);

	void submit(Device* device, CommandBuffer* commands); // TODO:

	// TODO: allow for multiple command buffers
	void submit_and_present(Device* device, const CommandBuffer* commands, Fence* inflightFence, Semaphore* waitSemaphore, Semaphore* signalSemaphore);

} // namespace gfx
