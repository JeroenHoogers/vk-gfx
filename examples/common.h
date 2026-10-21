// Copyright(c) 2026, Jeroen Hoogers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include <fstream>
#include <string>
#include <vector>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <vk_gfx.h>

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 uv;
};

struct UniformBufferObject
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

inline const std::vector<uint32_t> load_shader(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	size_t byteSize = (size_t)file.tellg();

	if (byteSize <= 0 || byteSize % static_cast<std::streamsize>(sizeof(std::uint32_t)) != 0) {
		throw std::runtime_error("invalid SPIR-V file size: " + filename);
	}

	std::vector<std::uint32_t> buffer(static_cast<std::size_t>(byteSize) / sizeof(std::uint32_t));

	file.seekg(0);
	file.read(reinterpret_cast<char*>(buffer.data()), byteSize);

	file.close();

	return buffer;
}

inline gfx::Texture load_image(gfx::Device* device, const std::string& filename)
{
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(filename.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	uint32_t imageSize = texWidth * texHeight * 4;

	if (!pixels) {
		throw std::runtime_error("failed to load texture image!");
	}

	uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
	gfx::Texture texture = gfx::create_texture(device, pixels, imageSize, {.format = VK_FORMAT_R8G8B8A8_SRGB, .extent = {.width = static_cast<uint32_t>(texWidth), .height = static_cast<uint32_t>(texHeight), .depth = 1}, .mipLevels = mipLevels, .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, .properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT});

	stbi_image_free(pixels);

	return texture;
}

inline gfx::Mesh create_cube(gfx::Device* device)
{
	constexpr glm::vec3 positions[] = {
		{-0.5f, -0.5f,  0.5f}, // 0
		{ 0.5f, -0.5f,  0.5f}, // 1
		{ 0.5f,  0.5f,  0.5f}, // 2
		{-0.5f,  0.5f,  0.5f}, // 3
		{-0.5f, -0.5f, -0.5f}, // 4
		{ 0.5f, -0.5f, -0.5f}, // 5
		{ 0.5f,  0.5f, -0.5f}, // 6
		{-0.5f,  0.5f, -0.5f}  // 7
	};

	constexpr glm::vec2 uvs[] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};

	constexpr uint32_t faces[6][4] = {
		{0, 1, 2, 3}, // front
		{5, 4, 7, 6}, // back
		{4, 0, 3, 7}, // left
		{1, 5, 6, 2}, // right
		{3, 2, 6, 7}, // top
		{4, 5, 1, 0}  // bottom
	};

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	vertices.reserve(24);
	indices.reserve(36);

	for (const auto& face : faces) {
		const glm::vec3& p0 = positions[face[0]];
		const glm::vec3& p1 = positions[face[1]];
		const glm::vec3& p2 = positions[face[2]];

		const glm::vec3 normal = glm::normalize(glm::cross(p2 - p0, p1 - p0));

		const uint32_t base = static_cast<uint32_t>(vertices.size());

		// face corners
		for (uint32_t j = 0; j < 4; j++) {
			vertices.push_back({
				positions[face[j]],
				normal,
				uvs[j]}
			);
		}

		// add triangles (0, 1, 2) and (0, 2, 3)
		indices.insert(indices.end(),
			{base + 0, base + 1, base + 2, base + 0, base + 2, base + 3}
		);
	}

	gfx::Mesh cube = gfx::create_mesh(device, {
		.vertices = {
			.data = vertices.data(),
			.count = vertices.size(),
			.stride = sizeof(Vertex)
		},
		.indices = {
			.data = indices.data(),
			.count = indices.size(),
			.type = VK_INDEX_TYPE_UINT32
		}
	});

	return cube;
}
