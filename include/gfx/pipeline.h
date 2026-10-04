#pragma once
#include <vector>
#include <vulkan/vulkan.h>
#include "dynamic_state.h"
#include "types.h"

namespace gfx
{
	struct Device;
	struct VertexLayout;
	struct UniformBuffer;
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
		VkBool32 restart_enable = VK_FALSE;
	};

	struct BlendParams
	{
		VkBool32 enable_blend = VK_FALSE;
	};

	struct PushConstantRange {
		uint32_t offset = 0;
		uint32_t size;
		VkShaderStageFlags stages;
	};

	// TODO: support separation of pipeline and layout (ability to re-use pipeline layout)
	struct GraphicsPipelineParams
	{
		std::vector<char> vertex_shader = {};
		std::vector<char> fragment_shader = {};
		std::vector<char> geometry_shader = {};
		VertexLayout* vertex_layout = nullptr;
		std::vector<ResourceSetLayout*> resource_set_layouts = {};
		std::vector<PushConstantRange> push_constants = {};
		RasterizerParams rasterizer = {};
		MultisamplingParams multisampling = {};
		InputAssemblyParams inputAssembly = {};
		BlendParams blending = {};
		DynamicStateFlags dynamic_states = DynamicStateFlags::Viewport | DynamicStateFlags::Scissor;
		// TODO: add color & depth formats?
		// TODO: allow more customization
	};

	struct ComputePipelineParams
	{
		std::vector<char> compute_shader = {};
		VertexLayout* vertex_layout = nullptr;
		std::vector<ResourceSetLayout*> resource_set_layouts = {};
		std::vector<PushConstantRange> push_constants = {};
	};

	struct Pipeline
	{
		VkPipeline pipeline;
		VkPipelineLayout pipelineLayout;
		std::vector<PushConstantRange> pushConstantRanges;
		VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

		DynamicStateFlags dynamicStates;
	};

	// TODO: allow dynamic state changes outside of the bind pipeline call
	Pipeline* create_graphics_pipeline(Device* device, const GraphicsPipelineParams& params);
	Pipeline* create_compute_pipeline(Device* device, const ComputePipelineParams& params);
	void bind_pipeline(Pipeline* pipeline, CommandBuffer commands, const DynamicState& dynamicState);
	void bind_pipeline(Pipeline* pipeline, CommandBuffer commands);

	void push_constants(CommandBuffer commands, Pipeline* pipeline, uint32_t rangeIndex, void* data);

	void destroy_pipeline(Device* device, Pipeline* pipeline);
} // namespace gfx
