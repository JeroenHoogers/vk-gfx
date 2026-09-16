#pragma once
#include <vector>
#include <string>
#include <fstream>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <vk_gfx.h>

struct Vertex {
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 uv;
};

struct UniformBufferObject {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

inline gfx::Image* load_image(gfx::Device* device, const std::string& filename) {
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(filename.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	uint32_t imageSize = texWidth * texHeight * 4;

	if (!pixels) {
		throw std::runtime_error("failed to load texture image!");
	}

	gfx::Image* image = gfx::create_image(device, pixels, {
		.size = imageSize,
		.format = VK_FORMAT_R8G8B8A8_SRGB,
		.extent = {
			.width = static_cast<uint32_t>(texWidth),
			.height = static_cast<uint32_t>(texHeight),
			.depth = 1},
		.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	});

	stbi_image_free(pixels);

	return image;
}

inline const std::vector<char> load_shader(const std::string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}
