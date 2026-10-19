// Copyright(c) 2026, Jeroen Hoogers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include "dynamic_state.h"
#include "types.h"
#include <variant>
#include <vector>
#include <vulkan/vulkan.h>
#include "resource.h"

namespace gfx
{
	struct Device;
	struct RenderPass;
	struct VertexLayout;

	struct RasterizerDesc
	{
		VkBool32 enableDepthClamp = VK_FALSE;
		VkBool32 enableRasterizerDiscard = VK_FALSE;
		VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
		VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
		VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		float lineWidth = 1.0f;
	};

	struct MultisamplingDesc
	{
		VkBool32 enableSampleShading = VK_FALSE;
		float minSampleShading = 1.0f;
		VkBool32 enableAlphaToCoverage = VK_FALSE;
		VkBool32 enableAlphaToOne = VK_FALSE;
	};

	struct InputAssemblyDesc
	{
		VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		VkBool32 restart_enable = VK_FALSE;
	};


	struct ViewportStateDesc
	{
		VkExtent2D extent;
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

	struct GraphicsPipelineParams
	{
		ShaderModule vertexModule = VK_NULL_HANDLE;
		ShaderModule fragmentModule = VK_NULL_HANDLE;
		ShaderModule geometryModule = VK_NULL_HANDLE;
		SpirvCode vertexSpirv = {};
		SpirvCode fragmentSpirv = {};
		SpirvCode geometrySpirv = {};
		RenderPass* renderPass = nullptr; // if nullptr -> use default device render pass
		PipelineLayoutSrc layout = PipelineLayoutDesc{};
		VertexLayout* vertexLayout = nullptr;
		InputAssemblyDesc inputAssembly = {};
		ViewportStateDesc viewport = {};
		RasterizerDesc rasterizer = {};
		MultisamplingDesc multisampling = {};
		BlendParams blending = {};
		DynamicStateFlags dynamicStates = DynamicStateFlags::Viewport | DynamicStateFlags::Scissor;
		// TODO: add color & depth formats?
		// TODO: allow more customization
	};

	struct ComputePipelineParams
	{
		ShaderModule computeModule = VK_NULL_HANDLE;
		SpirvCode computeSpirv = {};
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

	[[nodiscard]] PipelineCache create_pipeline_cache(Device* device, const void* intitialData = nullptr, size_t initialDataSize = 0);
	std::vector<char> get_pipeline_cache_data(Device* device, PipelineCache pipelineCache);
	void destroy_pipeline_cache(Device* device, PipelineCache pipelineCache);

	[[nodiscard]] PipelineLayout create_pipeline_layout(Device* device, const PipelineLayoutDesc& params);

	[[nodiscard]] Pipeline* create_graphics_pipeline(Device* device, const GraphicsPipelineParams& params);
	[[nodiscard]] Pipeline* create_compute_pipeline(Device* device, const ComputePipelineParams& params);

	// TODO: allow dynamic state changes outside of the bind pipeline call
	void bind_pipeline(Pipeline* pipeline, CommandBuffer commands, const DynamicState& dynamicState);
	void bind_pipeline(Pipeline* pipeline, CommandBuffer commands);

	void push_constants(CommandBuffer commands, Pipeline* pipeline, uint32_t rangeIndex, void* data);

	void destroy_pipeline(Device* device, Pipeline* pipeline);
} // namespace gfx
