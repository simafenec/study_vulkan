#include "Core/vulkan_core.h"

#include<stdexcept>
#include<iostream>

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
		CreateVkInstance();
	}
	void VulkanApplication::MainLoop() {
		// エラーが出るまではウィンドウに対するイベントを観察し続ける
		while (!glfwWindowShouldClose(window_)) {
			glfwPollEvents();
		}
	}

	void VulkanApplication::CleanUp() {
		vkDestroyInstance(instance_, nullptr);
		glfwDestroyWindow(window_);
		glfwTerminate();
	}

	void VulkanApplication::Run() {
		InitWindow();
		InitVulkan();
		MainLoop();
		CleanUp();
	}

	void VulkanApplication::CreateVkInstance() {
		VkApplicationInfo app_info{};
		// Vulkanでは構造体のsTypeメンバーで明示的に構造体の型を設定する必要がある。
		// pNext メンバで拡張情報を設定することもできるが、とりあえずnullptrのままにする
		app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.pApplicationName = "Triangle";
		app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		app_info.pEngineName = "No Engine";
		app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		app_info.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		create_info.pApplicationInfo = &app_info;
		auto extensions = getRequiredExtensions();
		if (!CheckExtensionsAvailable(extensions)) {
			throw std::runtime_error("拡張機能がサポートされていません！");
		}

		create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
		create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		create_info.ppEnabledExtensionNames = extensions.data();
		// バリデーションレイヤの設定
		create_info.enabledLayerCount = 0;
		// 二番目の引数はカスタムアロケーターへのポインタ
		VkResult result = vkCreateInstance(&create_info, nullptr, &instance_);
		if (result != VK_SUCCESS) {
			throw std::runtime_error("VkInstanceの生成に失敗しました！");
		}
	}

	bool VulkanApplication::CheckExtensionsAvailable(std::vector<const char*> required_instance) {
		uint32_t extension_count = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
		// 要素数を取得してから拡張機能配列を用意して名前を取得
		std::vector<VkExtensionProperties> extensions(extension_count);
		vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());

		// 含まれているかどうかを確認する
		for (const char* instance : required_instance) {
			bool isAvailable = false;
			for (const auto& prop : extensions) {
				if (strcmp(instance, prop.extensionName) == 0) {
					isAvailable = true;
					break;
				}
			}
			if (!isAvailable) {
				return false;
			}
		}
		return true;
	}

	std::vector<const char*>  VulkanApplication::getRequiredExtensions() {
		// global extension の設定
		// Vulkanはプラットフォーム非依存であるため、ウィンドウシステムと接続するには拡張機能が必要
		uint32_t glfw_extension_count = 0;
		// 文字列の配列なのでポインタのポインタになる
		const char** glfw_extensions;

		glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
		// MacOSではVK_ERROR_INCOMPATIBLE_DRIVERエラーが発生する
		// これに対応するには、VK_KHR_PORTABILITY_subset 拡張を追加する必要がある。
		std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);
		extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

		return extensions;
	}
}