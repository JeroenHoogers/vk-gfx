// Copyright(c) 2026, Jeroen Hoogers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include "dynamic_state.h"
#include "types.h"
#include <variant>
#include <vector>
#include <vulkan/vulkan.h>

namespace gfx
{
	struct Device;
	struct VertexLayout;
	struct UniformBuffer;

	struct RasterizerParams
	{
		VkBool32 enableDepthClamp = VK_FALSE;
		VkBool32 enableRasterizerDiscard = VK_FALSE;
		VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
		VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
		VkFrontFace frontFace = VK_FRONT_FACE_CLOCKWISE;
		float lineWidth = 1.0f;
	};

	struct MultisamplingParams
	{
		VkBool32 enableSampleShading = VK_FALSE;
		float minSampleShading = 1.0f;
		VkBool32 enableAlphaToCoverage = VK_FALSE;
		VkBool32 enableAlphaToOne = VK_FALSE;
	};

	struct InputAssemblyParams
	{
		VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		VkBool32 restart_enable = VK_FALSE;
	};

	struct BlendParams
	{
		VkBool32 enableBlend = VK_FALSE;
	};

	typedef std::vector<uint32_t> SpirvCode;

	struct PipelineLayoutDesc
	{
		std::vector<ResourceSetLayout> resourceSetLayouts = {};
		std::vector<PushConstantRange> pushConstants = {};
	};

	struct PipelineLayout
	{
		VkPipelineLayout layout = VK_NULL_HANDLE;
		std::vector<PushConstantRange> pushConstants = {};
	};

	using PipelineLayoutSrc = std::variant<PipelineLayout, PipelineLayoutDesc>;
	using ShaderSrc = std::variant<ShaderModule, SpirvCode>;

	struct GraphicsPipelineParams
	{
		ShaderSrc vertexShader;
		ShaderSrc fragmentShader;
		ShaderSrc geometryShader = ShaderModule{VK_NULL_HANDLE};
		PipelineLayoutSrc layout = PipelineLayoutDesc{};
		VertexLayout* vertexLayout = nullptr;
		InputAssemblyParams inputAssembly = {};
		RasterizerParams rasterizer = {};
		MultisamplingParams multisampling = {};
		BlendParams blending = {};
		DynamicStateFlags dynamicStates = DynamicStateFlags::Viewport | DynamicStateFlags::Scissor;
		// TODO: add color & depth formats?
		// TODO: allow more customization
	};

	struct ComputePipelineParams
	{
		ShaderSrc computeShader;
		PipelineLayoutSrc layout = PipelineLayoutDesc{};
	};

	struct Pipeline
	{
		VkPipeline pipeline;
		PipelineLayout layout;
		// Set automatically at creation, if the user supplies the layout as an argument the user is repsponsible for destroying it.
		// If the layout is created internally in `create_xxxxx_pipeline`, its lifetime will be tied to the pipeline
		const bool ownsLayout = false;
		// std::vector<PushConstantRange> pushConstantRanges;
		VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		DynamicStateFlags dynamicStates;
	};

	[[nodiscard]] ShaderModule create_shader_module(Device* device, const SpirvCode& shader);
	void destroy_shader_module(Device* device, ShaderModule shader_module);

	[[nodiscard]] PipelineLayout create_pipeline_layout(Device* device, const PipelineLayoutDesc& params);

	[[nodiscard]] Pipeline* create_graphics_pipeline(Device* device, const GraphicsPipelineParams& params);
	[[nodiscard]] Pipeline* create_compute_pipeline(Device* device, const ComputePipelineParams& params);

	// TODO: allow dynamic state changes outside of the bind pipeline call
	void bind_pipeline(Pipeline* pipeline, CommandBuffer commands, const DynamicState& dynamicState);
	void bind_pipeline(Pipeline* pipeline, CommandBuffer commands);

	void push_constants(CommandBuffer commands, Pipeline* pipeline, uint32_t rangeIndex, void* data);

	void destroy_pipeline(Device* device, Pipeline* pipeline);
} // namespace gfx
