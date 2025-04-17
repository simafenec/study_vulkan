#include "Core/vulkan_core.h"

// Vulkanの各種関数や構造体、列挙型を提供してくれるヘッダー
#include <vulkan/vulkan.h>

namespace Core {
	void VulkanApplication::InitWindow() {
		glfwInit();
		// GLFWはOpenGLのcontextを作るために設計されているため、まずそれを制御する必要がある。
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		// ウィンドウのリサイズは特別な対応が必要になるためいったんできないようにする
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		// 四つ目のパラメータは画面を開くモニターを指定でき、五つ目のパラメータはOpenGL以外では必要ない
		window_ = glfwCreateWindow(kWIDTH, kHEIGHT, "Vulkan", nullptr, nullptr);
	}

	void VulkanApplication::InitVulkan() {

	}
	void VulkanApplication::MainLoop() {
		// エラーが出るまではウィンドウに対するイベントを観察し続ける
		while (!glfwWindowShouldClose(window_)) {
			glfwPollEvents();
		}
	}

	void VulkanApplication::CleanUp() {
		glfwDestroyWindow(window_);
		glfwTerminate();
	}

	void VulkanApplication::Run() {
		InitWindow();
		InitVulkan();
		MainLoop();
		CleanUp();
	}
}