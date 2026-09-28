#include "gfx/window.h"
#include "gfx/device.h"
#include "gfx/render_target.h"
#include "gfx/swapchain.h"
#include "gfx/sync.h"

namespace gfx
{
	namespace detail
	{
		void init_window(VkInstance instance, Window* window)
		{
			VkSurfaceKHR surface = VK_NULL_HANDLE;

			// create surface
			VkResult result = window->callbacks.create_surface(instance, &surface, window->callbacks.user_data);
			VK_ASSERT(result);

			window->surface = surface;
		}
	} // namespace detail

	Window* create_window(WindowCallbacks callbacks)
	{
		return new Window{
			.callbacks = callbacks
		};
	}

	void destroy_window(Device* device, Window* window)
	{
		for (uint32_t i = 0; i < window->frames.size(); i++) {
			destroy_semaphore(device, &window->frames[i].imageAvailable);
			destroy_fence(device, &window->frames[i].inFlightFence);
		}
		destroy_render_target(device, window->renderTarget);

		destroy_swapchain(device, window->swapchain);

		vkDestroySurfaceKHR(device->instance, window->surface, nullptr);

		delete window;
		window = nullptr;
	}
} // namespace gfx
