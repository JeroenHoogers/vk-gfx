#include <cstdint>
#include <cstring>
#include <gfx/device.h>
#include <string>
#include <vector>

namespace gfx
{
	namespace
	{
		// TODO: allow user callback instead
		VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_callback([[maybe_unused]] VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, [[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, [[maybe_unused]] void* userData) {
			std::printf("validation layer: %s\n", callbackData->pMessage);
			return VK_FALSE;
		}

		VkDebugUtilsMessengerEXT create_debug_utils_messenger_ext(VkInstance instance, const VkAllocationCallbacks* allocator) {
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

		void destroy_debug_utils_messenger_ext(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* allocator) {
			auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
			if (func != nullptr) {
				func(instance, debugMessenger, allocator);
			}
		}

		VkInstance create_instance(const std::string& appName, const std::vector<const char*>& extensions, const std::vector<const char*>& layers) {
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

		bool check_layer_support(std::vector<const char*> layers) {
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

		bool find_queue_families(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, QueueFamilyIndices& indices) {
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

		bool is_device_suitable(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
			QueueFamilyIndices indices;

			bool has_queue_families = find_queue_families(physicalDevice, surface, indices);

			VkPhysicalDeviceProperties deviceProperties;
			VkPhysicalDeviceFeatures deviceFeatures;
			vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
			vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

			return has_queue_families;
		}

		VkPhysicalDevice pick_physical_device(VkInstance instance, VkSurfaceKHR surface) {
			std::uint32_t deviceCount = 0;
			vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

			if (deviceCount == 0) {
				fprintf(stderr, "failed to find GPUs with Vulkan support!");
				abort();
			}

			std::vector<VkPhysicalDevice> devices(deviceCount);
			vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

			for (const VkPhysicalDevice& physicalDevice : devices) {
				if (is_device_suitable(physicalDevice, surface)) {
					printf("found physical device!\n");
					return physicalDevice;
				}
			}

			fprintf(stderr, "failed to find a suitable GPU!");
			abort();
		}

		VkDevice create_logical_device(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, const std::vector<const char*>& extensions, VkQueue& graphicsQueue, VkQueue& presentQueue) {
			QueueFamilyIndices indices;
			if (!find_queue_families(physicalDevice, surface, indices)) {
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

			VkPhysicalDeviceFeatures deviceFeatures{};
			deviceFeatures.samplerAnisotropy = VK_TRUE;

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
				.pEnabledFeatures = &deviceFeatures
			};

			VkDevice device;
			VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
			VK_ASSERT(result);

			vkGetDeviceQueue(device, indices.graphicsFamily, 0, &graphicsQueue);
			vkGetDeviceQueue(device, indices.presentFamily, 0, &presentQueue);

			return device;
		}

	} // namespace

	DeviceInit create_device(DeviceCreateParams params) {
		constexpr const char* validationLayers = "VK_LAYER_KHRONOS_validation";

		if (params.enableValidation) {
			params.extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			params.layers.push_back(validationLayers);
		}

		if (!check_layer_support(params.layers)) {
			// printf("Layer")
		}

		// add window instance extensions
		if (params.window) {
			std::uint32_t windowExtensionCount = 0;
			VkResult result = params.window->get_required_instance_extensions(&windowExtensionCount, nullptr, nullptr);
			VK_ASSERT(result);

			// TODO: extend params vector and write into the offset?
			std::vector<const char*> windowExtensions(windowExtensionCount);
			result = params.window->get_required_instance_extensions(&windowExtensionCount, windowExtensions.data(), nullptr);
			VK_ASSERT(result);

			for (std::uint32_t i = 0; i < windowExtensionCount; i++) {
				printf("window ext %s\n", windowExtensions[i]);
				params.extensions.push_back(windowExtensions[i]);
			}
		}

		VkInstance instance = create_instance(params.appname, params.extensions, params.layers);
		VkSurfaceKHR surface = VK_NULL_HANDLE;

		// create surface
		if (params.window) {
			VkResult result = params.window->create_surface(instance, &surface, params.window->user_data);
			VK_ASSERT(result);
		}

		VkPhysicalDevice physicalDevice = pick_physical_device(instance, surface);

		VkQueue graphicsQueue = VK_NULL_HANDLE;
		VkQueue presentQueue = VK_NULL_HANDLE;
		VkDevice device = create_logical_device(physicalDevice, surface, params.deviceExtensions, graphicsQueue, presentQueue);

		VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
		if (params.enableValidation) {
			debugMessenger = create_debug_utils_messenger_ext(instance, nullptr);
		}

		Device* pDevice = new Device{
			.instance = instance,
			.physicalDevice = physicalDevice,
			.device = device,
			.surface = surface,
			.debugMessenger = debugMessenger,
			.graphicsQueue = graphicsQueue,
			.presentQueue = presentQueue
		};

		return DeviceInit{
			.device = pDevice
		};
	}

	void destroy_device(Device* device) {
		if (device->debugMessenger != VK_NULL_HANDLE) {
			destroy_debug_utils_messenger_ext(device->instance, device->debugMessenger, nullptr);
		}

		vkDestroySurfaceKHR(device->instance, device->surface, nullptr);
		vkDestroyDevice(device->device, nullptr);
		vkDestroyInstance(device->instance, nullptr);
	}
} // namespace gfx
