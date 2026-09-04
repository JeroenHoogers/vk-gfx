#pragma once
#include <vulkan/vulkan.h>

namespace gfx {

	struct DeviceInit {

	};

	struct Device
	{
		VkInstance          instance;
		VkPhysicalDevice    physicalDevice;
		VkDevice			device;
	};

	Device* create_device(DeviceInit init);

	void destroy_device(Device* device);
}
