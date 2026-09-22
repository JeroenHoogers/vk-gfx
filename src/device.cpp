#include "gfx/device.h"
#include "gfx/command_buffer.h"
#include "gfx/descriptor_set.h"
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

namespace gfx
{
	namespace
	{
		struct QueueFamilyIndices
		{
			std::uint32_t graphicsFamily;
			std::uint32_t presentFamily;
			std::uint32_t transferFamily;

			std::uint32_t queueFamilyCount;
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

		bool find_queue_families(VkPhysicalDevice physicalDevice, QueueFamilyIndices& indices, VkSurfaceKHR surface)
		{
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &indices.queueFamilyCount, nullptr);

			std::vector<VkQueueFamilyProperties> queueFamilies(indices.queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &indices.queueFamilyCount, queueFamilies.data());

			for (std::uint32_t i = 0; i < indices.queueFamilyCount; i++) {
				bool hasGraphics = false;
				if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
					indices.graphicsFamily = i;
					hasGraphics = true;
				}

				VkBool32 presentSupport = !surface;
				if (surface) {
					vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);

					if (presentSupport) {
						indices.presentFamily = i;
					}
				}

				if (hasGraphics && presentSupport)
					return true;
			}

			return false;
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

		VkInstance create_instance(const std::string& appName, const std::vector<const char*>& extensions, const std::vector<const char*>& layers)
		{
			VkApplicationInfo appInfo{
				.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
				.pNext = nullptr,
				.pApplicationName = appName.c_str(),
				.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
				.pEngineName = "No Engine",
				.engineVersion = VK_MAKE_VERSION(1, 0, 0),
				.apiVersion = VK_API_VERSION_1_3
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

		bool is_device_suitable(VkPhysicalDevice physicalDevice, const std::vector<const char*>& extensions, VkSurfaceKHR surface)
		{
			// check queue families
			QueueFamilyIndices indices;
			bool has_queue_families = find_queue_families(physicalDevice, indices, surface);

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
			if (has_extensions) {
				auto supportDetails = detail::query_swapchain_support(physicalDevice, surface);
				swapchain_adequate = !supportDetails.formats.empty() && !supportDetails.presentModes.empty();
			}

			// VkPhysicalDeviceProperties properties;
			// vkGetPhysicalDeviceProperties(physicalDevice, &properties);
			// properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU

			VkPhysicalDeviceFeatures supportedFeatures;
			vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);
			return has_queue_families && has_extensions && swapchain_adequate && supportedFeatures.samplerAnisotropy;
		}

		VkPhysicalDevice pick_physical_device(VkInstance instance, const std::vector<const char*>& extensions, VkSurfaceKHR surface)
		{
			std::uint32_t deviceCount = 0;
			vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

			if (deviceCount == 0) {
				fprintf(stderr, "failed to find GPUs with Vulkan support!");
				abort();
			}

			std::vector<VkPhysicalDevice> devices(deviceCount);
			vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

			for (const VkPhysicalDevice& physicalDevice : devices) {
				if (is_device_suitable(physicalDevice, extensions, surface)) {
					VkPhysicalDeviceProperties deviceProperties;
					vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

					printf("Found suitable physical device: %s \n", deviceProperties.deviceName);
					return physicalDevice;
				}
			}

			fprintf(stderr, "failed to find a suitable GPU!");
			abort();
		}

		VkDevice create_logical_device(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, const std::vector<const char*>& extensions, const VkPhysicalDeviceFeatures& features, Queue& graphicsQueue, Queue& presentQueue)
		{
			QueueFamilyIndices indices;
			if (!find_queue_families(physicalDevice, indices, surface)) {
				fprintf(stderr, "failed to find required queue families!");
			}

			// make queue create infos
			std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
			{
				// TODO: this assumes all queues are required, should be adapted
				std::vector<std::uint32_t> queueFamilies = {indices.graphicsFamily, indices.presentFamily};
				std::vector<bool> exists(indices.queueFamilyCount, false);

				float queuePriority = 1.0f;
				for (uint32_t queueFamily : queueFamilies) {
					if (exists[queueFamily]) {
						continue;
					}
					VkDeviceQueueCreateInfo queueCreateInfo{
						.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
						.pNext = nullptr,
						.flags = 0,
						.queueFamilyIndex = queueFamily,
						.queueCount = 1,
						.pQueuePriorities = &queuePriority
					};
					queueCreateInfos.push_back(queueCreateInfo);

					exists[queueFamily] = true;
				}
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
				.pEnabledFeatures = &features
			};

			VkDevice device;
			VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
			VK_ASSERT(result);

			graphicsQueue.familyIndex = indices.graphicsFamily;
			presentQueue.familyIndex = indices.presentFamily;

			vkGetDeviceQueue(device, graphicsQueue.familyIndex, graphicsQueue.queueIndex, &graphicsQueue.handle);
			vkGetDeviceQueue(device, presentQueue.familyIndex, presentQueue.queueIndex, &presentQueue.handle);

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

	DeviceInit create_device(const DeviceCreateParams& params)
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
			VkResult result = params.windows[0]->get_required_instance_extensions(&windowExtensionCount, nullptr, nullptr);
			VK_ASSERT(result);

			// TODO: extend params vector and write into the offset?
			std::vector<const char*> windowExtensions(windowExtensionCount);
			result = params.windows[0]->get_required_instance_extensions(&windowExtensionCount, windowExtensions.data(), nullptr);
			VK_ASSERT(result);

			for (std::uint32_t i = 0; i < windowExtensionCount; i++) {
				printf("window ext %s\n", windowExtensions[i]);
				extensions.push_back(windowExtensions[i]);
			}
		}

		VkInstance instance = create_instance(params.appname, extensions, layers);

		VkSurfaceKHR surface = VK_NULL_HANDLE;

		std::vector<Window*> windows(params.windows.size());
		for (uint32_t i = 0; i < params.windows.size(); i++) {
			windows[i] = detail::create_window(instance, params.windows[i]);
			surface = windows[i]->surface;
		}

		// always add swapchain extension
		deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

		VkPhysicalDevice physicalDevice = pick_physical_device(instance, deviceExtensions, surface);
		VkSampleCountFlagBits maxMsaaSamples = get_max_usable_sample_count(physicalDevice); // TODO: incorporate desired MSAA samples in device choice?
		VkSampleCountFlagBits msaaSamples = std::clamp(params.msaaSamples, VK_SAMPLE_COUNT_1_BIT, maxMsaaSamples);

		printf("Frames in Flight: %d\n", params.framesInFlight);
		printf("MSAA samples: %d/%d\n", msaaSamples, maxMsaaSamples);

		Queue graphicsQueue = {};
		Queue presentQueue = {};
		VkDevice device = create_logical_device(physicalDevice, surface, deviceExtensions, params.features, graphicsQueue, presentQueue);

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
			.msaaSamples = msaaSamples,
			.enableDepth = params.enableDepth
		};

		for (uint32_t i = 0; i < windows.size(); i++) {
			windows[i]->swapchain = create_swapchain(pDevice, windows[i], params.swapchainFormat);
		}

		pDevice->commandPool = create_command_pool(pDevice, graphicsQueue.familyIndex);
		pDevice->transientPool = create_command_pool(pDevice, graphicsQueue.familyIndex, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
		pDevice->renderPass = create_render_pass(pDevice);

		for (Window* window : windows) {
			std::vector<VkCommandBuffer> commandBuffers = detail::create_command_buffers(pDevice, pDevice->commandPool, params.framesInFlight);
			window->frames.resize(params.framesInFlight);
			for (uint32_t i = 0; i < params.framesInFlight; i++) {
				window->frames[i] = Frame{
					.commands = new CommandBuffer{.commandBuffer = commandBuffers[i]},
					.depthImage = nullptr,
					.inFlightFence = create_fence(pDevice),
					.imageAvailable = create_semaphore(pDevice)
				};
			}
			window->renderTarget = create_render_target(pDevice, window, pDevice->renderPass);
		}

		pDevice->windows = windows;

		if (params.resourcePool.max_sets > 0) {
			pDevice->resourcePool = create_resource_pool(pDevice, params.resourcePool);
		}

		return DeviceInit{
			.device = pDevice
		};
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

		for (uint32_t i = 0; i < device->windows.size(); i++) {
			for (uint32_t j = 0; j < device->windows[i]->frames.size(); j++) {
				destroy_semaphore(device, &device->windows[i]->frames[j].imageAvailable);
				destroy_fence(device, &device->windows[i]->frames[j].inFlightFence);
			}
			destroy_render_target(device, device->windows[i]->renderTarget);
		}

		vkDestroyCommandPool(device->device, device->transientPool, nullptr);
		vkDestroyCommandPool(device->device, device->commandPool, nullptr);
		destroy_render_pass(device, device->renderPass);

		for (uint32_t i = 0; i < device->windows.size(); i++) {
			detail::destroy_window(device, device->windows[i]);
		}

		vkDestroyDevice(device->device, nullptr);

		if (device->debugMessenger != VK_NULL_HANDLE) {
			destroy_debug_utils_messenger_ext(device->instance, device->debugMessenger, nullptr);
		}

		vkDestroyInstance(device->instance, nullptr);

		delete device;
		device = nullptr;
	}
} // namespace gfx
