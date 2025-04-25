/**
* @file vulkan_core.cpp
* @brief Vulkanの描画に関する基本的な処理を実装するクラスの定義ファイル
* @author はっとり
* @date 2025/4/17
*/

#include "Core/vulkan_core.h"

#include<stdexcept>
#include<optional>

namespace Core {
	/**
	* @brief
	* キューファミリーのインデックスを扱う構造体
	*/
	struct QueueFamilyIndices {
		std::optional<uint32_t> graphics_family_;

		/**
		* @fn
		* @brief グラフィックスファミリーの条件を満たしているかどうか
		*/
		bool IsComplete() const {
			return graphics_family_.has_value();
		}
	};


	void VulkanApplication::InitWindow() {
		glfwInit();
		// GLFWはOpenGLのcontextを作るために設計されているため、まずそれを制御する必要がある。
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		// ウィンドウのリサイズは特別な対応が必要になるためいったんできないようにする
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		// 四つ目のパラメータは画面を開くモニターを指定でき、五つ目のパラメータはOpenGL以外では必要ない
		window_ = glfwCreateWindow(kWidth, kHeight, "Vulkan", nullptr, nullptr);
	}

	void VulkanApplication::InitVulkan() {
		CreateVkInstance();
		SetupDebugMessenger();
		PickPhysicalDevice();
		CreateLogicalDevice();
	}
	void VulkanApplication::MainLoop() {
		// エラーが出るまではウィンドウに対するイベントを観察し続ける
		while (!glfwWindowShouldClose(window_)) {
			glfwPollEvents();
		}
	}

	void VulkanApplication::CleanUp() {
		if (kEnableValidationLayers) {
			DestroyDebugUtilsMessenger(nullptr);
		}
		// インスタンスは最後に破棄すること
		vkDestroyDevice(device_, nullptr);
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
		if (kEnableValidationLayers && !CheckValidationLayerSupport()) {
			throw std::runtime_error("バリデーションレイヤーの設定が求められましたが、利用可能ではありません");
		}
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
		auto extensions = GetRequiredExtensions();
		if (!CheckExtensionsAvailable(extensions)) {
			throw std::runtime_error("拡張機能がサポートされていません！");
		}

		create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
		create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		create_info.ppEnabledExtensionNames = extensions.data();

		// インスタンス生成 / 破棄時のエラーデバッグ対応
		// インスタンス生成時の pNextにデバッグ情報を与える
		VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
		// バリデーションレイヤの設定
		if (kEnableValidationLayers) {
			create_info.enabledLayerCount = static_cast<uint32_t>(kValidationLayers.size());
			create_info.ppEnabledLayerNames = kValidationLayers.data();

			// インスタンス生成時のバリデーション情報設定
			InitializeDebugMessengerCreateInfo(debug_create_info);
			create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debug_create_info;
		}
		else {
			create_info.enabledLayerCount = 0;
			create_info.pNext = nullptr;
		}
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

	std::vector<const char*>  VulkanApplication::GetRequiredExtensions() {
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
		if (kEnableValidationLayers) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
		return extensions;
	}

	bool VulkanApplication::CheckValidationLayerSupport() {
		// 使用可能なレイヤーをすべて取得
		uint32_t layer_count;
		vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

		std::vector<VkLayerProperties> available_layers(layer_count);
		vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

		// バリデーションレイヤーが使用可能かどうか確認
		for (const char* layer_name : kValidationLayers) {
			bool layer_found = false;
			for (const VkLayerProperties& prop : available_layers) {
				if (strcmp(layer_name, prop.layerName) == 0) {
					layer_found = true;
					break;
				}
			}
			if (!layer_found) {
				return false;
			}
		}
		return true;
	}
	void VulkanApplication::InitializeDebugMessengerCreateInfo(
		VkDebugUtilsMessengerCreateInfoEXT& create_info				// デバッグメッセンジャー生成情報インスタンスへの参照
	) {
		create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		// フラグはコールバックを呼び出すメッセージの種類を設定する
		create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		create_info.pfnUserCallback = DebugCallBack;
		create_info.pUserData = nullptr;
	}

	void VulkanApplication::SetupDebugMessenger() {
		if (!kEnableValidationLayers) {
			return;
		}
		VkDebugUtilsMessengerCreateInfoEXT create_info;
		InitializeDebugMessengerCreateInfo(create_info);

		if (CreateDebugUtilsMessenger(&create_info, nullptr) != VK_SUCCESS) {
			throw std::runtime_error("デバッグメッセンジャーの設定に失敗しました。");
		}
	}

	VkResult VulkanApplication::CreateDebugUtilsMessenger(
		const VkDebugUtilsMessengerCreateInfoEXT* p_create_info,	// デバッグメッセンジャーの生成情報
		const VkAllocationCallbacks* p_allocator					// カスタムアロケーターへのポインタ
	) {
		// デバッグメッセンジャーを生成する関数は拡張機能であるためデフォルトでロードされない
		// vkGetInstanceProcAddr 関数を使って関数のポインタを検索する必要がある。
		auto proc = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance_, "vkCreateDebugUtilsMessengerEXT");
		if (proc != nullptr) {
			return proc(instance_, p_create_info, p_allocator, &debug_messenger_);
		}
		else {
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	void VulkanApplication::DestroyDebugUtilsMessenger(
		VkAllocationCallbacks* p_allocator
	) {
		auto proc = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance_, "vkDestroyDebugUtilsMessengerEXT");
		if (proc != nullptr) {
			proc(instance_, debug_messenger_, p_allocator);
		}
	}

	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) {
		QueueFamilyIndices indices;
		uint32_t queue_family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
		std::vector<VkQueueFamilyProperties> queue_family_properties(queue_family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_family_properties.data());
		// グラフィックコマンドをサポートしているキューのインデックスを取得する。
		int graphics_queue_index = 0;
		for (const auto& prop : queue_family_properties) {
			if (prop.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphics_family_ = graphics_queue_index;
			}
			if (indices.IsComplete()) {
				break;
			}
			graphics_queue_index += 1;
		}
		return indices;
	}
	/**
	* @fn
	* @brief
	* デバイスが適当かどうかを確認する。
	*/
	bool IsDeviceSuitable(VkPhysicalDevice device) {
		/*
		VkPhysicalDeviceProperties device_properties;
		VkPhysicalDeviceFeatures device_features;
		vkGetPhysicalDeviceProperties(device, &device_properties);
		vkGetPhysicalDeviceFeatures(device, &device_features);

		return device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
			&& device_features.geometryShader;
		*/
		QueueFamilyIndices indices = FindQueueFamilies(device);
		return indices.IsComplete();
	}
	void VulkanApplication::PickPhysicalDevice() {
		uint32_t device_count = 0;
		vkEnumeratePhysicalDevices(instance_, &device_count, nullptr);
		if (device_count == 0) {
			throw std::runtime_error("Vulkanをサポートしているデバイスが存在しません");
		}
		std::vector<VkPhysicalDevice> devices(device_count);
		vkEnumeratePhysicalDevices(instance_, &device_count, devices.data());
		for (const auto& device : devices) {
			if (IsDeviceSuitable(device)) {
				physical_device_ = device;
				break;
			}
		}
		if (physical_device_ == VK_NULL_HANDLE) {
			throw std::runtime_error("条件に適したGPUが存在しませんでした");
		}
	}

	void VulkanApplication::CreateLogicalDevice() {
		// キューの情報をまずまとめる。
		QueueFamilyIndices indices = FindQueueFamilies(physical_device_);

		VkDeviceQueueCreateInfo queue_create_info{};
		queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_info.queueFamilyIndex = indices.graphics_family_.value();
		// 現在利用可能なドライバでは、キューファミリーごとに少数のキューしか作成できず、実際には複数のキューは必要ない。
		// これは、すべてのコマンドバッファを複数のスレッドで作成し、
		// オーバーヘッドの少ない単一の呼び出しでメインスレッドに一括送信できるためである。
		queue_create_info.queueCount = 1;
		// キューの優先度はたとえキューが一つでも設定する必要がある。
		float queue_priority = 1.0f;
		queue_create_info.pQueuePriorities = &queue_priority;

		// 論理デバイスの特徴を次にまとめる。
		// 現状は何も設定しない。　今後様々な機能を使いたくなった時に適宜フラグを立てていく
		VkPhysicalDeviceFeatures device_features{};
		
		// 論理デバイスの生成情報を次にまとめる。
		VkDeviceCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		create_info.pQueueCreateInfos = &queue_create_info;
		create_info.queueCreateInfoCount = 1;
		create_info.pEnabledFeatures = &device_features;

		// デバイス固有の拡張機能やバリデーションレイヤを設定する。
		// 拡張機能やバリデーションレイヤはVkInstanceで設定するものと同じである。
		create_info.enabledExtensionCount = 0;
		if (kEnableValidationLayers) {
			create_info.enabledLayerCount = static_cast<uint32_t>(kValidationLayers.size());
			create_info.ppEnabledLayerNames = kValidationLayers.data();
		}
		else {
			create_info.enabledLayerCount = 0;
		}
		if (vkCreateDevice(physical_device_, &create_info, nullptr, &device_) != VK_SUCCESS) {
			throw std::runtime_error("論理デバイスの生成に失敗しました！");
		}
		vkGetDeviceQueue(device_, indices.graphics_family_.value(), 0, &graphics_queue_);
	}
}