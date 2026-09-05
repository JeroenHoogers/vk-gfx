// TODO: not needed

#pragma once
#include <vulkan/vulkan.h>

namespace gfx
{
	struct WindowCallbacks {
		// used to query window specific instance extensions
	    VkResult (VKAPI_PTR *get_required_instance_extensions)(
			uint32_t* count,
			const char** names,
	        void* user_data
	    );

		// surface creation callback
	    VkResult (VKAPI_PTR *create_surface)(
	        VkInstance instance,
	        VkSurfaceKHR* surface,
	        void* user_data
	    );

		void* user_data;
	};
}
