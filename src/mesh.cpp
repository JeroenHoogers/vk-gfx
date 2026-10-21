// Copyright(c) 2026, Jeroen Hoogers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#include "gfx/mesh.h"
#include "gfx/buffer.h"
#include "gfx/device.h"
#include <cassert>
#include <cstring>

namespace gfx
{
	VertexLayout create_vertex_layout(const std::vector<VertexBinding>& bindings)
	{
		return VertexLayout{.bindings = bindings};
	}

	Mesh create_mesh(Device* device, const MeshDesc& params)
	{
		Buffer vertexBuffer = create_and_upload_buffer(device, params.vertices.data, {
			.size = params.vertices.count * params.vertices.stride,
			.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		});

		Buffer indexBuffer = {};

		if (params.indices.data)
		{
			const uint32_t indexStride =
		        params.indices.type == VK_INDEX_TYPE_UINT16 ? 2 :
		        params.indices.type == VK_INDEX_TYPE_UINT32 ? 4 : 0;

			indexBuffer = create_and_upload_buffer(device, params.indices.data, {
				.size = params.indices.count * indexStride,
				.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
				.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			});
		}

		return Mesh{
			.vertexBuffer = vertexBuffer,
			.indexBuffer = indexBuffer,
			.vertexCount = params.vertices.count,
			.indexCount = params.indices.count,
			.indexType = params.indices.type
		};
	}

	DynamicMesh create_dynamic_mesh(Device* device, const MeshDesc& params)
	{
		const uint64_t vertexSize = params.vertices.count * params.vertices.stride;

		MappedBuffer vertexBuffer = create_mapped_buffer(device, {
			.size = vertexSize,
			.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		});

		MappedBuffer indexBuffer = {};
		uint64_t indexSize = 0;

		if (params.indices.count > 0)
		{
			const uint32_t indexStride =
				params.indices.type == VK_INDEX_TYPE_UINT16 ? 2 :
				params.indices.type == VK_INDEX_TYPE_UINT32 ? 4 : 0;

			indexSize = params.indices.count * indexStride;

			indexBuffer = create_mapped_buffer(device, {
				.size = indexSize,
				.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
				.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			});
		}

		return DynamicMesh{
			.vertexBuffer = vertexBuffer,
			.indexBuffer = indexBuffer,
			.vertexCount = params.vertices.count,
			.indexCount = params.indices.count,
			.vertexStride = params.vertices.stride,
			.indexType = params.indices.type,
			.vertexCapacity = vertexSize,
			.indexCapacity = indexSize,
		};
	}

	void update_dynamic_mesh(DynamicMesh& mesh, const void* vertexData, uint32_t vertexCount, const void* indexData, uint32_t indexCount)
	{
		size_t vertexSize = vertexCount * mesh.vertexStride;
		assert(vertexSize <= mesh.vertexCapacity);

		memcpy(mesh.vertexBuffer.mapped, vertexData, vertexSize);
		mesh.vertexCount = vertexCount;

		if(indexData)
		{
			const uint32_t indexStride =
				mesh.indexType == VK_INDEX_TYPE_UINT16 ? 2 :
				mesh.indexType == VK_INDEX_TYPE_UINT32 ? 4 : 0;

			size_t indexSize = indexCount * indexStride;
			assert(indexSize <= mesh.indexCapacity);

			memcpy(mesh.indexBuffer.mapped, indexData, indexSize);
			mesh.indexCount = indexCount;
		}
	}

	void bind_mesh(CommandBuffer commands, const Mesh* mesh)
	{
		VkBuffer vertexBuffers[] = {mesh->vertexBuffer.buffer};
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(commands, 0, 1, vertexBuffers, offsets);
		if(mesh->indexCount > 0) {
			vkCmdBindIndexBuffer(commands, mesh->indexBuffer.buffer, 0, mesh->indexType);
		}
	}

	void bind_dynamic_mesh(CommandBuffer commands, const DynamicMesh* mesh)
	{
		VkBuffer vertexBuffers[] = {mesh->vertexBuffer.buffer.buffer};
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(commands, 0, 1, vertexBuffers, offsets);
		if(mesh->indexCount > 0) {
			vkCmdBindIndexBuffer(commands, mesh->indexBuffer.buffer.buffer, 0, mesh->indexType);
		}
	}

	void destroy_mesh(Device* device, Mesh& mesh)
	{
		destroy_buffer(device, mesh.vertexBuffer);

		if (mesh.indexBuffer.buffer) {
			destroy_buffer(device, mesh.indexBuffer);
		}
	}

	void destroy_dynamic_mesh(Device* device, DynamicMesh& mesh)
	{
		destroy_buffer(device, mesh.vertexBuffer.buffer);

		if (mesh.indexBuffer.buffer.buffer) {
			destroy_buffer(device, mesh.indexBuffer.buffer);
		}
	}
} // namespace gfx
