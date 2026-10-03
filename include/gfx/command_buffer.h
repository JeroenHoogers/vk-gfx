#pragma once
#include <vulkan/vulkan.h>
#include <cstdint>
#include <vector>
#include "device.h" // needed for Queue
#include "buffer.h" // needed for Buffer
#include "sync.h" // needed for Semaphore / Fence

namespace gfx
{
	struct Device;
	struct Window;
	struct Mesh;

	struct CommandBuffer // TODO: turn into typedef?
	{
		VkCommandBuffer commandBuffer;
	};

	namespace detail {
		[[nodiscard]] VkCommandBuffer create_command_buffer(Device* device, VkCommandPool pool);
		std::vector<VkCommandBuffer> create_command_buffers(Device* device, VkCommandPool pool, uint32_t count = 1);
		void begin_commands(VkCommandBuffer commands, VkCommandBufferUsageFlags flags = 0);

		VkCommandBuffer begin_one_time_commands(Device* device);
		void end_one_time_commands(Device* device, VkCommandBuffer commandBuffer);
	}

	struct SubmitParams { // TODO: avoid vector allocs?
		Queue& queue;
		std::vector<Semaphore> waitSemaphores = {};
		std::vector<Semaphore> signalSemaphores = {};
		std::vector<Fence> completedFence = {};
	};

	[[nodiscard]] CommandBuffer create_command_buffer(Device* device);
	[[nodiscard]] CommandBuffer* begin_commands(Window* window);
	CommandBuffer* begin_commands(CommandBuffer* commandBuffer);
	void end_commands(CommandBuffer* commands);

	void dispatch(CommandBuffer* commands, uint32_t groupCountX, uint32_t groupCountY = 1, uint32_t groupCountZ = 1);
	void draw(CommandBuffer* commands, const Mesh* mesh, uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t firstInstance = 0);

	void draw_indexed(CommandBuffer* commands, const Mesh* mesh, uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t vertexOffset = 0, uint32_t firstInstance = 0);

	void draw_indirect(CommandBuffer* commands, const Mesh* mesh, const Buffer& indirectBuffer, uint32_t drawCount, uint32_t offset = 0);
	void draw_indexed_indirect(CommandBuffer* commands, const Mesh* mesh, const Buffer& indirectBuffer, uint32_t drawCount, uint32_t offset = 0);

	void submit(const CommandBuffer* commands, const SubmitParams& params);

	// TODO: allow for multiple command buffers
	void submit_and_present(Device* device, Window* window, const CommandBuffer* commands);

} // namespace gfx
