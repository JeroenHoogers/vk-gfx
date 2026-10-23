// Copyright(c) 2026, Jeroen Hoogers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#include "gfx/pipeline.h"
#include "gfx/command_buffer.h"
#include "gfx/resource.h"
#include "gfx/device.h"
#include "gfx/mesh.h"
#include "gfx/render_pass.h"
#include "gfx/swapchain.h"
#include "gfx/uniform_buffer.h"
#include <cassert>

namespace gfx
{
	namespace
	{
		std::vector<VkDynamicState> get_dynamic_states(DynamicStateFlags flags)
		{
			uint32_t count = std::popcount(static_cast<uint32_t>(flags));

			std::vector<VkDynamicState> dynamicStates(count);
			uint32_t i = 0;

			if ((flags & DynamicStateFlags::Viewport) != DynamicStateFlags::None)
				dynamicStates[i++] = VK_DYNAMIC_STATE_VIEWPORT;
			if ((flags & DynamicStateFlags::Scissor) != DynamicStateFlags::None)
				dynamicStates[i++] = VK_DYNAMIC_STATE_SCISSOR;
			if ((flags & DynamicStateFlags::CullMode) != DynamicStateFlags::None)
				dynamicStates[i++] = VK_DYNAMIC_STATE_CULL_MODE;
			if ((flags & DynamicStateFlags::FrontFace) != DynamicStateFlags::None)
				dynamicStates[i++] = VK_DYNAMIC_STATE_FRONT_FACE;
			if ((flags & DynamicStateFlags::PrimitiveTopology) != DynamicStateFlags::None)
				dynamicStates[i++] = VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY;
			if ((flags & DynamicStateFlags::LineWidth) != DynamicStateFlags::None)
				dynamicStates[i++] = VK_DYNAMIC_STATE_LINE_WIDTH;

			return dynamicStates;
		}

		VkPipelineShaderStageCreateInfo create_shader_stage(VkShaderModule module, VkShaderStageFlagBits stage){
			return VkPipelineShaderStageCreateInfo {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.stage = stage,
				.module = module,
				.pName = "main",
				.pSpecializationInfo = nullptr
			};
		}

		void add_shader_stage(VkShaderModule module, VkShaderStageFlagBits stage, std::vector<VkPipelineShaderStageCreateInfo>& stages) {
			if(module == VK_NULL_HANDLE) {
				return;
			}

			stages.push_back(create_shader_stage(module, stage));
		}
	} // namespace

	ShaderModule create_shader_module(Device* device, const SpirvCode& shader)
	{
		if (shader.empty()) {
			return VK_NULL_HANDLE;
		}

		VkShaderModuleCreateInfo createInfo{
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.codeSize = shader.size() * sizeof(uint32_t),
			.pCode = shader.data()
		};

		VkShaderModule shaderModule;
		VkResult result = vkCreateShaderModule(device->device, &createInfo, nullptr, &shaderModule);
		VK_ASSERT(result);
		return shaderModule;
	}

	void destroy_shader_module(Device* device, ShaderModule shader_module) {
		vkDestroyShaderModule(device->device, shader_module, nullptr);
	}

	PipelineLayout create_pipeline_layout(Device* device, const PipelineLayoutDesc& params)
	{
		std::vector<VkDescriptorSetLayout> setLayouts(params.resourceSetLayouts.size());

		for (uint32_t i = 0; i < setLayouts.size(); i++) {
			setLayouts[i] = params.resourceSetLayouts[i].handle;
		}

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.setLayoutCount = static_cast<uint32_t>(setLayouts.size()),
			.pSetLayouts = setLayouts.data(),
			.pushConstantRangeCount = static_cast<uint32_t>(params.pushConstants.size()),
			.pPushConstantRanges = params.pushConstants.data()
		};

		VkPipelineLayout pipelineLayout;
		VkResult result = vkCreatePipelineLayout(device->device, &pipelineLayoutInfo, nullptr, &pipelineLayout);
		VK_ASSERT(result);

		return PipelineLayout{
			.handle = pipelineLayout,
			.pushConstants = params.pushConstants
		};
	}

	PipelineCache create_pipeline_cache(Device* device, const void* initialData, size_t initialDataSize)
	{
		VkPipelineCacheCreateInfo info{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.initialDataSize = initialDataSize,
			.pInitialData = initialData,
		};

		VkPipelineCache cache;
		VkResult result = vkCreatePipelineCache(device->device, &info, nullptr, &cache);
		VK_ASSERT(result);

		return cache;
	}

	std::vector<char> get_pipeline_cache_data(Device* device, PipelineCache pipelineCache) {
		size_t size = 0;
		VkResult result = vkGetPipelineCacheData(device->device, pipelineCache, &size, nullptr);
		VK_ASSERT(result);

		std::vector<char> data(size);
		result = vkGetPipelineCacheData(device->device, pipelineCache, &size, data.data());
		VK_ASSERT(result);

		data.resize(size);
		return data;
	}

	Pipeline* create_graphics_pipeline(Device* device, const GraphicsPipelineParams& params)
	{
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages{};

		// Use device default renderpass if it is not provided
		RenderPass* renderPass = params.renderPass ? params.renderPass : device->renderPass;

		ShaderModule vertexModule = params.vertexModule;
		if (vertexModule == VK_NULL_HANDLE) {
			vertexModule = create_shader_module(device, params.vertexSpirv);
		}

		assert(vertexModule != VK_NULL_HANDLE);

		ShaderModule fragmentModule = params.fragmentModule;
		if (fragmentModule == VK_NULL_HANDLE) {
			fragmentModule = create_shader_module(device, params.fragmentSpirv);
		}
		assert(fragmentModule != VK_NULL_HANDLE);

		// geometry is optional
		ShaderModule geometryModule = params.geometryModule;
		if (geometryModule == VK_NULL_HANDLE && !params.geometrySpirv.empty()) {
			geometryModule = create_shader_module(device, params.geometrySpirv);
		}

		add_shader_stage(vertexModule, VK_SHADER_STAGE_VERTEX_BIT, shaderStages);
		add_shader_stage(fragmentModule, VK_SHADER_STAGE_FRAGMENT_BIT, shaderStages);
		add_shader_stage(geometryModule, VK_SHADER_STAGE_GEOMETRY_BIT, shaderStages);

		// Dynamic state
		std::vector<VkDynamicState> dynamicStates = get_dynamic_states(params.dynamicStates);

		VkPipelineDynamicStateCreateInfo dynamicState{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
			.pDynamicStates = dynamicStates.data()
		};

		VkViewport viewport{
			.x = 0,
			.y = 0,
			.width = static_cast<float>(params.viewport.extent.width),
			.height = static_cast<float>(params.viewport.extent.height),
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};

		VkRect2D scissor{
			.offset = {0, 0},
			.extent = params.viewport.extent,
		};

		VkPipelineViewportStateCreateInfo viewportState{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.viewportCount = 1,
			.pViewports = &viewport,
			.scissorCount = 1,
			.pScissors = &scissor
		};

		// Vertex Input
		std::vector<VkVertexInputBindingDescription> bindings{};
		std::vector<VkVertexInputAttributeDescription> attributes{};
		if (params.vertexLayout != nullptr) {
			bindings.resize(params.vertexLayout->bindings.size());
			for (uint32_t i = 0; i < bindings.size(); i++) {
				const auto& binding = params.vertexLayout->bindings[i];
				bindings[i] = {
					.binding = i,
					.stride = binding.stride,
					.inputRate = binding.rate
				};

				for (uint32_t j = 0; j < binding.attributes.size(); j++) {
					attributes.push_back({.location = j, .binding = i, .format = binding.attributes[j].format, .offset = binding.attributes[j].offset});
				}
			}
		}

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size()),
			.pVertexBindingDescriptions = bindings.data(),
			.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size()),
			.pVertexAttributeDescriptions = attributes.data()
		};

		// Input Assembly
		VkPipelineInputAssemblyStateCreateInfo inputAssembly{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.topology = params.inputAssembly.topology,
			.primitiveRestartEnable = params.inputAssembly.restart_enable
		};

		// Rasterizer
		VkPipelineRasterizationStateCreateInfo rasterizer{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.depthClampEnable = VK_FALSE,
			.rasterizerDiscardEnable = VK_FALSE,
			.polygonMode = params.rasterizer.polygonMode,
			.cullMode = params.rasterizer.cullMode,
			.frontFace = params.rasterizer.frontFace,
			.depthBiasEnable = VK_FALSE,
			.depthBiasConstantFactor = 0.0f,
			.depthBiasClamp = 0.0f,
			.depthBiasSlopeFactor = 0.0f,
			.lineWidth = params.rasterizer.lineWidth
		};

		// Multisampling
		VkPipelineMultisampleStateCreateInfo multisampling{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.rasterizationSamples = device->msaaSamples,
			.sampleShadingEnable = params.multisampling.enableSampleShading,
			.minSampleShading = params.multisampling.minSampleShading,
			.pSampleMask = nullptr,
			.alphaToCoverageEnable = params.multisampling.enableAlphaToCoverage,
			.alphaToOneEnable = params.multisampling.enableAlphaToOne
		};

		// DepthStencil
		VkPipelineDepthStencilStateCreateInfo depthStencil{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.depthTestEnable = renderPass->useDepth ? VK_TRUE : VK_FALSE,
			.depthWriteEnable = renderPass->useDepth ? VK_TRUE : VK_FALSE,
			.depthCompareOp = VK_COMPARE_OP_LESS,
			.depthBoundsTestEnable = VK_FALSE,
			.stencilTestEnable = VK_FALSE,
			.front = {},
			.back = {},
			.minDepthBounds = 0.0f,
			.maxDepthBounds = 1.0f
		};

		// Color Blend Attachment
		VkPipelineColorBlendAttachmentState colorBlendAttachment{
			.blendEnable = params.blending.enableBlend,
			.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
			.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			.colorBlendOp = VK_BLEND_OP_ADD,
			.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
			.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			.alphaBlendOp = VK_BLEND_OP_ADD,
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
		};

		VkPipelineColorBlendStateCreateInfo colorBlending{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.logicOpEnable = VK_FALSE,
			.logicOp = VK_LOGIC_OP_COPY,
			.attachmentCount = 1,
			.pAttachments = &colorBlendAttachment,
			.blendConstants{0.0f, 0.0f, 0.0f, 0.0f}
		};

		const PipelineLayoutDesc* pipelineLayoutParams = std::get_if<PipelineLayoutDesc>(&params.layout);
		PipelineLayout pipelineLayout = {};
		if (pipelineLayoutParams) {
			pipelineLayout = create_pipeline_layout(device, *pipelineLayoutParams);
		} else {
			pipelineLayout = *std::get_if<PipelineLayout>(&params.layout);
		}

		VkGraphicsPipelineCreateInfo pipelineInfo{
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.stageCount = static_cast<uint32_t>(shaderStages.size()),
			.pStages = shaderStages.data(),
			.pVertexInputState = &vertexInputInfo,
			.pInputAssemblyState = &inputAssembly,
			.pTessellationState = nullptr,
			.pViewportState = &viewportState,
			.pRasterizationState = &rasterizer,
			.pMultisampleState = &multisampling,
			.pDepthStencilState = &depthStencil,
			.pColorBlendState = &colorBlending,
			.pDynamicState = &dynamicState,
			.layout = pipelineLayout.handle,
			.renderPass = renderPass->renderPass,
			.subpass = 0,
			.basePipelineHandle = VK_NULL_HANDLE,
			.basePipelineIndex = -1
		};

		VkPipeline graphicsPipeline;
		VkResult result = vkCreateGraphicsPipelines(device->device, device->pipelineCache, 1, &pipelineInfo, nullptr, &graphicsPipeline);
		VK_ASSERT(result);

		// clean up shader modules if they were created inline
		if (!params.vertexModule && !params.vertexSpirv.empty()) {
			vkDestroyShaderModule(device->device, vertexModule, nullptr);
		}
		if (!params.fragmentModule && !params.fragmentSpirv.empty()) {
			vkDestroyShaderModule(device->device, fragmentModule, nullptr);
		}
		if (!params.geometryModule && !params.geometrySpirv.empty()) {
			vkDestroyShaderModule(device->device, geometryModule, nullptr);
		}

		return new Pipeline{
			.handle = graphicsPipeline,
			.layout = pipelineLayout,
			.ownsLayout = pipelineLayoutParams != nullptr, // true if it was created inline, otherwise this is the users responsibility to manage it's lifetime
			.bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.dynamicStates = params.dynamicStates
		};
	}

	Pipeline* create_compute_pipeline(Device* device, const ComputePipelineParams& params)
	{
		ShaderModule computeModule = params.computeModule;
		if (computeModule == VK_NULL_HANDLE) {
			computeModule = create_shader_module(device, params.computeSpirv);
		}

		assert(computeModule != VK_NULL_HANDLE);

		VkPipelineShaderStageCreateInfo computeShaderStageInfo = create_shader_stage(computeModule, VK_SHADER_STAGE_COMPUTE_BIT);

		const PipelineLayoutDesc* pipelineLayoutParams = std::get_if<PipelineLayoutDesc>(&params.layout);
		PipelineLayout pipelineLayout = {};
		if (pipelineLayoutParams) {
			pipelineLayout = create_pipeline_layout(device, *pipelineLayoutParams);
		} else {
			pipelineLayout = *std::get_if<PipelineLayout>(&params.layout);
		}

		VkComputePipelineCreateInfo pipelineInfo{
			.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.stage = computeShaderStageInfo,
			.layout = pipelineLayout.handle,
			.basePipelineHandle = VK_NULL_HANDLE,
			.basePipelineIndex = 0
		};

		VkPipeline computePipeline;
		VkResult result = vkCreateComputePipelines(device->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &computePipeline);
		VK_ASSERT(result);

		if (!params.computeModule && !params.computeSpirv.empty()) {
			vkDestroyShaderModule(device->device, computeModule, nullptr);
		}

		return new Pipeline{
			.handle = computePipeline,
			.layout = pipelineLayout,
			.ownsLayout = pipelineLayoutParams != nullptr, // true if it was created inline, otherwise this is the users responsibility to manage it's lifetime
			.bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE,
			.dynamicStates = DynamicStateFlags::None
		};
	}

	void bind_pipeline(Pipeline* pipeline, CommandBuffer commands) {
		vkCmdBindPipeline(commands, pipeline->bindPoint, pipeline->handle);
	}

	void bind_pipeline(Pipeline* pipeline, CommandBuffer commands, const DynamicState& dynamicState)
	{
		vkCmdBindPipeline(commands, pipeline->bindPoint, pipeline->handle);

		// set dynamic states
		if ((pipeline->dynamicStates & DynamicStateFlags::Viewport) != DynamicStateFlags::None) {
			vkCmdSetViewport(commands, 0, 1, &dynamicState.viewport);
		}

		if ((pipeline->dynamicStates & DynamicStateFlags::Scissor) != DynamicStateFlags::None) {
			vkCmdSetScissor(commands, 0, 1, &dynamicState.scissor);
		}

		if ((pipeline->dynamicStates & DynamicStateFlags::CullMode) != DynamicStateFlags::None) {
			vkCmdSetCullMode(commands, dynamicState.cullMode);
		}

		if ((pipeline->dynamicStates & DynamicStateFlags::FrontFace) != DynamicStateFlags::None) {
			vkCmdSetFrontFace(commands, dynamicState.frontFace);
		}

		if ((pipeline->dynamicStates & DynamicStateFlags::PrimitiveTopology) != DynamicStateFlags::None) {
			vkCmdSetPrimitiveTopology(commands, dynamicState.primitiveTopology);
		}

		if ((pipeline->dynamicStates & DynamicStateFlags::LineWidth) != DynamicStateFlags::None) {
			vkCmdSetLineWidth(commands, dynamicState.lineWidth);
		}
	}

	void push_constants(CommandBuffer commands, Pipeline* pipeline, uint32_t rangeIndex, void* data){
		// TODO: add bounds checking or return a safe handle?
		const PushConstantRange& range = pipeline->layout.pushConstants[rangeIndex];

		vkCmdPushConstants(
		    commands,
		    pipeline->layout.handle,
		    range.stageFlags, range.offset, range.size,
		    data
		);
	}

	void destroy_pipeline_cache(Device* device, PipelineCache pipelineCache) {
		vkDestroyPipelineCache(device->device, pipelineCache, nullptr);
	}

	void destroy_pipeline_layout(Device *device, const PipelineLayout& layout) {
		vkDestroyPipelineLayout(device->device, layout.handle, nullptr);
	}

	void destroy_pipeline(Device* device, Pipeline* pipeline)
	{
		vkDestroyPipeline(device->device, pipeline->handle, nullptr);
		if(pipeline->ownsLayout) {
			destroy_pipeline_layout(device, pipeline->layout);
		}

		delete pipeline;
		pipeline = nullptr;
	}
} // namespace gfx
