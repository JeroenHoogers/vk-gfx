// Copyright(c) 2026, Jeroen Hoogers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

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
		uint64_t vertexCount = 0;
		uint64_t indexCount = 0;
		VkIndexType indexType = VK_INDEX_TYPE_UINT32;
	};

	struct DynamicMesh
	{
		MappedBuffer vertexBuffer = {};
		MappedBuffer indexBuffer = {};
		uint64_t vertexCount = 0;
		uint64_t indexCount = 0;

		uint32_t vertexStride = 0;
		VkIndexType indexType = VK_INDEX_TYPE_UINT32;

		VkDeviceSize vertexCapacity = 0; // bytes allocated
		VkDeviceSize indexCapacity = 0;  // bytes allocated
	};

	struct VertexBufferDesc
	{
		const void* data = nullptr;
		uint64_t count = 0;
		uint32_t stride = 0;
	};

	struct IndexBufferDesc
	{
		const void* data = nullptr;
		uint64_t count = 0;
		VkIndexType type = VK_INDEX_TYPE_UINT32;
	};

	struct MeshDesc
	{
		VertexBufferDesc vertices = {};
		IndexBufferDesc indices = {};
	};

	VertexLayout create_vertex_layout(const std::vector<VertexBinding>& bindings);

	[[nodiscard]] Mesh create_mesh(Device* device, const MeshDesc& meshData);
	[[nodiscard]] DynamicMesh create_dynamic_mesh(Device* device, const MeshDesc& dynamicMeshParams);

	void update_dynamic_mesh(DynamicMesh& mesh, const void* vertexData, uint32_t vertexCount, const void* indexData = nullptr, uint32_t indexCount = 0);

	void bind_mesh(CommandBuffer commands, const Mesh* mesh);

	void destroy_mesh(Device* device, Mesh& mesh);
	void destroy_dynamic_mesh(Device* device, DynamicMesh& mesh);
} // namespace gfx
