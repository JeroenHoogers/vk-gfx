// Copyright(c) 2026, Jeroen Hoogers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include <vulkan/vulkan.h>
#include "types.h" // Needed for ResourceSet, ResourceSetLayout
#include <vector>

namespace gfx
{
	struct Device;

	struct SamplerDesc
	{
		VkFilter filter = VK_FILTER_LINEAR;
		VkSamplerMipmapMode mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		bool enableAnisotrophy = true;
		float maxAnisotropy = 16.0f;
		VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	};

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
		void transition_image_layout(VkCommandBuffer commands, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels = 1);

		// TODO: extract upload function for CPU -> GPU transfer of pixel data
	}

	struct Image
	{
		VkImage image;
		VkImageView imageView;
		VkDeviceMemory memory;
		VkFormat format;
	};

	typedef std::vector<Image> MultiImage;

	struct Texture {
		Image image;
		Sampler sampler;
	};

	struct TextureResource {
		Texture texture;
		ResourceSet resourceSet;
	};

	Image create_image(Device* device, const ImageDesc& params);
	Image create_image(Device* device, void* pixels, uint64_t size, const ImageDesc& params);
	Sampler create_image_sampler(Device* device, const SamplerDesc& params = {});
	Texture create_texture(Device* device, void* pixels, uint64_t size, const ImageDesc& params);

	TextureResource create_texture_resource(Device* device, void* pixels, uint64_t size, const ImageDesc& params, ResourceSetLayout* resourceLayout);

	void transition_image(CommandBuffer commands, const Image& image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels = 1);

	void destroy_image(Device* device, Image& image);
	void destroy_sampler(Device* device, Sampler& sampler);
	void destroy_texture(Device* device, Texture& texture);
} // namespace gfx
