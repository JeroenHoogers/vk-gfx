#include <cstdint>
#include "glfw_window.h"

int main() {
	constexpr std::uint32_t width = 512;
	constexpr std::uint32_t height = 512;

	Window window = create_window(width, height, "triangle example");

	destroy_window(window);
}
