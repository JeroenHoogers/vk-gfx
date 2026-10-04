#pragma once

#include <vulkan/vulkan.h>
#include "types.h" // Needed for ResourceSet

namespace gfx
{
	struct Buffer;
	struct Device;
	struct ResourceSetLayout;

	struct ImageDesc
	{
		VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
		VkExtent3D extent;
		uint32_t mipLevels = 1;
		VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
		VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
		VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
	};

	namespace detail
	{
		VkImage create_image(Device* device, const ImageDesc& params, VkDeviceMemory& imageMemory);
		VkImageView create_image_view(Device* device, VkImage image, VkFormat format, VkImageAspectFlags aspect, uint32_t mipLevels = 1);
		void transition_image_layout(Device* device, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels = 1);

		// TODO: extract upload function for CPU -> GPU transfer of pixel data
	}
	struct Image
	{
		VkImage image;
		VkImageView imageView;
		VkDeviceMemory memory;
	};

	// struct Sampler {
	// 	VkSampler sampler;
	// };

	struct Texture {
		Image* image;
		VkSampler sampler;
	};


	struct TextureResource {
		Texture* texture;
		ResourceSet resourceSet;
	};

	Image* create_image(Device* device, const ImageDesc& params);
	Image* create_image(Device* device, void* pixels, uint64_t size, const ImageDesc& params);
	Texture* create_texture(Device* device, void* pixels, uint64_t size, const ImageDesc& params);

	TextureResource create_texture_resource(Device* device, void* pixels, uint64_t size, const ImageDesc& params, ResourceSetLayout* resourceLayout);

	void destroy_image(Device* device, Image* image);
	void destroy_texture(Device* device, Texture* texture);
} // namespace gfx
