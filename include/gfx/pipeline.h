#pragma once
#include <vector>
#include <vulkan/vulkan.h>
#include "dynamic_state.h"

namespace gfx
{
	struct Device;
	struct CommandBuffer;
	struct VertexLayout;
	struct UniformBuffer;
	struct ResourceSet;
	struct ResourceSetLayout;

	struct RasterizerParams
	{
		VkPolygonMode polygon_mode = VK_POLYGON_MODE_FILL;
		VkCullModeFlags cull_mode = VK_CULL_MODE_BACK_BIT;
		VkFrontFace front_face = VK_FRONT_FACE_CLOCKWISE;
	};

	struct MultisamplingParams
	{
		VkBool32 enable_sample_shading = VK_FALSE;
		VkBool32 enable_alpha_to_coverage = VK_FALSE;
		float min_sample_shading = 1.0f;
	};

	struct InputAssemblyParams {
		VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		VkBool32 restartEnable = VK_FALSE;
	};

	struct BlendParams
	{
		VkBool32 enable_blend = VK_FALSE;
	};

	struct PipelineParams
	{
		std::vector<char> vertex_shader = {};
		std::vector<char> fragment_shader = {};
		std::vector<char> geometry_shader = {};
		VertexLayout* vertex_layout = nullptr;
		std::vector<ResourceSetLayout*> resource_set_layouts = {};
		RasterizerParams rasterizer = {};
		MultisamplingParams multisampling = {};
		InputAssemblyParams inputAssembly = {};
		BlendParams blending = {};
		DynamicStateFlags dynamic_states = DynamicStateFlags::Viewport | DynamicStateFlags::Scissor;
		// TODO: add color & depth formats?
		// TODO: allow more customization
	};

	struct Pipeline
	{
		VkPipeline pipeline;
		VkPipelineLayout pipelineLayout;
		VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

		DynamicStateFlags dynamicStates;
	};

	// TODO: allow dynamic state changes outside of the bind pipeline call

	Pipeline* create_graphics_pipeline(Device* device, const PipelineParams& params);
	void bind_pipeline(Pipeline* pipeline, CommandBuffer* commands, const DynamicState& dynamicState);

	void destroy_pipeline(Device* device, Pipeline* pipeline);
} // namespace gfx
