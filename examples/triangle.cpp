#include <cstdint>
#include "glfw_window.h"
#include <vk_gfx.h>

int main() {
	constexpr std::uint32_t width = 800;
	constexpr std::uint32_t height = 600;
	constexpr bool enableValidationLayers = true;

	std::string appName = "triangle example";
	Window window = create_window(width, height, appName);

	gfx::DeviceInit deviceInit = gfx::create_device({
		.appname = appName,
		.extensions = window.extensions,
		.enableValidation = enableValidationLayers
	});

	gfx::Device* device = deviceInit.device;

	while (poll_window_events(window)) {

	}

	gfx::destroy_device(device);
	close_window(window);

	return  EXIT_SUCCESS;
}
