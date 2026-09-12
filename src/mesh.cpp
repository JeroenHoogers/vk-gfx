#include "gfx/mesh.h"
#include "gfx/buffer.h"
#include "gfx/device.h"
#include <cstring>

namespace gfx
{
	namespace
	{
		// TODO: make internal?
		Buffer* create_vertex_buffer(Device* device, const VertexData& vertexData)
		{
			Buffer* buffer = create_buffer(device, {
				.size = vertexData.size,
				.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				.sharingMode = VK_SHARING_MODE_EXCLUSIVE
			});

			void* data;
			VkResult result = vkMapMemory(device->device, buffer->memory, 0, buffer->size, 0, &data);
			VK_ASSERT(result);
			memcpy(data, vertexData.data, static_cast<size_t>(buffer->size));
			vkUnmapMemory(device->device, buffer->memory);

			return buffer;
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
