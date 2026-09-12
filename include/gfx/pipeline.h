#pragma once
#include <vulkan/vulkan.h>
#include <vector>

namespace gfx
{
	struct Device;
	struct CommandBuffer;
	struct VertexLayout;
	struct UniformBuffer;

	struct PipelineParams
	{
		std::vector<char> vertex_shader = {};
		std::vector<char> fragment_shader = {};
		std::vector<char> geometry_shader = {};
		VertexLayout* vertex_layout = nullptr;
		std::vector<UniformBuffer*> uniform_buffers = {};
		// TODO: allow customization
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
