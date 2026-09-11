#include "gfx/sync.h"
#include "gfx/device.h"

namespace gfx
{
	Semaphore create_semaphore(Device* device)
	{
		VkSemaphoreCreateInfo semaphoreInfo{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0
		};

		VkSemaphore semaphore;
		VkResult result = vkCreateSemaphore(device->device, &semaphoreInfo, nullptr, &semaphore);
		VK_ASSERT(result);

		return Semaphore{.semaphore = semaphore};
	}

	Fence create_fence(Device* device, bool signaled)
	{
		VkFenceCreateFlagBits flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : static_cast<VkFenceCreateFlagBits>(0);

		VkFenceCreateInfo fenceInfo{
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.pNext = nullptr,
			.flags = flags
		};

		VkFence fence;
		VkResult result = vkCreateFence(device->device, &fenceInfo, nullptr, &fence);
		VK_ASSERT(result);

		return Fence{.fence = fence};
	}

	void wait_for_fence(Device* device, Fence* fence, uint64_t timeout)
	{
		vkWaitForFences(device->device, 1, &fence->fence, VK_TRUE, timeout);
	}

	void reset_fence(Device* device, Fence* fence)
	{
		vkResetFences(device->device, 1, &fence->fence);
	}

	void destroy_semaphore(Device* device, Semaphore* semaphore)
	{
		vkDestroySemaphore(device->device, semaphore->semaphore, nullptr);
	}

	void destroy_fence(Device* device, Fence* fence)
	{
		vkDestroyFence(device->device, fence->fence, nullptr);
	}
} // namespace gfx
