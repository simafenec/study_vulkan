#include "Core/VulkanCore.h"

// Vulkanの各種関数や構造体、列挙型を提供してくれるヘッダー
#include <vulkan/vulkan.h>

namespace Core {
	void VulkanApplication::initWindow() {
		glfwInit();
		// GLFWはOpenGLのcontextを作るために設計されているため、まずそれを制御する必要がある。
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		// ウィンドウのリサイズは特別な対応が必要になるためいったんできないようにする
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		// 四つ目のパラメータは画面を開くモニターを指定でき、五つ目のパラメータはOpenGL以外では必要ない
		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	}

	void VulkanApplication::initVulkan() {

	}
	void VulkanApplication::mainLoop() {
		// エラーが出るまではウィンドウに対するイベントを観察し続ける
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
		}
	}

	void VulkanApplication::cleanUp() {
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void VulkanApplication::run() {
		initWindow();
		initVulkan();
		mainLoop();
		cleanUp();
	}
}