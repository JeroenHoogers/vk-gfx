// Copyright(c) 2026, Jeroen Hoogers
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#include "gfx/device.h"
#include "gfx/command_buffer.h"
#include "gfx/image.h"
#include "gfx/render_pass.h"
#include "gfx/render_target.h"
#include "gfx/swapchain.h"
#include "gfx/sync.h"

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include <map>

namespace gfx
{
	namespace
	{
		struct QueueAssignment {
			uint32_t familyIndex = VK_QUEUE_FAMILY_IGNORED;
			uint32_t queueIndex = 0;
		};

		VkSampleCountFlagBits get_max_usable_sample_count(VkPhysicalDevice physicalDevice) {
			VkPhysicalDeviceProperties properties;
			vkGetPhysicalDeviceProperties(physicalDevice, &properties);

			VkSampleCountFlags counts = properties.limits.framebufferColorSampleCounts & properties.limits.framebufferDepthSampleCounts;
			if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
			if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
			if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
			if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
			if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
			if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

			return VK_SAMPLE_COUNT_1_BIT;
		}

		bool is_queue_suitable(VkPhysicalDevice physicalDevice, VkQueueFamilyProperties familyProperties, uint32_t familyIndex, QueueFlags requestedFlags, VkSurfaceKHR surface = VK_NULL_HANDLE)
		{
			QueueFlags familyFlags = QueueFlags::None;

			if(familyProperties.queueFlags & VkQueueFlagBits::VK_QUEUE_GRAPHICS_BIT) {
				familyFlags |= QueueFlags::Graphics;
			}
			if(familyProperties.queueFlags & VkQueueFlagBits::VK_QUEUE_COMPUTE_BIT) {
				familyFlags |= QueueFlags::Compute;
			}
			if(familyProperties.queueFlags & VkQueueFlagBits::VK_QUEUE_TRANSFER_BIT) {
				familyFlags |= QueueFlags::Transfer;
			}

			if (surface) {
				VkBool32 presentSupport;
				vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, familyIndex, surface, &presentSupport);

				if(presentSupport) {
					familyFlags |= QueueFlags::Present;
				}
			}

			return (familyFlags & requestedFlags) == requestedFlags;
		}

		std::vector<QueueAssignment> find_queue_families(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, const std::vector<QueueRequest>& requestedQueues)
		{
			std::vector<QueueAssignment> assignments;
			assignments.reserve(requestedQueues.size());

			uint32_t queueFamilyCount;
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

			std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

			for (uint32_t i = 0; i < requestedQueues.size(); i++) {
				for (std::uint32_t j = 0; j < queueFamilyCount; j++) {
					// TODO: this greedy approach may not always work, safer to find all candidates per request and then decide
					if(is_queue_suitable(physicalDevice, queueFamilies[j], j, requestedQueues[i].flags, surface)) {
						assignments.push_back( { .familyIndex = j, .queueIndex = 0 }); // TODO: support multiple queues for the same family
						break;
					}
				}
			}

			return assignments;
		}

		// TODO: allow user callback instead
		VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_callback([[maybe_unused]] VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, [[maybe_unused]] void* userData)
		{
			std::printf("validation layer: %s\n", callbackData->pMessage);
			return VK_FALSE;
		}

		VkDebugUtilsMessengerEXT create_debug_utils_messenger_ext(VkInstance instance, const VkAllocationCallbacks* allocator)
		{
			VkDebugUtilsMessengerCreateInfoEXT createInfo{
				.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
				.pNext = nullptr,
				.flags = 0,
				.messageSeverity =
					VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
					VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
					VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
				.messageType =
					VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
					VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
					VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
				.pfnUserCallback = vulkan_debug_callback,
				.pUserData = nullptr
			};

			auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

			VkResult result = VK_SUCCESS;
			VkDebugUtilsMessengerEXT debugMessenger = nullptr;
			if (func != nullptr) {
				result = func(instance, &createInfo, allocator, &debugMessenger);
			} else {
				result = VK_ERROR_EXTENSION_NOT_PRESENT;
			}

			VK_ASSERT(result);

			return debugMessenger;
		}

		void destroy_debug_utils_messenger_ext(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* allocator)
		{
			auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
			if (func != nullptr) {
				func(instance, debugMessenger, allocator);
			}
		}

		VkInstance create_instance(const std::string& appName, uint32_t apiVersion, const std::vector<const char*>& extensions, const std::vector<const char*>& layers)
		{
			VkApplicationInfo appInfo{
				.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
				.pNext = nullptr,
				.pApplicationName = appName.c_str(),
				.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
				.pEngineName = "No Engine",
				.engineVersion = VK_MAKE_VERSION(1, 0, 0),
				.apiVersion = apiVersion
			};

			std::printf("extensions %lu, layers: %lu\n", extensions.size(), layers.size());

			VkInstanceCreateInfo createInfo{
				.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.pApplicationInfo = &appInfo,
				.enabledLayerCount = static_cast<std::uint32_t>(layers.size()),
				.ppEnabledLayerNames = layers.data(),
				.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size()),
				.ppEnabledExtensionNames = extensions.data()
			};

			VkInstance instance;
			VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
			VK_ASSERT(result);

			return instance;
		}

		bool check_layer_support(std::vector<const char*> layers)
		{
			std::uint32_t layerCount;
			vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

			std::vector<VkLayerProperties> availableLayers(layerCount);
			vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

			for (const char* layerName : layers) {
				bool layerFound = false;

				for (const auto& layerProperties : availableLayers) {
					if (std::strncmp(layerName, layerProperties.layerName, sizeof(layerProperties.layerName)) == 0) {
						layerFound = true;
						break;
					}
				}

				if (!layerFound) {
					std::printf("Requested Layer unsupported!: %s", layerName);
					return false;
				}
			}

			return true;
		}

		bool check_device_features(const VkPhysicalDeviceFeatures& requested, const VkPhysicalDeviceFeatures& supported)
		{
			constexpr size_t count = sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32);

			VkBool32 requestedBools[count];
			VkBool32 supportedBools[count];

			std::memcpy(requestedBools, &requested, sizeof requestedBools);
			std::memcpy(supportedBools, &supported, sizeof supportedBools);

			for (std::size_t i = 0; i < count; ++i) {
				if (requestedBools[i] && !supportedBools[i]) {
					return false;
				}
			}
			return true;
		}

		bool is_device_suitable(VkPhysicalDevice physicalDevice, const std::vector<const char*>& extensions, const DeviceCreateParams& params, VkSurfaceKHR surface)
		{
			// check required extensions
			std::uint32_t extensionCount;
			vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
			std::vector<VkExtensionProperties> extensionProperties(extensionCount);

			vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, extensionProperties.data());

			bool has_extensions = true;
			for (const auto& requiredExtension : extensions) {
				bool found = false;
				for (const auto& deviceExtension : extensionProperties) {
					if (std::strncmp(deviceExtension.extensionName, requiredExtension, sizeof(deviceExtension.extensionName)) == 0) {
						found = true;
						break;
					}
				}
				has_extensions |= found;
			}

			// check swapchain
			bool swapchain_adequate = false;
			if (has_extensions && surface) {
				auto supportDetails = detail::query_swapchain_support(physicalDevice, surface);
				swapchain_adequate = !supportDetails.formats.empty() && !supportDetails.presentModes.empty();
			}
			bool present_adequate = !surface || swapchain_adequate;

			// VkPhysicalDeviceProperties properties;
			// vkGetPhysicalDeviceProperties(physicalDevice, &properties);
			// properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU

			// check queue families
			// TODO: inefficient to repeatedly query this, we should probably save these assignments after querying them once
			std::vector<QueueAssignment> assignments = find_queue_families(physicalDevice, surface, params.queues);
			bool has_queue_families = assignments.size() == params.queues.size();

			VkPhysicalDeviceFeatures supportedFeatures;
			vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);
			bool features_supported = check_device_features(params.features, supportedFeatures);
			// TODO: check supported features against provided features

			return has_queue_families && has_extensions && present_adequate && features_supported;
		}

		VkPhysicalDevice pick_physical_device(VkInstance instance, const std::vector<const char*>& extensions, const DeviceCreateParams& params, VkSurfaceKHR surface)
		{
			std::uint32_t deviceCount = 0;
			VkResult result = vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
			VK_ASSERT(result);

			if (deviceCount == 0) {
				fprintf(stderr, "failed to find GPUs with Vulkan support!");
				abort();
			}

			std::vector<VkPhysicalDevice> devices(deviceCount);
			result = vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
			VK_ASSERT(result);

			for (const VkPhysicalDevice& physicalDevice : devices) {
				VkPhysicalDeviceProperties deviceProperties;
				vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

				if (is_device_suitable(physicalDevice, extensions, params, surface)) {
					printf("Found suitable physical device: %s \n", deviceProperties.deviceName);
					return physicalDevice;
				}
			}

			fprintf(stderr, "failed to find a suitable GPU!");
			abort();
		}

		VkDevice create_logical_device(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, const std::vector<const char*>& extensions, const DeviceCreateParams& params, Queue& graphicsQueue, Queue& presentQueue, Queue& computeQueue)
		{
			std::vector<QueueAssignment> queueAssignments = find_queue_families(physicalDevice, surface, params.queues);
			if (queueAssignments.size() != params.queues.size()) {
				fprintf(stderr, "failed to find required queue families!");
			}

			// make queue create infos
				// TODO: this assumes all queues are required, should be adapted
				// std::vector<std::uint32_t> queueFamilies = {indices.graphicsFamily};
				// if(surface) {
				// 	queueFamilies.push_back(indices.presentFamily);
				// }
			std::map<uint32_t, uint32_t> queueCounts;

			float queuePriority = 1.0f;
			for (const auto& assignment : queueAssignments) {
				queueCounts[assignment.familyIndex] = std::max(queueCounts[assignment.familyIndex], assignment.queueIndex + 1);
			}
			std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
			queueCreateInfos.reserve(queueCounts.size());

			for (const auto& [familyIndex, queueCount] : queueCounts) {
				VkDeviceQueueCreateInfo queueCreateInfo{
					.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.queueFamilyIndex = familyIndex,
					.queueCount = queueCount,
					.pQueuePriorities = &queuePriority
				};
				queueCreateInfos.push_back(queueCreateInfo);
			}

			VkDeviceCreateInfo createInfo{
				.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.queueCreateInfoCount = static_cast<std::uint32_t>(queueCreateInfos.size()),
				.pQueueCreateInfos = queueCreateInfos.data(),
				.enabledLayerCount = 0,
				.ppEnabledLayerNames = {},
				.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size()),
				.ppEnabledExtensionNames = extensions.data(),
				.pEnabledFeatures = &params.features
			};

			VkDevice device;
			VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
			VK_ASSERT(result);

			// TODO: store list of queues instead and store a mapping to them
			for (uint32_t i = 0; i < params.queues.size(); i++) {
				if ((params.queues[i].flags & QueueFlags::Graphics) == QueueFlags::Graphics) {
					graphicsQueue.familyIndex = queueAssignments[i].familyIndex;
					graphicsQueue.queueIndex = queueAssignments[i].queueIndex;
					vkGetDeviceQueue(device, graphicsQueue.familyIndex, graphicsQueue.queueIndex, &graphicsQueue.handle);
				}
				if ((params.queues[i].flags & QueueFlags::Present) == QueueFlags::Present) {
					presentQueue.familyIndex = queueAssignments[i].familyIndex;
					presentQueue.queueIndex = queueAssignments[i].queueIndex;
					vkGetDeviceQueue(device, presentQueue.familyIndex, presentQueue.queueIndex, &presentQueue.handle);
				}
				if ((params.queues[i].flags & QueueFlags::Compute) == QueueFlags::Compute) {
					computeQueue.familyIndex = queueAssignments[i].familyIndex;
					computeQueue.queueIndex = queueAssignments[i].queueIndex;
					vkGetDeviceQueue(device, computeQueue.familyIndex, computeQueue.queueIndex, &computeQueue.handle);
				}
			}

			return device;
		}

		VkCommandPool create_command_pool(Device* device, uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)
		{
			VkCommandPoolCreateInfo poolInfo{
				.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
				.pNext = nullptr,
				.flags = flags,
				.queueFamilyIndex = queueFamilyIndex
			};

			VkCommandPool commandPool;
			VkResult result = vkCreateCommandPool(device->device, &poolInfo, nullptr, &commandPool);
			VK_ASSERT(result);
			return commandPool;
		}
	} // namespace

	Device* create_device(const DeviceCreateParams& params)
	{
		constexpr const char* validationLayers = "VK_LAYER_KHRONOS_validation";
		std::vector<const char*> deviceExtensions = params.deviceExtensions;
		std::vector<const char*> extensions = params.extensions;
		std::vector<const char*> layers = params.layers;

		if (params.enableValidation) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			layers.push_back(validationLayers);
		}

		if (!check_layer_support(params.layers)) {
			// printf("Layer")
		}

		// TODO: move to window.h
		// add window instance extensions
		if (params.windows.size() > 0) {
			std::uint32_t windowExtensionCount = 0;
			VkResult result = (*params.windows.begin()).window->callbacks.get_required_instance_extensions(&windowExtensionCount, nullptr, nullptr);
			VK_ASSERT(result);

			// TODO: extend params vector and write into the offset?
			std::vector<const char*> windowExtensions(windowExtensionCount);
			result = (*params.windows.begin()).window->callbacks.get_required_instance_extensions(&windowExtensionCount, windowExtensions.data(), nullptr);
			VK_ASSERT(result);

			for (std::uint32_t i = 0; i < windowExtensionCount; i++) {
				printf("window ext %s\n", windowExtensions[i]);
				extensions.push_back(windowExtensions[i]);
			}
		}

		VkInstance instance = create_instance(params.appname, params.apiVersion, extensions, layers);

		VkSurfaceKHR surface = VK_NULL_HANDLE; // TODO: support windowless?

		for (auto& windowParams : params.windows) {
			Window* window = windowParams.window;
			detail::init_window(instance, window);
			surface = window->surface;
		}

		// add swapchain extension if we have a surface
		if(surface) {
			deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		}

		VkPhysicalDevice physicalDevice = pick_physical_device(instance, deviceExtensions, params, surface);
		VkSampleCountFlagBits maxMsaaSamples = get_max_usable_sample_count(physicalDevice); // TODO: incorporate desired MSAA samples in device choice?
		VkSampleCountFlagBits msaaSamples = std::clamp(params.msaaSamples, VK_SAMPLE_COUNT_1_BIT, maxMsaaSamples);

		printf("Frames in Flight: %d\n", params.framesInFlight);
		printf("MSAA samples: %d/%d\n", msaaSamples, maxMsaaSamples);

		Queue graphicsQueue = {};
		Queue presentQueue = {};
		Queue computeQueue = {};
		VkDevice device = create_logical_device(physicalDevice, surface, deviceExtensions, params, graphicsQueue, presentQueue, computeQueue);

		VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
		if (params.enableValidation) {
			debugMessenger = create_debug_utils_messenger_ext(instance, nullptr);
		}

		Device* pDevice = new Device{
			.instance = instance,
			.physicalDevice = physicalDevice,
			.device = device,
			.debugMessenger = debugMessenger,
			.graphicsQueue = graphicsQueue,
			.presentQueue = presentQueue,
			.computeQueue = computeQueue,
			.msaaSamples = msaaSamples,
		};

		for (const auto& windowParams : params.windows) {
			Window* window = windowParams.window;
			window->swapchain = create_swapchain(pDevice, window, windowParams.swapchain);
		}

		pDevice->commandPool = create_command_pool(pDevice, graphicsQueue.familyIndex);
		pDevice->transientPool = create_command_pool(pDevice, graphicsQueue.familyIndex, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
		pDevice->renderPass = create_render_pass(pDevice, params.renderPass);

		for (const auto& windowParams : params.windows) {
			Window* window = windowParams.window;
			std::vector<VkCommandBuffer> commandBuffers = detail::create_command_buffers(pDevice, pDevice->commandPool, params.framesInFlight);
			window->frames.resize(params.framesInFlight);
			for (uint32_t i = 0; i < params.framesInFlight; i++) {
				window->frames[i] = Frame{
					.commands = commandBuffers[i],
					.depthImage = nullptr,
					.inFlightFence = create_fence(pDevice),
					.imageAvailable = create_semaphore(pDevice)
				};
			}
			window->renderTarget = create_render_target(pDevice, window, pDevice->renderPass);
		}

		if (params.resourcePool.max_sets > 0) {
			pDevice->resourcePool = create_resource_pool(pDevice, params.resourcePool);
		}

		return pDevice;
	}

	void wait_idle(Device* device)
	{
		vkDeviceWaitIdle(device->device);
	}

	void destroy_device(Device* device)
	{
		if (device->resourcePool) {
			destroy_resource_pool(device, device->resourcePool);
		}

		vkDestroyCommandPool(device->device, device->transientPool, nullptr);
		vkDestroyCommandPool(device->device, device->commandPool, nullptr);
		destroy_render_pass(device, device->renderPass);

		vkDestroyDevice(device->device, nullptr);

		if (device->debugMessenger != VK_NULL_HANDLE) {
			destroy_debug_utils_messenger_ext(device->instance, device->debugMessenger, nullptr);
		}

		vkDestroyInstance(device->instance, nullptr);

		delete device;
		device = nullptr;
	}
} // namespace gfx
