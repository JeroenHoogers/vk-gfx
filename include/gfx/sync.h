#pragma once

#include <vulkan/vulkan.h>

namespace gfx {
	struct Device;

	struct Semaphore{
		VkSemaphore semaphore;
	};

	struct Fence {
		VkFence fence;
	};

	[[nodiscard]] Semaphore create_semaphore(Device* device);
	[[nodiscard]] Fence create_fence(Device* device, bool signaled = true);
	// TODO: add timeline

	void wait_for_fence(Device* device, Fence* fence, uint64_t timeout = UINT64_MAX);
	void reset_fence(Device* device, Fence* fence);

	void destroy_semaphore(Device* device, Semaphore* semaphore);
	void destroy_fence(Device* device, Fence* fence);
}
