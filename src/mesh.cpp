#include "gfx/mesh.h"
#include "gfx/buffer.h"
#include "gfx/device.h"
#include <cstring>

namespace gfx
{
	namespace
	{
		Buffer* create_vertex_buffer(Device* device, const VertexData& vertexData)
		{
			Buffer* staging = create_buffer(device, {
				.size = vertexData.size,
				.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			});

			void* data;
			VkResult result = vkMapMemory(device->device, staging->memory, 0, staging->size, 0, &data);
			VK_ASSERT(result);
			memcpy(data, vertexData.data, static_cast<size_t>(staging->size));
			vkUnmapMemory(device->device, staging->memory);

			Buffer* vertexBuffer = create_buffer(device, {
				.size = vertexData.size,
				.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			});

			copy_buffer(device, staging, vertexBuffer);

			destroy_buffer(device, staging);

			return vertexBuffer;
		}

	} // namespace

	VertexLayout create_vertex_layout(const std::vector<VertexBinding>& bindings)
	{
		return VertexLayout{.bindings = bindings};
	}

	Mesh create_mesh(Device* device, const MeshData& meshData) {
		Buffer* vertexBuffer = create_vertex_buffer(device, meshData.vertices);
		Buffer* indexBuffer = nullptr;
		if(meshData.indices.data) {
			//indexBuffer = create_index_buffer(device, indexData);
		}

		return Mesh { .vertexBuffer = vertexBuffer, .indexBuffer = indexBuffer };
	}

	void destroy_mesh(Device* device, Mesh* mesh) {
		destroy_buffer(device, mesh->vertexBuffer);

		if (mesh->indexBuffer) {
			destroy_buffer(device, mesh->indexBuffer);
		}
	}
} // namespace gfx
