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

		bool find_queue_families(VkPhysicalDevice physicalDevice, QueueFamilyIndices& indices) {
			std::uint32_t queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

			std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

			for (std::uint32_t i = 0; i < queueFamilyCount; i++) {
				// bool hasGraphics = false;
				if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
					indices.graphicsFamily = i;
					return true;
					// hasGraphics = true;
				}

				// VkBool32 presentSupport = false;
				// vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);

				// if (presentSupport) {
				//     indices.presentFamily = i;
				// }

				// if (hasGraphics && presentSupport)
				// break;
			}

			return false;
		}

		bool is_device_suitable(VkPhysicalDevice physicalDevice) {
			QueueFamilyIndices indices;

			bool has_queue_families = find_queue_families(physicalDevice, indices);

			VkPhysicalDeviceProperties deviceProperties;
			VkPhysicalDeviceFeatures deviceFeatures;
			vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
			vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

			return has_queue_families;
		}

		VkPhysicalDevice pick_physical_device(VkInstance instance) {
			std::uint32_t deviceCount = 0;
			vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

			if (deviceCount == 0) {
				fprintf(stderr, "failed to find GPUs with Vulkan support!");
				abort();
			}

			std::vector<VkPhysicalDevice> devices(deviceCount);
			vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

			for (const VkPhysicalDevice& physicalDevice : devices) {
				if (is_device_suitable(physicalDevice)) {
					printf("found physical device!\n");
					return physicalDevice;
				}
			}

			fprintf(stderr, "failed to find a suitable GPU!");
			abort();
		}

		VkDevice create_logical_device(VkPhysicalDevice physicalDevice, const std::vector<const char*>& extensions, VkQueue& graphicsQueue) {
			QueueFamilyIndices indices;
			if (!find_queue_families(physicalDevice, indices)) {
			}

			float queuePriority = 1.0f;

			VkDeviceQueueCreateInfo queueCreateInfo{
				.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.queueFamilyIndex = indices.graphicsFamily,
				.queueCount = 1,
				.pQueuePriorities = &queuePriority
			};

			VkPhysicalDeviceFeatures deviceFeatures{};
			deviceFeatures.samplerAnisotropy = VK_TRUE;

			VkDeviceCreateInfo createInfo{
				.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.queueCreateInfoCount = 1,
				.pQueueCreateInfos = &queueCreateInfo,
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

			return device;
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

		VkInstance instance = create_instance(params.appname, params.extensions, params.layers);
		VkPhysicalDevice physicalDevice = pick_physical_device(instance);

		VkQueue graphicsQueue = VK_NULL_HANDLE;
		VkDevice device = create_logical_device(physicalDevice, params.deviceExtensions, graphicsQueue);

		VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
		if (params.enableValidation) {
			debugMessenger = create_debug_utils_messenger_ext(instance, nullptr);
		}

		Device* pDevice = new Device{
			.instance = instance,
			.physicalDevice = physicalDevice,
			.device = device,
			.debugMessenger = debugMessenger,
			.graphicsQueue = graphicsQueue
		};

		return DeviceInit{
			.device = pDevice
		};
	}

	void destroy_device(Device* device) {
		if (device->debugMessenger != VK_NULL_HANDLE) {
			destroy_debug_utils_messenger_ext(device->instance, device->debugMessenger, nullptr);
		}

		vkDestroyInstance(device->instance, nullptr);
	}
} // namespace gfx
