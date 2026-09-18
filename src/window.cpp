#include "gfx/window.h"
#include "gfx/device.h"
#include "gfx/swapchain.h"

namespace gfx {
	namespace detail {
		Window* create_window(VkInstance instance, WindowCallbacks* callbacks) {
			VkSurfaceKHR surface = VK_NULL_HANDLE;

			// create surface
			VkResult result = callbacks->create_surface(instance, &surface, callbacks->user_data);
			VK_ASSERT(result);

			return new Window{
				.callbacks = callbacks,
				.surface = surface
			};
		}

		void destroy_window(Device* device, Window* window) {
			destroy_swapchain(device, window->swapchain);

			vkDestroySurfaceKHR(device->instance, window->surface, nullptr);

			delete window;
			window = nullptr;
		}
	}
}
