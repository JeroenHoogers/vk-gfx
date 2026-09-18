#pragma once

#include <vector>
#include <vulkan/vulkan.h>
#include "sync.h"

namespace gfx
{
	struct Device;
	struct Swapchain;
	struct RenderTarget;
	struct CommandBuffer;
	struct Image;

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

	struct Frame {
		CommandBuffer* commands;
		Image* depthImage = nullptr;
		Fence inFlightFence;
		Semaphore imageAvailable;
	};

	struct Window {
		WindowCallbacks* callbacks;
		VkSurfaceKHR surface;
		Swapchain* swapchain = nullptr;
		RenderTarget* renderTarget = nullptr;
		std::vector<Frame> frames = {};
		uint32_t currentFrame = 0;
	};

	namespace detail {
		Window* create_window(VkInstance instance, WindowCallbacks* callbacks);
		void destroy_window(Device* device, Window* window);
	}
}
