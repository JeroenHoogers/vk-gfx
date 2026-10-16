// Copyright(c) 2026, Jeroen Hoogers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#include "gfx/mesh.h"
#include "gfx/buffer.h"
#include "gfx/device.h"

namespace gfx
{
	VertexLayout create_vertex_layout(const std::vector<VertexBinding>& bindings)
	{
		return VertexLayout{.bindings = bindings};
	}

	Mesh create_mesh(Device* device, const MeshData& meshData)
	{
		Buffer vertexBuffer = create_and_upload_buffer(device, meshData.vertices.data, {
			.size = meshData.vertices.size,
			.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		});

		Buffer indexBuffer = {};

		uint32_t vertexCount = meshData.vertices.size / meshData.vertices.stride;
		uint32_t indexCount = 0;

		VkIndexType indexType = VK_INDEX_TYPE_NONE_KHR;
		if (meshData.indices.data) {
			indexBuffer = create_and_upload_buffer(device, meshData.indices.data, {
				.size = meshData.indices.size,
				.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
				.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			});
			indexCount = meshData.indices.size / meshData.indices.stride;

			if (meshData.indices.stride == sizeof(uint32_t)) {
				indexType = VK_INDEX_TYPE_UINT32;
			} else if ((meshData.indices.stride == sizeof(uint16_t))) {
				indexType = VK_INDEX_TYPE_UINT16;
			} else if ((meshData.indices.stride == sizeof(uint8_t))) {
				indexType = VK_INDEX_TYPE_UINT8;
			}
		}

		return Mesh{
			.vertexBuffer = vertexBuffer,
			.indexBuffer = indexBuffer,
			.indexCount = indexCount,
			.vertexCount = vertexCount,
			.indexType = indexType
		};
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

	void destroy_mesh(Device* device, Mesh& mesh)
	{
		destroy_buffer(device, mesh.vertexBuffer);

		if (mesh.indexBuffer.buffer) {
			destroy_buffer(device, mesh.indexBuffer);
		}
	}
} // namespace gfx
