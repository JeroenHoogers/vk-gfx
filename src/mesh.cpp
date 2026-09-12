#include "gfx/mesh.h"
#include "gfx/buffer.h"
#include "gfx/device.h"
#include <cstring>

namespace gfx
{
	namespace
	{
		Buffer* create_and_upload_buffer(Device* device, const BufferData& bufferData, VkBufferUsageFlags usage = 0)
		{
			Buffer* staging = create_buffer(device, {.size = bufferData.size, .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT, .properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT});

			void* data;
			VkResult result = vkMapMemory(device->device, staging->memory, 0, staging->size, 0, &data);
			VK_ASSERT(result);
			memcpy(data, bufferData.data, static_cast<size_t>(staging->size));
			vkUnmapMemory(device->device, staging->memory);

			Buffer* buffer = create_buffer(device, {.size = bufferData.size, .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage, .properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT});

			copy_buffer(device, staging, buffer);

			destroy_buffer(device, staging);

			return buffer;
		}
	} // namespace

	VertexLayout create_vertex_layout(const std::vector<VertexBinding>& bindings)
	{
		return VertexLayout{.bindings = bindings};
	}

	Mesh create_mesh(Device* device, const MeshData& meshData)
	{
		Buffer* vertexBuffer = create_and_upload_buffer(device, meshData.vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
		Buffer* indexBuffer = nullptr;

		VkIndexType indexType = VK_INDEX_TYPE_NONE_KHR;
		if (meshData.indices.data) {
			indexBuffer = create_and_upload_buffer(device, meshData.indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
			if (meshData.indices.stride == sizeof(uint32_t)) {
				indexType = VK_INDEX_TYPE_UINT32;
			} else if ((meshData.indices.stride == sizeof(uint16_t))) {
				indexType = VK_INDEX_TYPE_UINT16;
			} else if ((meshData.indices.stride == sizeof(uint8_t))) {
				indexType = VK_INDEX_TYPE_UINT8;
			}
		}

		return Mesh{.vertexBuffer = vertexBuffer, .indexBuffer = indexBuffer, .indexType = indexType};
	}

	void destroy_mesh(Device* device, Mesh* mesh)
	{
		destroy_buffer(device, mesh->vertexBuffer);

		if (mesh->indexBuffer) {
			destroy_buffer(device, mesh->indexBuffer);
		}
	}
} // namespace gfx
