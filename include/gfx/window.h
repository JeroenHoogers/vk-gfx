// TODO: not needed

#pragma once
#include <vulkan/vulkan.h>

namespace gfx
{
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
}
