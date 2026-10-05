// Copyright(c) 2026, Jeroen Hoogers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#include "gfx/command_buffer.h"
#include "gfx/buffer.h"
#include "gfx/device.h"
#include "gfx/mesh.h"
#include "gfx/swapchain.h"
#include "gfx/sync.h"
#include "gfx/uniform_buffer.h"

namespace gfx
{
	namespace
	{
		void present(Device* device, Window* window, VkSemaphore waitSemaphore)
		{
			VkSemaphore waitSemaphores[] = {waitSemaphore};
			VkSwapchainKHR swapChains[] = {window->swapchain->swapchain};

			VkPresentInfoKHR presentInfo{
				.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
				.pNext = nullptr,
				.waitSemaphoreCount = 1,
				.pWaitSemaphores = waitSemaphores,
				.swapchainCount = 1,
				.pSwapchains = swapChains,
				.pImageIndices = &window->swapchain->imageIndex,
				.pResults = nullptr
			};

			VkResult result = vkQueuePresentKHR(device->presentQueue.handle, &presentInfo);
			if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window->swapchain->resized) {
				window->swapchain->resized = false;
				recreate_swapchain(device, window);
				return;
			} else {
				VK_ASSERT(result);
			}
		}

	} // namespace

	namespace detail
	{
		VkCommandBuffer create_command_buffer(Device* device, VkCommandPool pool)
		{
			VkCommandBufferAllocateInfo allocInfo{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
				.pNext = nullptr,
				.commandPool = pool,
				.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				.commandBufferCount = 1
			};

			VkCommandBuffer commandBuffer;
			VkResult result = vkAllocateCommandBuffers(device->device, &allocInfo, &commandBuffer);
			VK_ASSERT(result);
			return commandBuffer;
		}

		std::vector<VkCommandBuffer> create_command_buffers(Device* device, VkCommandPool pool, uint32_t count)
		{
			VkCommandBufferAllocateInfo allocInfo{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
				.pNext = nullptr,
				.commandPool = pool,
				.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				.commandBufferCount = count
			};

			std::vector<VkCommandBuffer> commandBuffers(count);
			VkResult result = vkAllocateCommandBuffers(device->device, &allocInfo, commandBuffers.data());
			VK_ASSERT(result);
			return commandBuffers;
		}

		void begin_commands(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags flags)
		{
			VkCommandBufferBeginInfo beginInfo{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
				.pNext = nullptr,
				.flags = flags,
				.pInheritanceInfo = nullptr
			};

			vkBeginCommandBuffer(commandBuffer, &beginInfo);
		}

		VkCommandBuffer begin_one_time_commands(Device* device)
		{
			VkCommandBuffer commandBuffer = create_command_buffer(device, device->transientPool);
			begin_commands(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

			return commandBuffer;
		}

		void end_one_time_commands(Device* device, VkCommandBuffer commandBuffer)
		{
			vkEndCommandBuffer(commandBuffer);

			VkSubmitInfo submitInfo{};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &commandBuffer;

			// TODO: create transferqueue?
			vkQueueSubmit(device->graphicsQueue.handle, 1, &submitInfo, VK_NULL_HANDLE);
			vkQueueWaitIdle(device->graphicsQueue.handle);

			vkFreeCommandBuffers(device->device, device->transientPool, 1, &commandBuffer);
		}

	} // namespace detail

	CommandBuffer create_command_buffer(Device* device) {
		return detail::create_command_buffer(device, device->commandPool);
	}

	std::vector<CommandBuffer> create_command_buffers(Device* device, uint32_t count) {
		return detail::create_command_buffers(device, device->commandPool, count);
	}

	CommandBuffer begin_commands(Window* window)
	{
		CommandBuffer commandBuffer = window->frames[window->currentFrame].commands;
		vkResetCommandBuffer(commandBuffer, 0);
		detail::begin_commands(commandBuffer);

		return commandBuffer;
	};

	CommandBuffer begin_commands(CommandBuffer commandBuffer)
	{
		vkResetCommandBuffer(commandBuffer, 0);
		detail::begin_commands(commandBuffer);

		return commandBuffer;
	};

	void end_commands(CommandBuffer commands)
	{
		VkResult result = vkEndCommandBuffer(commands);
		VK_ASSERT(result);
	}

	void submit(const CommandBuffer commands, const SubmitParams& params)
	{
		VkSubmitInfo submitInfo{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr,
			.waitSemaphoreCount = static_cast<uint32_t>(params.waitSemaphores.size()),
			.pWaitSemaphores = params.waitSemaphores.data(),
			.pWaitDstStageMask = params.waitStages.data(),
			.commandBufferCount = 1,
			.pCommandBuffers = &commands,
			.signalSemaphoreCount = static_cast<uint32_t>(params.signalSemaphores.size()),
			.pSignalSemaphores = params.signalSemaphores.data()
		};

		VkResult result = vkQueueSubmit(params.queue.handle, 1, &submitInfo, params.completedFence);
		VK_ASSERT(result);
	}

	void submit_and_present(Device* device, const SwapchainFrame& frame, const CommandBuffer commands, const SubmitParams& params)
	{
		submit(commands, params);
		present(device, frame.window, params.signalSemaphores[0]); // TODO: signal semaphore 0 is assumed to be render finished

		frame.window->currentFrame = (frame.index + 1) % static_cast<uint32_t>(frame.window->frames.size());
	}

	void submit_and_present(Device* device, const SwapchainFrame& frame, const CommandBuffer commands)
	{
		Frame& frameInFlight = frame.window->frames[frame.index];

		VkSemaphore signalSemaphore = frame.renderFinished;
		VkSemaphore signalSemaphores[] = {signalSemaphore};
		VkSemaphore waitSemaphores[] = {frameInFlight.imageAvailable};

		VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

		VkSubmitInfo submitInfo{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = waitSemaphores,
			.pWaitDstStageMask = waitStages,
			.commandBufferCount = 1,
			.pCommandBuffers = &commands,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = signalSemaphores
		};

		VkResult result = vkQueueSubmit(device->graphicsQueue.handle, 1, &submitInfo, frameInFlight.inFlightFence);
		VK_ASSERT(result);

		present(device, frame.window, signalSemaphore);

		frame.window->currentFrame = (frame.index + 1) % static_cast<uint32_t>(frame.window->frames.size());
	}

	void dispatch(CommandBuffer commands, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
		vkCmdDispatch(commands, groupCountX, groupCountY, groupCountZ);
	}

	void dispatch_indirect(CommandBuffer commands, const Buffer& indirectBuffer, uint32_t offset) {
		vkCmdDispatchIndirect(commands, indirectBuffer.buffer, offset);
	}

	void draw(CommandBuffer commands, const Mesh* mesh, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
	{
		// bind mesh
		if (mesh != nullptr) {
			VkBuffer vertexBuffers[] = {mesh->vertexBuffer.buffer};
			VkDeviceSize offsets[] = {0};
			vkCmdBindVertexBuffers(commands, 0, 1, vertexBuffers, offsets);
		}

		vkCmdDraw(commands, vertexCount, instanceCount, firstVertex, firstInstance);
	}

	void draw_indexed(CommandBuffer commands, const Mesh* mesh, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
	{
		// bind mesh
		VkBuffer vertexBuffers[] = {mesh->vertexBuffer.buffer};
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(commands, 0, 1, vertexBuffers, offsets);

		vkCmdBindIndexBuffer(commands, mesh->indexBuffer.buffer, 0, mesh->indexType);
		vkCmdDrawIndexed(commands, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}

	void draw_indirect(CommandBuffer commands, const Mesh* mesh, const Buffer& indirectBuffer, uint32_t drawCount, uint32_t offset)
	{
		// bind mesh
		VkBuffer vertexBuffers[] = {mesh->vertexBuffer.buffer};
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(commands, 0, 1, vertexBuffers, offsets);

		vkCmdDrawIndirect(commands, indirectBuffer.buffer, offset, drawCount, sizeof(VkDrawIndirectCommand));
	}

	void draw_indexed_indirect(CommandBuffer commands, const Mesh* mesh, const Buffer& indirectBuffer, uint32_t drawCount, uint32_t offset)
	{
		// bind mesh
		VkBuffer vertexBuffers[] = {mesh->vertexBuffer.buffer};
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(commands, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commands, mesh->indexBuffer.buffer, 0, mesh->indexType);

		vkCmdDrawIndexedIndirect(commands, indirectBuffer.buffer, offset, drawCount, sizeof(VkDrawIndexedIndirectCommand));
	}
} // namespace gfx
