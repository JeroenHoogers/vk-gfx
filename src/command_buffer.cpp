#include "gfx/command_buffer.h"
#include "gfx/device.h"
#include "gfx/swapchain.h"
#include "gfx/sync.h"

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

		void present(Device* device, Semaphore* signalSemaphore) {
			VkSemaphore signalSemaphores[] = {signalSemaphore->semaphore};
			VkSwapchainKHR swapChains[] = {device->swapchain->swapchain};

			VkPresentInfoKHR presentInfo{
				.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
				.pNext = nullptr,
				.waitSemaphoreCount = 1,
				.pWaitSemaphores = signalSemaphores,
				.swapchainCount = 1,
				.pSwapchains = swapChains,
				.pImageIndices = &device->swapchain->imageIndex,
				.pResults = nullptr
			};

			VkResult result = vkQueuePresentKHR(device->presentQueue, &presentInfo);
			if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
				recreate_swapchain(device, device->swapchain);
				return;
			} else {
				VK_ASSERT(result);
			}
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

		vkResetCommandBuffer(commandBuffer, 0); // TODO: do we need this?

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		CommandBuffer* pCommandBuffer = new CommandBuffer{
			.commandBuffer = commandBuffer
		};

		return pCommandBuffer;
	};

	void end_commands(CommandBuffer* commands) {
		VkResult result = vkEndCommandBuffer(commands->commandBuffer);
		VK_ASSERT(result);
	}

	void submit_and_present(Device* device, const CommandBuffer* commands, Fence* inflightFence, Semaphore* waitSemaphore, Semaphore* signalSemaphore) {
		VkSemaphore waitSemaphores[] = {waitSemaphore->semaphore};
		VkSemaphore signalSemaphores[] = {signalSemaphore->semaphore};

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

		VkResult result = vkQueueSubmit(device->graphicsQueue, 1, &submitInfo, inflightFence->fence);
		VK_ASSERT(result);

		present(device, signalSemaphore);
	}

	void draw(CommandBuffer* commands, void* mesh, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
		// TODO: bind mesh
		vkCmdDraw(commands->commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
	}
} // namespace gfx
