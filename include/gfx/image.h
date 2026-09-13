#pragma once

#include <vulkan/vulkan.h>

namespace gfx {
	struct Buffer;
	struct Device;

	struct ImageDesc {
		uint64_t size;
		VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
		VkExtent3D extent;
		uint32_t mipLevels = 1;
		VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT;
		VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
	};

	struct Image {
		VkImage image;
		VkDeviceMemory memory;
	};

	Image* create_image(Device* device, void* pixels, const ImageDesc& params);
	void destroy_image(Device* device, Image* image);
}
