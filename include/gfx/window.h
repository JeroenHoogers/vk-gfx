#pragma once

#include <vulkan/vulkan.h>

namespace gfx
{
	struct Device;
	struct Swapchain;

	struct WindowCallbacks {
		// used to query window specific instance extensions
	    VkResult (*get_required_instance_extensions)(
			uint32_t* count,
			const char** names,
	        void* user_data
	    );

	    void (*get_framebuffer_size)(
	        uint32_t* width,
	        uint32_t* height,
	        void* user_data
	    );

		// surface creation callback
	    VkResult (*create_surface)(
	        VkInstance instance,
	        VkSurfaceKHR* surface,
	        void* user_data
	    );

		void* user_data;
	};

	struct Window {
		WindowCallbacks* callbacks;
		VkSurfaceKHR surface;
		Swapchain* swapchain = nullptr;
	};

	namespace detail {
		Window* create_window(VkInstance instance, WindowCallbacks* callbacks);
		void destroy_window(Device* device, Window* window);
	}
}
