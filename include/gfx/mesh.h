#pragma once
#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>

namespace gfx
{
	struct Device;
	struct Buffer;

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

	struct Mesh {
		Buffer* vertexBuffer = nullptr;
		Buffer* indexBuffer = nullptr;
	};

	struct VertexData {
		const void* data = nullptr;
		uint64_t size = 0;
		uint32_t stride = 0;
	};

	struct IndexData {
		const uint32_t* data = nullptr;
		size_t size = 0;
	};

	struct MeshData {
		VertexData vertices = {};
		IndexData indices = {};
	};

	Mesh create_mesh(Device* device, const MeshData& meshData);
	void destroy_mesh(Device* device, Mesh* mesh);

	VertexLayout create_vertex_layout(const std::vector<VertexBinding>& bindings);

} // namespace gfx
