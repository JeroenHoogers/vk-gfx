#include "gfx/pipeline.h"
#include "gfx/command_buffer.h"
#include "gfx/resource.h"
#include "gfx/device.h"
#include "gfx/mesh.h"
#include "gfx/render_pass.h"
#include "gfx/swapchain.h"
#include "gfx/uniform_buffer.h"
#include <bit>

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
	} // namespace

	namespace detail
	{
		VkShaderModule create_shader_module(Device* device, const std::vector<char>& shader)
		{
			VkShaderModuleCreateInfo createInfo{
				.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.codeSize = shader.size(),
				.pCode = reinterpret_cast<const uint32_t*>(shader.data())
			};

			VkShaderModule shaderModule;
			VkResult result = vkCreateShaderModule(device->device, &createInfo, nullptr, &shaderModule);
			VK_ASSERT(result);
			return shaderModule;
		}
	} // namespace detail

	Pipeline* create_graphics_pipeline(Device* device, const PipelineParams& params)
	{
		VkShaderModule vertShaderModule = detail::create_shader_module(device, params.vertex_shader);
		VkShaderModule fragShaderModule = detail::create_shader_module(device, params.fragment_shader);
		VkShaderModule geomShaderModule = VK_NULL_HANDLE;

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = vertShaderModule,
			.pName = "main",
			.pSpecializationInfo = nullptr
		};

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = fragShaderModule,
			.pName = "main",
			.pSpecializationInfo = nullptr
		};

		std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {vertShaderStageInfo, fragShaderStageInfo};

		if (!params.geometry_shader.empty()) {
			geomShaderModule = detail::create_shader_module(device, params.geometry_shader);

			VkPipelineShaderStageCreateInfo geomShaderStageInfo{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.stage = VK_SHADER_STAGE_GEOMETRY_BIT,
				.module = geomShaderModule,
				.pName = "main",
				.pSpecializationInfo = nullptr
			};
			shaderStages.push_back(geomShaderStageInfo);
		}

		// Dynamic state
		std::vector<VkDynamicState> dynamicStates = get_dynamic_states(params.dynamic_states);

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
		if (params.vertex_layout != nullptr) {
			bindings.resize(params.vertex_layout->bindings.size());
			for (uint32_t i = 0; i < bindings.size(); i++) {
				const auto& binding = params.vertex_layout->bindings[i];
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

		// // Viewport
		// VkViewport viewport{
		// 	.x = 0.0f,
		// 	.y = 0.0f,
		// 	.width = (float)device->swapchain->extent.width,
		// 	.height = (float)device->swapchain->extent.height,
		// 	.minDepth = 0.0f,
		// 	.maxDepth = 1.0f
		// };

		// VkRect2D scissor{};
		// scissor.offset = {0, 0};
		// scissor.extent = device->swapchain->extent;

		// Rasterizer
		VkPipelineRasterizationStateCreateInfo rasterizer{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.depthClampEnable = VK_FALSE,
			.rasterizerDiscardEnable = VK_FALSE,
			.polygonMode = params.rasterizer.polygon_mode,
			.cullMode = params.rasterizer.cull_mode,
			.frontFace = params.rasterizer.front_face,
			.depthBiasEnable = VK_FALSE,
			.depthBiasConstantFactor = 0.0f,
			.depthBiasClamp = 0.0f,
			.depthBiasSlopeFactor = 0.0f,
			.lineWidth = 1.0f
		};

		// Multisampling
		VkPipelineMultisampleStateCreateInfo multisampling{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.rasterizationSamples = device->msaaSamples,
			.sampleShadingEnable = params.multisampling.enable_sample_shading,
			.minSampleShading = params.multisampling.min_sample_shading,
			.pSampleMask = nullptr,
			.alphaToCoverageEnable = VK_FALSE,
			.alphaToOneEnable = VK_FALSE
		};

		// Color Blend Attachment
		VkPipelineColorBlendAttachmentState colorBlendAttachment{
			.blendEnable = params.blending.enable_blend,
			.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
			.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			.colorBlendOp = VK_BLEND_OP_ADD,
			.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
			.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			.alphaBlendOp = VK_BLEND_OP_ADD,
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
		};

		VkPipelineDepthStencilStateCreateInfo depthStencil{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.depthTestEnable = device->enableDepth ? VK_TRUE : VK_FALSE,
			.depthWriteEnable = device->enableDepth ? VK_TRUE : VK_FALSE,
			.depthCompareOp = VK_COMPARE_OP_LESS,
			.depthBoundsTestEnable = VK_FALSE,
			.stencilTestEnable = VK_FALSE,
			.front = {},
			.back = {},
			.minDepthBounds = 0.0f,
			.maxDepthBounds = 1.0f
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

		std::vector<VkDescriptorSetLayout> setLayouts(params.resource_set_layouts.size());
		for (uint32_t i = 0; i < params.resource_set_layouts.size(); i++) {
			setLayouts[i] = params.resource_set_layouts[i]->descriptorSetLayout;
		}

		std::vector<VkPushConstantRange> pushConstantRanges(params.push_constants.size());
		for (uint32_t i = 0; i < params.push_constants.size(); i++) {
			pushConstantRanges[i] = {
				.stageFlags = params.push_constants[i].stages,
				.offset = params.push_constants[i].offset,
				.size = params.push_constants[i].size
			};
		}

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.setLayoutCount = static_cast<uint32_t>(setLayouts.size()),
			.pSetLayouts = setLayouts.data(),
			.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size()),
			.pPushConstantRanges = pushConstantRanges.data()
		};

		VkPipelineLayout pipelineLayout;
		VkResult result = vkCreatePipelineLayout(device->device, &pipelineLayoutInfo, nullptr, &pipelineLayout);
		VK_ASSERT(result);

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
			.layout = pipelineLayout,
			.renderPass = device->renderPass->renderPass,
			.subpass = 0,
			.basePipelineHandle = VK_NULL_HANDLE,
			.basePipelineIndex = -1
		};

		VkPipeline graphicsPipeline;
		result = vkCreateGraphicsPipelines(device->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline);
		VK_ASSERT(result);

		Pipeline* pPipeline = new Pipeline{
			.pipeline = graphicsPipeline,
			.pipelineLayout = pipelineLayout,
			.pushConstantRanges = params.push_constants, // TODO: we could std::move() this if we know the pipeline params are not re-used (maybe add a && overload?)
			.bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.dynamicStates = params.dynamic_states
		};

		// cleanup shader modules
		vkDestroyShaderModule(device->device, fragShaderModule, nullptr);
		vkDestroyShaderModule(device->device, vertShaderModule, nullptr);

		if (geomShaderModule) {
			vkDestroyShaderModule(device->device, geomShaderModule, nullptr);
		}

		return pPipeline;
	}

	void bind_pipeline(Pipeline* pipeline, CommandBuffer* commands, const DynamicState& dynamicState)
	{
		vkCmdBindPipeline(commands->commandBuffer, pipeline->bindPoint, pipeline->pipeline);

		// set dynamic states
		if ((pipeline->dynamicStates & DynamicStateFlags::Viewport) != DynamicStateFlags::None) {
			vkCmdSetViewport(commands->commandBuffer, 0, 1, &dynamicState.viewport);
		}

		if ((pipeline->dynamicStates & DynamicStateFlags::Scissor) != DynamicStateFlags::None) {
			vkCmdSetScissor(commands->commandBuffer, 0, 1, &dynamicState.scissor);
		}

		if ((pipeline->dynamicStates & DynamicStateFlags::CullMode) != DynamicStateFlags::None) {
			vkCmdSetCullMode(commands->commandBuffer, dynamicState.cullMode);
		}

		if ((pipeline->dynamicStates & DynamicStateFlags::FrontFace) != DynamicStateFlags::None) {
			vkCmdSetFrontFace(commands->commandBuffer, dynamicState.frontFace);
		}

		if ((pipeline->dynamicStates & DynamicStateFlags::PrimitiveTopology) != DynamicStateFlags::None) {
			vkCmdSetPrimitiveTopology(commands->commandBuffer, dynamicState.primitiveTopology);
		}

		if ((pipeline->dynamicStates & DynamicStateFlags::LineWidth) != DynamicStateFlags::None) {
			vkCmdSetLineWidth(commands->commandBuffer, dynamicState.lineWidth);
		}
	}

	void push_constants(CommandBuffer* commands, Pipeline* pipeline, uint32_t rangeIndex, void* data){
		// TODO: add bounds checking or return a safe handle?
		const PushConstantRange& range = pipeline->pushConstantRanges[rangeIndex];

		vkCmdPushConstants(
		    commands->commandBuffer,
		    pipeline->pipelineLayout,
		    range.stages, range.offset, range.size,
		    data
		);
	}

	void destroy_pipeline(Device* device, Pipeline* pipeline)
	{
		vkDestroyPipeline(device->device, pipeline->pipeline, nullptr);
		vkDestroyPipelineLayout(device->device, pipeline->pipelineLayout, nullptr);

		pipeline = nullptr;
	}

} // namespace gfx
