#pragma once
#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>
#include "buffer.h" // Required for Buffer

namespace gfx
{
	struct Device;

	struct VertexAttribute
	{
		VkFormat format;
		uint32_t offset;
	};

	struct VertexBinding
	{
		uint32_t stride;
		std::vector<VertexAttribute> attributes;
		VkVertexInputRate rate = VK_VERTEX_INPUT_RATE_VERTEX;
	};

	struct VertexLayout
	{
		std::vector<VertexBinding> bindings;
	};

	struct Mesh
	{
		Buffer vertexBuffer = {};
		Buffer indexBuffer = {};
		uint32_t indexCount = 0;
		uint32_t vertexCount = 0;
		VkIndexType indexType = VK_INDEX_TYPE_UINT16;
	};

	struct BufferData
	{
		const void* data = nullptr;
		uint64_t size = 0;
		uint32_t stride = 0; // TODO: we might be able to remove this
	};

	struct MeshData
	{
		BufferData vertices = {};
		BufferData indices = {};
	};

	Mesh create_mesh(Device* device, const MeshData& meshData);
	void destroy_mesh(Device* device, Mesh* mesh);

	VertexLayout create_vertex_layout(const std::vector<VertexBinding>& bindings);

} // namespace gfx
