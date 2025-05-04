/**
* @file vulkan_core.cpp
* @brief Vulkanの描画に関する基本的な処理を実装するクラスの定義ファイル
* @author はっとり
* @date 2025/4/17
*/

#include "Core/vulkan_core.h"

#include<stdexcept>
#include<set>
#include<cstdint>
#include<limits>
#include<algorithm>

namespace Core {
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
		CreateSurface();
		PickPhysicalDevice();
		CreateLogicalDevice();
		CreateSwapChain();
		CreateImageViews();
		CreateRenderPass();
		CreateGraphicsPipeline();
		CreateFramebuffers();
		CreateCommandPool();
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
		vkDestroyCommandPool(device_, command_pool_, nullptr);
		for (auto framebuffer : swap_chain_frame_buffers_) {
			vkDestroyFramebuffer(device_, framebuffer, nullptr);
		}
		for (auto image_view : swap_chain_image_views_) {
			vkDestroyImageView(device_, image_view, nullptr);
		}
		// インスタンスは最後に破棄すること
		vkDestroyPipeline(device_, graphics_pipeline_, nullptr);
		vkDestroyPipelineLayout(device_, pipeline_layout_, nullptr);
		vkDestroyRenderPass(device_, render_pass_, nullptr);
		vkDestroySwapchainKHR(device_, swap_chain_, nullptr);
		vkDestroyDevice(device_, nullptr);
		vkDestroySurfaceKHR(instance_, surface_, nullptr);
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

	QueueFamilyIndices VulkanApplication::FindQueueFamilies(VkPhysicalDevice device) {
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
			// Presentation Queueの存在チェック
			VkBool32 presentSupported = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, graphics_queue_index, surface_, &presentSupported);
			if (presentSupported) {
				indices.present_family_ = graphics_queue_index;
			}
			if (indices.IsComplete()) {
				break;
			}
			graphics_queue_index += 1;
		}
		return indices;
	}

	bool VulkanApplication::IsDeviceSuitable(VkPhysicalDevice device) {
		QueueFamilyIndices indices = FindQueueFamilies(device);
		bool extension_supported = CheckDeviceExtensionSupport(device);
		bool swap_chain_adequate = false;
		if (extension_supported) {
			SwapChainSupportDetails swap_chain_support = QuerySwapChainSupprot(device);
			swap_chain_adequate = !swap_chain_support.formats_.empty() && !swap_chain_support.present_modes_.empty();
		}
		return indices.IsComplete() && extension_supported && swap_chain_adequate;
	}
	bool VulkanApplication::CheckDeviceExtensionSupport(VkPhysicalDevice device) {
		uint32_t extension_count = 0;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);
		std::vector<VkExtensionProperties> available_extensions(extension_count);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

		std::set<std::string> required_extensions(kDeviceExtensions.begin(), kDeviceExtensions.end());
		// 要求されている拡張機能名のリストからサポートされている拡張機能名を除いていき、要求リストが空になればOK
		for (const auto& prop : available_extensions) {
			required_extensions.erase(prop.extensionName);
		}
		return required_extensions.empty();
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

		std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
		std::set<uint32_t> unique_queue_families = { indices.graphics_family_.value(), indices.present_family_.value() };
		float queue_priority = 1.0f;
		for (uint32_t queue_family : unique_queue_families) {
			VkDeviceQueueCreateInfo queue_create_info{};
			queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queue_create_info.queueFamilyIndex = queue_family;
			// 現在利用可能なドライバでは、キューファミリーごとに少数のキューしか作成できず、実際には複数のキューは必要ない。
			// これは、すべてのコマンドバッファを複数のスレッドで作成し、
			// オーバーヘッドの少ない単一の呼び出しでメインスレッドに一括送信できるためである。
			queue_create_info.queueCount = 1;
			// キューの優先度はたとえキューが一つでも設定する必要がある。
			queue_create_info.pQueuePriorities = &queue_priority;
			queue_create_infos.push_back(queue_create_info);
		}

		// 論理デバイスの特徴を次にまとめる。
		// 現状は何も設定しない。　今後様々な機能を使いたくなった時に適宜フラグを立てていく
		VkPhysicalDeviceFeatures device_features{};
		
		// 論理デバイスの生成情報を次にまとめる。
		VkDeviceCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		create_info.pQueueCreateInfos = queue_create_infos.data();
		create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
		create_info.pEnabledFeatures = &device_features;

		// デバイス固有の拡張機能やバリデーションレイヤを設定する。
		// 拡張機能やバリデーションレイヤはVkInstanceで設定するものと同じである。
		create_info.enabledExtensionCount = static_cast<uint32_t>(kDeviceExtensions.size());
		create_info.ppEnabledExtensionNames = kDeviceExtensions.data();
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
		vkGetDeviceQueue(device_, indices.present_family_.value(), 0, &present_queue_);
	}

	void VulkanApplication::CreateSurface() {
		// glfwのウィンドウサーフェス生成を使う
		// 環境に依存せず生成できる
		if (glfwCreateWindowSurface(instance_, window_, nullptr, &surface_) != VK_SUCCESS) {
			throw std::runtime_error("ウィンドウサーフェスの生成に失敗しました！");
		}
	}

	SwapChainSupportDetails VulkanApplication::QuerySwapChainSupprot(VkPhysicalDevice device) {
		SwapChainSupportDetails details;
		// サーフェースの基本情報を得る
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_, &details.capabilities_);
		// フォーマット情報を得る
		uint32_t format_count;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &format_count, nullptr);
		if (format_count != 0) {
			details.formats_.resize(format_count);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &format_count, details.formats_.data());
		}
		// 表示モード情報を得る
		uint32_t present_mode_count;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &present_mode_count, nullptr);
		if (present_mode_count != 0) {
			details.present_modes_.resize(present_mode_count);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &present_mode_count, details.present_modes_.data());
		}
		return details;
	}

	VkSurfaceFormatKHR VulkanApplication::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats) {
		for (const auto& available_format : available_formats) {
			if (
				available_format.format == VK_FORMAT_B8G8R8A8_SRGB && 
				available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
			) {
				return available_format;
			}
		}
		// 求めているものがなかった場合は先頭のものをとりあえず返す
		return available_formats[0];
	}
	VkPresentModeKHR VulkanApplication::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes) {
		for (const auto& available_present_mode : available_present_modes) {
			if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return available_present_mode;
			}
		}
		return VK_PRESENT_MODE_FIFO_KHR;
	}
	VkExtent2D VulkanApplication::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			return capabilities.currentExtent;
		}
		else {
			// 現在の画面のバッファサイズを取得し、スワップチェイン画像の最大 / 最小画素数との間に入るように設定する。
			int width, height;
			glfwGetFramebufferSize(window_, &width, &height);
			VkExtent2D actual_extent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};
			actual_extent.width = std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actual_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
			return actual_extent;
		}
	}
	void VulkanApplication::CreateSwapChain() {
		SwapChainSupportDetails swap_chain_support = QuerySwapChainSupprot(physical_device_);

		VkSurfaceFormatKHR surface_format = ChooseSwapSurfaceFormat(swap_chain_support.formats_);
		VkPresentModeKHR present_mode = ChooseSwapPresentMode(swap_chain_support.present_modes_);
		VkExtent2D extent = ChooseSwapExtent(swap_chain_support.capabilities_);

		// スワップチェインが持てる画像数は余裕を持っておくと吉
		uint32_t image_count = swap_chain_support.capabilities_.minImageCount + 1;
		if (swap_chain_support.capabilities_.maxImageCount > 0 && image_count > swap_chain_support.capabilities_.maxImageCount) {
			image_count = swap_chain_support.capabilities_.maxImageCount;
		}

		VkSwapchainCreateInfoKHR create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		create_info.surface = surface_;
		create_info.minImageCount = image_count;
		create_info.imageFormat = surface_format.format;
		create_info.imageColorSpace = surface_format.colorSpace;
		create_info.imageExtent = extent;
		create_info.imageArrayLayers = 1; // 画像が構成するレイヤーの量。三次元の3Dアプリケーション(VR的な)でないかぎりは1
		create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // スワップチェイン内の画像をどのような操作に使うかを指定する。今回は直接レンダリングする
		// キューファミリーの間で使用されるスワップチェイン画像の処理方法を指定する。
		// グラフィックスキューとプレゼンテーションキューが異なる場合は画像をキュー間で共有できるように設定する。
		// 明示的に所有権をやり取りすることで共有しないようにすることもできる
		QueueFamilyIndices indices = FindQueueFamilies(physical_device_);
		uint32_t queue_family_indices[] = { indices.graphics_family_.value(), indices.present_family_.value() };
		if (indices.graphics_family_ != indices.present_family_) {
			create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			create_info.queueFamilyIndexCount = 2;
			create_info.pQueueFamilyIndices = queue_family_indices;
		}
		else {
			create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			create_info.queueFamilyIndexCount = 0;
			create_info.pQueueFamilyIndices = nullptr;
		}
		// 画像に対する事前の変換処理を指定する。ここではそのままにする
		create_info.preTransform = swap_chain_support.capabilities_.currentTransform;
		// ウィンドウシステム内のほかのウィンドウとのブレンドにアルファチャンネルを使うかどうか
		// 今回は無視する設定
		create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		create_info.presentMode = present_mode;
		// 他のウィンドウが前にあるなどして隠されているピクセルの色は考慮されない
		create_info.clipped = VK_TRUE;
		// 古いスワップチェインへの参照 ウィンドウのサイズが変更になった際などに設定する必要がある。
		create_info.oldSwapchain = VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(device_, &create_info, nullptr, &swap_chain_) != VK_SUCCESS) {
			throw std::runtime_error("スワップチェインの生成に失敗しました!");
		}
		// スワップチェイン内の画像へのハンドルを取得する。
		vkGetSwapchainImagesKHR(device_, swap_chain_, &image_count, nullptr);
		swap_chain_images_.resize(image_count);
		vkGetSwapchainImagesKHR(device_, swap_chain_, &image_count, swap_chain_images_.data());
		// スワップチェインの設定内容を今後の参照のために保存しておく
		swap_chain_image_format_ = surface_format.format;
		swap_chain_extent_ = extent;
	}

	void VulkanApplication::CreateImageViews() {
		swap_chain_image_views_.resize(swap_chain_images_.size());
		for (size_t index = 0; index < swap_chain_images_.size(); index++) {
			VkImageViewCreateInfo create_info{};
			create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			create_info.image = swap_chain_images_[index];
			create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
			create_info.format = swap_chain_image_format_;
			create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			create_info.subresourceRange.baseMipLevel = 0;
			create_info.subresourceRange.levelCount = 1;
			create_info.subresourceRange.baseArrayLayer = 0;
			create_info.subresourceRange.layerCount = 1;

			if (vkCreateImageView(device_, &create_info, nullptr, &swap_chain_image_views_[index]) != VK_SUCCESS) {
				throw std::runtime_error("イメージビューの生成に失敗しました！");
			}
		}
	}

	VkShaderModule VulkanApplication::CreateShaderModule(const std::vector<char>& code) {
		VkShaderModuleCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		create_info.codeSize = code.size();
		// uint32_tのポインタにキャストしてあげる必要がある。
		// vectorはデフォルトでアロケータを持つのでアラインメント要件を気にしなくてもよい
		create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());
		VkShaderModule shader_module;
		if (vkCreateShaderModule(device_, &create_info, nullptr, &shader_module) != VK_SUCCESS) {
			throw std::runtime_error("シェーダーモジュールの生成に失敗しました！");
		}
		return shader_module;
	}
	
	void VulkanApplication::CreateRenderPass() {
		VkAttachmentDescription color_attachment{};
		color_attachment.format = swap_chain_image_format_;
		color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference color_attachment_reference{};
		color_attachment_reference.attachment = 0;
		color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &color_attachment_reference;

		VkRenderPassCreateInfo render_pass_info{};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_info.attachmentCount = 1;
		render_pass_info.pAttachments = &color_attachment;
		render_pass_info.subpassCount = 1;
		render_pass_info.pSubpasses = &subpass;

		if (vkCreateRenderPass(device_, &render_pass_info, nullptr, &render_pass_) != VK_SUCCESS) {
			throw std::runtime_error("レンダーパスの生成に失敗しました");
		}
	}

	void VulkanApplication::CreateGraphicsPipeline() {
		// 頂点シェーダーとフラグメントシェーダーを設定する。
		auto vert_shader_code = ReadFile("shaders/vert.spv");
		auto frag_shader_code = ReadFile("shaders/frag.spv");

		VkShaderModule vert_shader_module = CreateShaderModule(vert_shader_code);
		VkShaderModule frag_shader_module = CreateShaderModule(frag_shader_code);

		VkPipelineShaderStageCreateInfo vert_shader_stage_info{};
		vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vert_shader_stage_info.module = vert_shader_module;
		// シェーダーのエントリポイント名を設定する
		vert_shader_stage_info.pName = "main";
		
		VkPipelineShaderStageCreateInfo frag_shader_stage_info{};
		frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		frag_shader_stage_info.module = frag_shader_module;
		// シェーダーのエントリポイント名を設定する
		frag_shader_stage_info.pName = "main";
		// pSpecializationInfo をここでさらに指定すると、パイプライン作成時にその動作を設定することができる

		VkPipelineShaderStageCreateInfo shader_stages[] = { vert_shader_stage_info, frag_shader_stage_info };

		// 頂点入力の形式に関する情報を与える
		VkPipelineVertexInputStateCreateInfo vertex_input_info{};
		vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertex_input_info.vertexBindingDescriptionCount = 0;
		vertex_input_info.pVertexBindingDescriptions = nullptr;
		vertex_input_info.vertexAttributeDescriptionCount = 0;
		vertex_input_info.pVertexAttributeDescriptions = nullptr;

		// Input Assembly を設定する
		VkPipelineInputAssemblyStateCreateInfo input_assembly{};
		input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		input_assembly.primitiveRestartEnable = VK_FALSE;

		// Viewportを設定する
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(swap_chain_extent_.width);
		viewport.height = static_cast<float>(swap_chain_extent_.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		//Scissor 矩形を設定する
		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = swap_chain_extent_;

		// 動的に描画設定を与えるステージを設定

		std::vector<VkDynamicState> dynamic_states = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamic_state{};
		dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic_state.pDynamicStates = dynamic_states.data();
		dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());

		VkPipelineViewportStateCreateInfo viewport_state{};
		viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		// 動的にこれらは設定するので、カウントだけ設定しておく
		viewport_state.viewportCount = 1;
		viewport_state.scissorCount = 1;

		// ラスタライザの設定
		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f;
		rasterizer.depthBiasClamp = 0.0f;
		rasterizer.depthBiasSlopeFactor = 0.0f;

		// マルチサンプリング設定
		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f;
		multisampling.pSampleMask = nullptr;
		multisampling.alphaToCoverageEnable = VK_FALSE;
		multisampling.alphaToOneEnable = VK_FALSE;

		// 深度テストとステンシルテストの設定

		// Color Blending設定
		VkPipelineColorBlendAttachmentState color_blend_attachment{};
		color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		color_blend_attachment.blendEnable = VK_FALSE;
		color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
		color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo color_blending{};
		color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		color_blending.logicOpEnable = VK_FALSE;
		color_blending.logicOp = VK_LOGIC_OP_COPY;
		color_blending.attachmentCount = 1;
		color_blending.pAttachments = &color_blend_attachment;
		color_blending.blendConstants[0] = 0.0f;
		color_blending.blendConstants[1] = 0.0f;
		color_blending.blendConstants[2] = 0.0f;
		color_blending.blendConstants[3] = 0.0f;

		VkPipelineLayoutCreateInfo pipeline_layout_info{};
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.setLayoutCount = 0;
		pipeline_layout_info.pSetLayouts = nullptr;
		pipeline_layout_info.pushConstantRangeCount = 0;
		pipeline_layout_info.pPushConstantRanges = nullptr;

		if (vkCreatePipelineLayout(device_, &pipeline_layout_info, nullptr, &pipeline_layout_) != VK_SUCCESS) {
			throw std::runtime_error("パイプラインレイアウト生成に失敗しました!");
		}

		VkGraphicsPipelineCreateInfo pipeline_info{};
		pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_info.stageCount = 2;
		pipeline_info.pStages = shader_stages;
		pipeline_info.pVertexInputState = &vertex_input_info;
		pipeline_info.pInputAssemblyState = &input_assembly;
		pipeline_info.pViewportState = &viewport_state;
		pipeline_info.pRasterizationState = &rasterizer;
		pipeline_info.pMultisampleState = &multisampling;
		pipeline_info.pDepthStencilState = nullptr;
		pipeline_info.pColorBlendState = &color_blending;
		pipeline_info.pDynamicState = &dynamic_state;
		pipeline_info.layout = pipeline_layout_;
		pipeline_info.renderPass = render_pass_;
		pipeline_info.subpass = 0;
		// 既存のパイプラインから派生させて新しいグラフィックスパイプラインを作る際に使われる。
		pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
		pipeline_info.basePipelineIndex = -1;

		if (vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &graphics_pipeline_) != VK_SUCCESS) {
			throw std::runtime_error("グラフィックスパイプラインの生成に失敗しました!");
		}

		vkDestroyShaderModule(device_, frag_shader_module, nullptr);
		vkDestroyShaderModule(device_, vert_shader_module, nullptr);
	}

	void VulkanApplication::CreateFramebuffers() {
		// ImageViewの数とフレームバッファの数は同じ
		swap_chain_frame_buffers_.resize(swap_chain_image_views_.size());
		// 各ImageViewに対応するフレームバッファを作成する。
		for (size_t i = 0; i < swap_chain_image_views_.size(); i++) {
			VkImageView attachments[] = {swap_chain_image_views_[i]};
			VkFramebufferCreateInfo frame_buffer_info{};
			frame_buffer_info.renderPass = render_pass_;
			frame_buffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			frame_buffer_info.attachmentCount = 1;
			frame_buffer_info.pAttachments = attachments;
			frame_buffer_info.width = swap_chain_extent_.width;
			frame_buffer_info.height = swap_chain_extent_.height;
			frame_buffer_info.layers = 1;

			if (vkCreateFramebuffer(device_, &frame_buffer_info, nullptr, &swap_chain_frame_buffers_[i]) != VK_SUCCESS) {
				throw std::runtime_error("フレームバッファの生成に失敗しました！");
			}
		}
	}

	void VulkanApplication::CreateCommandPool() {
		QueueFamilyIndices queue_family_indices = FindQueueFamilies(physical_device_);

		VkCommandPoolCreateInfo pool_info{};
		pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		pool_info.queueFamilyIndex = queue_family_indices.graphics_family_.value();

		if (vkCreateCommandPool(device_, &pool_info, nullptr, &command_pool_) != VK_SUCCESS) {
			throw std::runtime_error("コマンドプールの生成に失敗しました！");
		}
	}
}