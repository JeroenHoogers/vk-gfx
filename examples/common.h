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

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <unordered_map>

#include <vk_gfx.h>

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 uv;

	bool operator==(const Vertex& other) const
	{
		return pos == other.pos && uv == other.uv && color == other.color;
	}
};

namespace std
{
	template <>
	struct hash<Vertex>
	{
		size_t operator()(Vertex const& vertex) const
		{
			return ((hash<glm::vec3>()(vertex.pos) ^
					 (hash<glm::vec3>()(vertex.color) << 1)) >>
					1) ^
				   (hash<glm::vec2>()(vertex.uv) << 1);
		}
	};
} // namespace std

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
	gfx::Texture texture = gfx::create_texture(device, pixels, imageSize, {
		.format = VK_FORMAT_R8G8B8A8_SRGB,
		.extent = {
			.width = static_cast<uint32_t>(texWidth),
			.height = static_cast<uint32_t>(texHeight),
			.depth = 1
		},
		.mipLevels = mipLevels,
		.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	});

	stbi_image_free(pixels);

	return texture;
}

inline gfx::Mesh load_model(gfx::Device* device, const std::string& filename)
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename.c_str())) {
		throw std::runtime_error(err);
	}

	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			Vertex vertex{
				.pos = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				},
				.color = {1.0f, 1.0f, 1.0f},
				.uv = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
				}
			};

			if (uniqueVertices.count(vertex) == 0) {
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}

			indices.push_back(uniqueVertices[vertex]);
		}
	}

	gfx::Mesh model = gfx::create_mesh(device, {.vertices{.data = vertices.data(), .size = sizeof(Vertex) * vertices.size(), .stride = sizeof(Vertex)}, .indices{.data = indices.data(), .size = sizeof(uint32_t) * indices.size(), .stride = sizeof(uint32_t)}});

	return model;
}
