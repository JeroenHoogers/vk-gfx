#pragma once
#include "device.h"
#include "gfx/swapchain.h"
#include <vector>

namespace gfx
{
	struct VertexLayout {
	    std::vector<VkVertexInputBindingDescription> bindings;
	    std::vector<VkVertexInputAttributeDescription> attributes;
	};

	struct PipelineParams
	{
		std::vector<char> vertex_shader = {};
		std::vector<char> fragment_shader = {};
		std::vector<char> geometry_shader = {};
		// TODO: allow customization
		VertexLayout vertex_layout = {};
	};

	struct Pipeline
	{
		VkPipeline pipeline;
		VkPipelineLayout pipelineLayout;
		VkRenderPass renderPass;
	};

	Pipeline* create_pipeline(Device* device, const PipelineParams& params);
	void bind_pipeline(Pipeline* pipeline); // TODO: add commands

	void destroy_pipeline(Device* device, Pipeline* pipeline);
} // namespace gfx
