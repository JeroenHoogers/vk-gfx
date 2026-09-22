#pragma once
#include <vulkan/vulkan.h>
#include <cstdint>
#include <vector>

namespace gfx
{
	struct Device;
	struct Window;
	struct Fence;
	struct Semaphore;
	struct Mesh;
	struct Buffer;

	struct CommandBuffer
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

	[[nodiscard]] CommandBuffer* begin_commands(Window* window);
	void end_commands(CommandBuffer* commands);

	void draw(CommandBuffer* commands, Mesh* mesh, uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t firstInstance = 0);

	void draw_indexed(CommandBuffer* commands, Mesh* mesh, uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t vertexOffset = 0, uint32_t firstInstance = 0);

	void draw_indirect(CommandBuffer* commands, Mesh* mesh, Buffer* indirectBuffer, uint32_t drawCount, uint32_t offset = 0);
	void submit(Device* device, CommandBuffer* commands); // TODO:

	// TODO: allow for multiple command buffers
	void submit_and_present(Device* device, Window* window, const CommandBuffer* commands);

} // namespace gfx
