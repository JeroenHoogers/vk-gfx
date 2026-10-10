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
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.setLayoutCount = static_cast<uint32_t>(params.resourceSetLayouts.size()),
			.pSetLayouts = params.resourceSetLayouts.data(),
			.pushConstantRangeCount = static_cast<uint32_t>(params.pushConstants.size()),
			.pPushConstantRanges = params.pushConstants.data()
		};

		VkPipelineLayout pipelineLayout;
		VkResult result = vkCreatePipelineLayout(device->device, &pipelineLayoutInfo, nullptr, &pipelineLayout);
		VK_ASSERT(result);

		return PipelineLayout{
			.layout = pipelineLayout,
			.pushConstants = params.pushConstants
		};
	}

	Pipeline* create_graphics_pipeline(Device* device, const GraphicsPipelineParams& params)
	{
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages{};

		// TODO: make helper?
		const SpirvCode* vertexCode = std::get_if<SpirvCode>(&params.vertexShader);
		ShaderModule vertexModule = VK_NULL_HANDLE;
		if (vertexCode) {
			vertexModule = create_shader_module(device, *vertexCode);
		} else {
			vertexModule = *std::get_if<ShaderModule>(&params.vertexShader);
		}

		const SpirvCode* fragmentCode = std::get_if<SpirvCode>(&params.fragmentShader);
		ShaderModule fragmentModule = VK_NULL_HANDLE;
		if (fragmentCode) {
			fragmentModule = create_shader_module(device, *fragmentCode);
		} else {
			fragmentModule = *std::get_if<ShaderModule>(&params.fragmentShader);
		}

		const SpirvCode* geometryCode = std::get_if<SpirvCode>(&params.geometryShader);
		ShaderModule geometryModule = VK_NULL_HANDLE;
		if (geometryCode) {
			geometryModule = create_shader_module(device, *geometryCode);
		} else if (std::holds_alternative<ShaderModule>(params.geometryShader)) {
			geometryModule = *std::get_if<ShaderModule>(&params.geometryShader);
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

		VkPipelineViewportStateCreateInfo viewportState{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.viewportCount = 1,
			.pViewports = nullptr,
			.scissorCount = 1,
			.pScissors = nullptr
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
			.depthTestEnable = device->renderPass->useDepth ? VK_TRUE : VK_FALSE,
			.depthWriteEnable = device->renderPass->useDepth ? VK_TRUE : VK_FALSE,
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

		// ENABLED BLENDING:
		// colorBlendAttachment.blendEnable = VK_TRUE;
		// colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		// colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		// colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		// colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		// colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		// colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

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
			.layout = pipelineLayout.layout,
			.renderPass = device->renderPass->renderPass,
			.subpass = 0,
			.basePipelineHandle = VK_NULL_HANDLE,
			.basePipelineIndex = -1
		};

		VkPipeline graphicsPipeline;
		VkResult result = vkCreateGraphicsPipelines(device->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline);
		VK_ASSERT(result);

		// clean up shader modules if they were created inline
		if (vertexCode) {
			vkDestroyShaderModule(device->device, vertexModule, nullptr);
		}
		if (fragmentCode) {
			vkDestroyShaderModule(device->device, fragmentModule, nullptr);
		}
		if (geometryCode) {
			vkDestroyShaderModule(device->device, geometryModule, nullptr);
		}

		Pipeline* pPipeline = new Pipeline{
			.pipeline = graphicsPipeline,
			.layout = pipelineLayout,
			.ownsLayout = pipelineLayoutParams != nullptr, // true if it was created inline, otherwise this is the users responsibility to manage it's lifetime
			.bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.dynamicStates = params.dynamicStates
		};

		return pPipeline;
	}

	Pipeline* create_compute_pipeline(Device* device, const ComputePipelineParams& params)
	{
		// TODO: make helper?
		const SpirvCode* computeCode = std::get_if<SpirvCode>(&params.computeShader);
		ShaderModule computeModule = VK_NULL_HANDLE;
		if (computeCode) {
			computeModule = create_shader_module(device, *computeCode);
		} else {
			computeModule = *std::get_if<ShaderModule>(&params.computeShader);
		}

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
			.layout = pipelineLayout.layout,
			.basePipelineHandle = VK_NULL_HANDLE,
			.basePipelineIndex = 0
		};

		VkPipeline computePipeline;
		VkResult result = vkCreateComputePipelines(device->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &computePipeline);
		VK_ASSERT(result);

		if (computeCode) {
			vkDestroyShaderModule(device->device, computeModule, nullptr);
		}

		Pipeline* pPipeline = new Pipeline{
			.pipeline = computePipeline,
			.layout = pipelineLayout,
			.ownsLayout = pipelineLayoutParams != nullptr, // true if it was created inline, otherwise this is the users responsibility to manage it's lifetime
			.bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE,
			.dynamicStates = DynamicStateFlags::None
		};

		return pPipeline;
	}

	void bind_pipeline(Pipeline* pipeline, CommandBuffer commands) {
		vkCmdBindPipeline(commands, pipeline->bindPoint, pipeline->pipeline);
	}

	void bind_pipeline(Pipeline* pipeline, CommandBuffer commands, const DynamicState& dynamicState)
	{
		vkCmdBindPipeline(commands, pipeline->bindPoint, pipeline->pipeline);

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
		    pipeline->layout.layout,
		    range.stageFlags, range.offset, range.size,
		    data
		);
	}

	void destroy_pipeline(Device* device, Pipeline* pipeline)
	{
		vkDestroyPipeline(device->device, pipeline->pipeline, nullptr);
		if(pipeline->ownsLayout) {
			vkDestroyPipelineLayout(device->device, pipeline->layout.layout, nullptr);
		}

		pipeline = nullptr;
	}
} // namespace gfx
