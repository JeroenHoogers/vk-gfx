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
		void present(Device* device, Window* window, VkSemaphore signalSemaphore)
		{
			VkSemaphore signalSemaphores[] = {signalSemaphore};
			VkSwapchainKHR swapChains[] = {window->swapchain->swapchain};

			VkPresentInfoKHR presentInfo{
				.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
				.pNext = nullptr,
				.waitSemaphoreCount = 1,
				.pWaitSemaphores = signalSemaphores,
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

	CommandBuffer* begin_commands(Window* window)
	{
		CommandBuffer* commandBuffer = window->frames[window->currentFrame].commands;
		vkResetCommandBuffer(commandBuffer->commandBuffer, 0);
		detail::begin_commands(commandBuffer->commandBuffer);

		return commandBuffer;
	};

	void end_commands(CommandBuffer* commands)
	{
		VkResult result = vkEndCommandBuffer(commands->commandBuffer);
		VK_ASSERT(result);
	}

	void submit_and_present(Device* device, Window* window, const CommandBuffer* commands)
	{
		Frame& frame = window->frames[window->currentFrame];

		uint32_t imageIndex = window->swapchain->imageIndex;
		VkSemaphore signalSemaphore = window->swapchain->images[imageIndex].renderFinished;
		VkSemaphore signalSemaphores[] = {signalSemaphore};
		VkSemaphore waitSemaphores[] = {frame.imageAvailable.semaphore};

		VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

		VkSubmitInfo submitInfo{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = nullptr,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = waitSemaphores,
			.pWaitDstStageMask = waitStages,
			.commandBufferCount = 1,
			.pCommandBuffers = &commands->commandBuffer,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = signalSemaphores
		};

		VkResult result = vkQueueSubmit(device->graphicsQueue.handle, 1, &submitInfo, frame.inFlightFence.fence);
		VK_ASSERT(result);

		present(device, window, signalSemaphore);

		// TODO: should frames in flight be per window?
		window->currentFrame = (window->currentFrame + 1) % static_cast<uint32_t>(window->frames.size());
	}

	void draw(CommandBuffer* commands, Mesh* mesh, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
	{
		// bind mesh
		if (mesh != nullptr) {
			VkBuffer vertexBuffers[] = {mesh->vertexBuffer->buffer};
			VkDeviceSize offsets[] = {0};
			vkCmdBindVertexBuffers(commands->commandBuffer, 0, 1, vertexBuffers, offsets);
		}

		vkCmdDraw(commands->commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
	}

	void draw_indexed(CommandBuffer* commands, Mesh* mesh, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
	{
		// bind mesh
		VkBuffer vertexBuffers[] = {mesh->vertexBuffer->buffer};
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(commands->commandBuffer, 0, 1, vertexBuffers, offsets);

		vkCmdBindIndexBuffer(commands->commandBuffer, mesh->indexBuffer->buffer, 0, mesh->indexType);
		vkCmdDrawIndexed(commands->commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}
} // namespace gfx
