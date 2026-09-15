#pragma once
#include <vulkan/vulkan.h>
#include <vector>

namespace gfx
{
	struct Device;
	struct CommandBuffer;
	struct VertexLayout;
	struct UniformBuffer;
	struct ResourceSet;
	struct ResourceSetLayout;

	struct RasterizerParams {
		VkPolygonMode polygon_mode = VK_POLYGON_MODE_FILL;
		VkCullModeFlags cull_mode = VK_CULL_MODE_BACK_BIT;
		VkFrontFace front_face = VK_FRONT_FACE_CLOCKWISE;
	};

	struct PipelineParams
	{
		std::vector<char> vertex_shader = {};
		std::vector<char> fragment_shader = {};
		std::vector<char> geometry_shader = {};
		VertexLayout* vertex_layout = nullptr;
		std::vector<ResourceSetLayout*> resource_set_layouts = {};
		RasterizerParams rasterizer = {};
		// TODO: allow more customization
	};

	struct Pipeline
	{
		VkPipeline pipeline;
		VkPipelineLayout pipelineLayout;
		VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	};

	Pipeline* create_graphics_pipeline(Device* device, const PipelineParams& params);
	void bind_pipeline(Device* device, Pipeline* pipeline, CommandBuffer* commands);

	void destroy_pipeline(Device* device, Pipeline* pipeline);
} // namespace gfx
