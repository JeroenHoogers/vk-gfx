#include "gfx/command_buffer.h"
#include "gfx/device.h"
#include "gfx/swapchain.h"
#include "gfx/sync.h"
#include "gfx/mesh.h"
#include "gfx/buffer.h"

namespace gfx
{
	namespace
	{
		void present(Device* device, VkSemaphore signalSemaphore)
		{
			VkSemaphore signalSemaphores[] = {signalSemaphore};
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

		void begin_commands(VkCommandBuffer commandBuffer){
			VkCommandBufferBeginInfo beginInfo{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
				.pNext = nullptr,
				.flags = 0,
				.pInheritanceInfo = nullptr
			};

			vkBeginCommandBuffer(commandBuffer, &beginInfo);
		}

	} // namespace detail

	CommandBuffer* begin_commands(Device* device)
	{
		CommandBuffer* commandBuffer = device->frames[device->currentFrame].commands;
		vkResetCommandBuffer(commandBuffer->commandBuffer, 0);
		detail::begin_commands(commandBuffer->commandBuffer);

		return commandBuffer;
	};

	void end_commands(CommandBuffer* commands)
	{
		VkResult result = vkEndCommandBuffer(commands->commandBuffer);
		VK_ASSERT(result);
	}

	void submit_and_present(Device* device, const CommandBuffer* commands)
	{
		Frame& frame = device->frames[device->currentFrame];

		uint32_t imageIndex = device->swapchain->imageIndex;
		VkSemaphore signalSemaphore = device->swapchain->images[imageIndex].renderFinished;
		VkSemaphore signalSemaphores[] = { signalSemaphore };
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

		VkResult result = vkQueueSubmit(device->graphicsQueue, 1, &submitInfo, frame.fence.fence);
		VK_ASSERT(result);

		present(device, signalSemaphore);

		device->currentFrame = (device->currentFrame + 1) % static_cast<uint32_t>(device->frames.size());
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
