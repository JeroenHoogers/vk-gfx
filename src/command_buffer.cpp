#include "gfx/command_buffer.h"
#include "gfx/device.h"
#include "gfx/swapchain.h"

namespace gfx
{
	namespace
	{
		VkCommandBuffer create_command_buffer(Device* device) {
			VkCommandBufferAllocateInfo allocInfo{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
				.pNext = nullptr,
				.commandPool = device->commandPool,
				.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				.commandBufferCount = 1
			};

			VkCommandBuffer commandBuffer;
			VkResult result = vkAllocateCommandBuffers(device->device, &allocInfo, &commandBuffer);
			VK_ASSERT(result);
			return commandBuffer;
		}
	} // namespace

	CommandBuffer* begin_commands(Device* device) {
		VkCommandBuffer commandBuffer = create_command_buffer(device);

		VkCommandBufferBeginInfo beginInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.pNext = nullptr,
			.flags = 0,
			.pInheritanceInfo = nullptr
		};

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		// TODO: does this belong here?
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(device->swapchain->extent.width);
		viewport.height = static_cast<float>(device->swapchain->extent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = {0, 0};
		scissor.extent = device->swapchain->extent;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		CommandBuffer* pCommandBuffer = new CommandBuffer{
			.commandBuffer = commandBuffer
		};

		return pCommandBuffer;
	};

	void end_commands(CommandBuffer* commands) {
		VkResult result = vkEndCommandBuffer(commands->commandBuffer);
		VK_ASSERT(result);
	}

	void draw(CommandBuffer* commands, void* mesh, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
		// TODO: bind mesh
		vkCmdDraw(commands->commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
	}
} // namespace gfx
