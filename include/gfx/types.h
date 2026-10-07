// Copyright(c) 2026, Jeroen Hoogers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include <vulkan/vulkan.h>

namespace gfx
{
	typedef VkCommandBuffer CommandBuffer;

	typedef VkShaderModule ShaderModule;
	typedef VkDescriptorSetLayout ResourceSetLayout;
	typedef VkDescriptorPool ResourcePool;
	typedef VkDescriptorSet ResourceSet;

	typedef VkPushConstantRange PushConstantRange;

	typedef VkSemaphore Semaphore;
	typedef VkFence Fence;

} // namespace gfx
