#pragma once
#include <vulkan/vulkan.h>
#include <vector>

namespace gfx
{
	struct Device;
	struct CommandBuffer;
	struct VertexLayout;

	struct PipelineParams
	{
		std::vector<char> vertex_shader = {};
		std::vector<char> fragment_shader = {};
		std::vector<char> geometry_shader = {};
		VertexLayout* vertex_layout = nullptr;
		// TODO: allow customization
	};

	struct Pipeline
	{
		VkPipeline pipeline;
		VkPipelineLayout pipelineLayout;
	};

	Pipeline* create_graphics_pipeline(Device* device, const PipelineParams& params);
	void bind_pipeline(Device* device, Pipeline* pipeline, CommandBuffer* commands);

	void destroy_pipeline(Device* device, Pipeline* pipeline);
} // namespace gfx
