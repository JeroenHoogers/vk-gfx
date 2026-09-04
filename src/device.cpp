#include <gfx/device.h>
#include <cstdint>
#include <string>
#include <vector>

namespace gfx
{
	namespace
	{
		VkInstance create_instance(const std::string &appName, const std::vector<const char *> &extensions) {
			VkApplicationInfo appInfo{
				.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
				.pNext = nullptr,
				.pApplicationName = appName.c_str(),
				.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
				.pEngineName = "No Engine",
				.engineVersion = VK_MAKE_VERSION(1, 0, 0),
				.apiVersion = VK_API_VERSION_1_3
			};

			printf("extensions %lu\n", extensions.size());
			VkInstanceCreateInfo createInfo{
				.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.pApplicationInfo = &appInfo,
				.enabledLayerCount = 0,
				.ppEnabledLayerNames = nullptr,
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
		VkInstance instance = create_instance(params.appname, params.extensions);
		return DeviceInit{
			.device = new Device{
				.instance = instance,
				.physicalDevice = nullptr,
				.device = nullptr}
		};
	}

	void destroy_device(Device *device) {
		vkDestroyInstance(device->instance, nullptr);
	}
} // namespace gfx
