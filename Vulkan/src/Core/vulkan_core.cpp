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
		CreateVkApplicationInfo(app_info);
		VkInstanceCreateInfo create_info{};
		CreateVkInstanceCreateInfo(create_info, app_info);
		// 二番目の引数はカスタムアロケーターへのポインタ
		if (vkCreateInstance(&create_info, nullptr, &instance_) != VK_SUCCESS) {
			throw std::runtime_error("VkInstanceの生成に失敗しました！");
		}
	}

	/**
	* @fn
	* @brief Vulkan アプリケーションの基本情報を設定する
	*/
	void VulkanApplication::CreateVkApplicationInfo(VkApplicationInfo& info) {
		// Vulkanでは構造体のsTypeメンバーで明示的に構造体の型を設定する必要がある。
		// pNext メンバで拡張情報を設定することもできるが、とりあえずnullptrのままにする
		info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		info.pApplicationName = "Triangle";
		info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		info.pEngineName = "No Engine";
		info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		info.apiVersion = VK_API_VERSION_1_0;
	}

	/**
	* @fn
	* @brief VkInstanceの生成設定を行う
	* @detail デバイスにかかわらず全てのプログラムに適用する拡張(extensions)と使用するバリデーションレイヤの設定を行う。
	*/
	void VulkanApplication::CreateVkInstanceCreateInfo(VkInstanceCreateInfo& create_info, const VkApplicationInfo& application_info) {
		create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		create_info.pApplicationInfo = &application_info;

		// global extension の設定
		// Vulkanはプラットフォーム非依存であるため、ウィンドウシステムと接続するには拡張機能が必要
		uint32_t glfw_extension_count = 0;
		// 文字列の配列なのでポインタのポインタになる
		const char** glfw_extensions;

		glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

		uint32_t extension_count = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
		// MacOSではVK_ERROR_INCOMPATIBLE_DRIVERエラーが発生する
		// これに対応するには、VK_KHR_PORTABILITY_subset 拡張を追加する必要がある。
		//std::vector<const char*> required_extensions;
		//for (uint32_t index = 0; index < glfw_extension_count; index++) {
		//	required_extensions.emplace_back(glfw_extensions[index]);
		//}
		//required_extensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

		//create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

		//if (!CheckExtensionsAvailable(required_extensions)) {
		//	throw std::runtime_error("拡張機能がサポートされていません！");
		//}
		//create_info.enabledExtensionCount = (uint32_t)required_extensions.size();
		//create_info.ppEnabledExtensionNames = required_extensions.data();
		create_info.enabledExtensionCount = glfw_extension_count;
		create_info.ppEnabledExtensionNames = glfw_extensions;

		// バリデーションレイヤの設定
		create_info.enabledLayerCount = 0;
	}

	bool VulkanApplication::CheckExtensionsAvailable(std::vector<const char*> required_instance) {
		uint32_t extension_count = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
		// 要素数を取得してから拡張機能配列を用意して名前を取得
		std::vector<VkExtensionProperties> extensions(extension_count);
		vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());
		// 拡張機能名の配列を作る
		std::vector<const char*> extension_names;
		for (const auto& property : extensions) {
			extension_names.emplace_back(property.extensionName);
		}
		// 含まれているかどうかを確認する
		for (const auto& instance : required_instance) {
			if (std::find(extension_names.begin(), extension_names.end(), instance) == extension_names.end()) {
				return false;
			}
		}
		return true;
	}
}