/**
* @file vulkan_core.cpp
* @brief Vulkanの描画に関する基本的な処理を実装するクラスの定義ファイル
* @author はっとり
* @date 2025/4/17
*/

#include "Core/vulkan_core.h"

#include <glm/gtc/matrix_transform.hpp>

#include<stdexcept>
#include<set>
#include<cstdint>
#include<limits>
#include<algorithm>
#include <chrono>
#include <unordered_map>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

namespace Core {
	void VulkanApplication::InitWindow() {
		glfwInit();
		// GLFWはOpenGLのcontextを作るために設計されているため、まずそれを制御する必要がある。
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		// ウィンドウのリサイズは特別な対応が必要になるためいったんできないようにする
		// glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		// 四つ目のパラメータは画面を開くモニターを指定でき、五つ目のパラメータはOpenGL以外では必要ない
		window_ = glfwCreateWindow(kWidth, kHeight, "Vulkan", nullptr, nullptr);
		glfwSetWindowUserPointer(window_, this);
		glfwSetFramebufferSizeCallback(window_, FramebufferReizeCallback);
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
		CreateDescriptorSetLayout();
		CreateGraphicsPipeline();
		CreateCommandPool();
		CreateDepthResources();
		CreateFramebuffers();
		CreateTextureImage();
		CreateTextureImageView();
		CreateTextureSampler();
		LoadModel();
		CreateVertexBuffer();
		CreateIndexBuffer();
		CreateUniformBuffers();
		CreateDescriptorPool();
		CreateDescriptorSets();
		CreateCommandBuffers();
		CreateSyncObjects();
	}
	void VulkanApplication::MainLoop() {
		// エラーが出るまではウィンドウに対するイベントを観察し続ける
		while (!glfwWindowShouldClose(window_)) {
			glfwPollEvents();
			DrawFrame();
		}
		// メインループが終わったとたんに各種オブジェクトが破棄されると処理中のものが残っているときに困るので
		// 論理デバイスがIdleになるまで待機する。
		vkDeviceWaitIdle(device_);
	}

	void VulkanApplication::CleanUp() {
		vkDestroyCommandPool(device_, command_pool_, nullptr);
		vkDestroyCommandPool(device_, transfer_command_pool_, nullptr);
		// インスタンスは最後に破棄すること
		for (int index = 0; index < kMaxFramesInFlight; index++) {
			vkDestroySemaphore(device_, image_available_semaphores_[index], nullptr);
			vkDestroySemaphore(device_, render_finished_semaphores_[index], nullptr);
			vkDestroyFence(device_, in_flight_fences_[index], nullptr);
		}
		CleanUpSwapChainDependents();
		vkDestroySampler(device_, texture_sampler_, nullptr);
		vkDestroyImageView(device_, texture_image_view_, nullptr);
		vkDestroyImage(device_, texture_image_, nullptr);
		vkFreeMemory(device_, image_device_memory_, nullptr);
		vkDestroySwapchainKHR(device_, swap_chain_, nullptr);
		for (size_t i = 0; i < kMaxFramesInFlight; i++)
		{
			vkDestroyBuffer(device_, uniform_buffers_[i], nullptr);
			vkFreeMemory(device_, uniform_buffers_memory_[i], nullptr);
		}
		// descriptor set は pool が破棄された瞬間に付随して解放される
		vkDestroyDescriptorPool(device_, descriptor_pool_, nullptr);
		vkDestroyDescriptorSetLayout(device_, descriptor_set_layout_, nullptr);
		vkDestroyBuffer(device_, vertex_buffer_, nullptr);
		vkFreeMemory(device_, vertex_buffer_memory_, nullptr);
		vkDestroyBuffer(device_, index_buffer_, nullptr);
		vkFreeMemory(device_, index_buffer_memory_, nullptr);
		vkDestroyDevice(device_, nullptr);

		if (kEnableValidationLayers) {
			DestroyDebugUtilsMessenger(nullptr);
		}

		vkDestroySurfaceKHR(instance_, surface_, nullptr);
		vkDestroyInstance(instance_, nullptr);
		glfwDestroyWindow(window_);
		glfwTerminate();
	}

	void VulkanApplication::CleanUpSwapChainDependents() {
		for (auto framebuffer : swap_chain_frame_buffers_) {
			vkDestroyFramebuffer(device_, framebuffer, nullptr);
		}
		for (auto image_view : swap_chain_image_views_) {
			vkDestroyImageView(device_, image_view, nullptr);
		}
		vkDestroyImageView(device_, depth_image_view_, nullptr);
		vkDestroyImage(device_, depth_image_, nullptr);
		vkFreeMemory(device_, depth_image_device_memory_, nullptr);
		vkDestroyPipeline(device_, graphics_pipeline_, nullptr);
		vkDestroyPipelineLayout(device_, pipeline_layout_, nullptr);
		vkDestroyRenderPass(device_, render_pass_, nullptr);
	}

	void VulkanApplication::Run() {
		InitWindow();
		InitVulkan();
		MainLoop();
		CleanUp();
	}

	void VulkanApplication::DrawFrame() {
		// 一つ前のフレームを待機
		vkWaitForFences(device_, 1, &in_flight_fences_[current_frame_], VK_TRUE, UINT64_MAX);
		uint32_t image_index;
		VkResult result = vkAcquireNextImageKHR(device_, swap_chain_, UINT64_MAX, image_available_semaphores_[current_frame_], VK_NULL_HANDLE, &image_index);
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			RecreateSwapChain();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("スワップチェーンイメージの取得に失敗しました！");
		}
		// フェンスを明示的にリセット
		vkResetFences(device_, 1, &in_flight_fences_[current_frame_]);
		
		// コマンドバッファに描画コマンドを記録
		vkResetCommandBuffer(command_buffers_[current_frame_], 0);
		RecordCommandBuffer(command_buffers_[current_frame_], image_index);
		// ユニフォームバッファを更新
		UpdateUniformBuffers(current_frame_);
		// コマンドの提出タイミングなどを設定
		VkSubmitInfo submit_info{};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore wait_semaphores[] = { image_available_semaphores_[current_frame_]};
		VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		submit_info.waitSemaphoreCount = 1;
		submit_info.pWaitSemaphores = wait_semaphores;
		submit_info.pWaitDstStageMask = wait_stages;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &command_buffers_[current_frame_];
		VkSemaphore signal_semaphores[] = { render_finished_semaphores_[current_frame_]};
		submit_info.signalSemaphoreCount = 1;
		submit_info.pSignalSemaphores = signal_semaphores;
		if (vkQueueSubmit(graphics_queue_, 1, &submit_info, in_flight_fences_[current_frame_] ) != VK_SUCCESS) {
			throw std::runtime_error("描画コマンドの発行に失敗しました！");
		}
		VkPresentInfoKHR present_info{};
		present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present_info.waitSemaphoreCount = 1;
		present_info.pWaitSemaphores = signal_semaphores;
		VkSwapchainKHR swap_chains[] = { swap_chain_ };
		present_info.swapchainCount = 1;
		present_info.pSwapchains = swap_chains;
		present_info.pImageIndices = &image_index;
		present_info.pResults = nullptr;

		result = vkQueuePresentKHR(present_queue_, &present_info);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebuffer_resized_) {
			framebuffer_resized_ = false;
			RecreateSwapChain();
		}
		else if (result != VK_SUCCESS) {
			throw std::runtime_error("スワップチェーンイメージの取得に失敗しました！");
		}

		// モジュロにすることで常に利用可能な範囲のインデックスを反復するようになる
		current_frame_ = (current_frame_ + 1) % kMaxFramesInFlight;
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
		VkPhysicalDeviceFeatures features;
		vkGetPhysicalDeviceFeatures(device, &features);
		return indices.IsComplete() && extension_supported && swap_chain_adequate && features.samplerAnisotropy;
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
		device_features.samplerAnisotropy = VK_TRUE;
		
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
		create_info.oldSwapchain = old_swap_chain_;

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
			swap_chain_image_views_[index] = CreateImageView(swap_chain_images_[index], swap_chain_image_format_, VK_IMAGE_ASPECT_COLOR_BIT, mip_levels_);
		}
	}

	VkImageView VulkanApplication::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipMapLevels)
	{
		VkImageViewCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		create_info.image = image;
		create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		create_info.format = format;
		create_info.subresourceRange.aspectMask = aspectFlags;
		create_info.subresourceRange.baseMipLevel = 0;
		create_info.subresourceRange.levelCount = mipMapLevels;
		create_info.subresourceRange.baseArrayLayer = 0;
		create_info.subresourceRange.layerCount = 1;

		VkImageView view;
		if (vkCreateImageView(device_, &create_info, nullptr, &view) != VK_SUCCESS) {
			throw std::runtime_error("イメージビューの生成に失敗しました！");
		}
		return view;
	}

	void VulkanApplication::CreateDescriptorSetLayout()
	{
		// UBO 向けのlayoutを設定する
		VkDescriptorSetLayoutBinding uboBinding{};
		uboBinding.binding = 0;											// 今回は0番
		uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboBinding.descriptorCount = 1;
		uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;				// 今は頂点シェーダーだけからアクセスされるものとする
		uboBinding.pImmutableSamplers = nullptr;						// 画像のdescriptorに関連するものなので無視

		// 次にテクスチャサンプラー用のレイアウトを設定する
		VkDescriptorSetLayoutBinding samplerBinding{};
		samplerBinding.binding = 1;
		samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerBinding.descriptorCount = 1;
		samplerBinding.pImmutableSamplers = nullptr;
		samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;		// テクスチャ等の色情報はフラグメントシェーダーで扱う

		std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboBinding, samplerBinding };
		// set layoutを生成
		VkDescriptorSetLayoutCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		createInfo.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(device_, &createInfo, nullptr, &descriptor_set_layout_) != VK_SUCCESS)
		{
			throw std::runtime_error("Descriptor Set Layoutの生成に失敗しました！");
		}
	}

	void VulkanApplication::CreateDescriptorPool()
	{
		// まずはプールの大きさを定める
		std::array<VkDescriptorPoolSize, 2> poolSizes{};
		// 今回はUniform bufferのプールを作成する
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(kMaxFramesInFlight);
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = static_cast<uint32_t>(kMaxFramesInFlight);
		// 続いてプールの作成情報をいつも通り生成
		VkDescriptorPoolCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		createInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		createInfo.pPoolSizes = poolSizes.data();
		// 最大いくつのセットが割り当てられるのかは予め伝えておかないといけない
		createInfo.maxSets = static_cast<uint32_t>(kMaxFramesInFlight);
		if (vkCreateDescriptorPool(device_, &createInfo, nullptr, &descriptor_pool_) != VK_SUCCESS)
		{
			throw std::runtime_error("Descriptor Pool の生成に失敗しました！");
		}
	}

	void VulkanApplication::CreateDescriptorSets()
	{
		// ここでは同じレイアウトで各フレームのDescriptor Set を生成する
		// 全て同じであっても Allocateの際に layouts の長さとdescriptorSetCountが一致していることが求められるためコピーを作る必要がある
		std::vector<VkDescriptorSetLayout> layouts(kMaxFramesInFlight, descriptor_set_layout_);
		// どのようにSetをプールからAllocateするかを定める
		VkDescriptorSetAllocateInfo allocateInfo{};
		allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocateInfo.descriptorPool = descriptor_pool_;
		allocateInfo.descriptorSetCount = static_cast<uint32_t>(kMaxFramesInFlight);
		allocateInfo.pSetLayouts = layouts.data();

		// ベクトルをリサイズしてアロケーション
		descriptor_sets_.resize(kMaxFramesInFlight);
		if (vkAllocateDescriptorSets(device_, &allocateInfo, descriptor_sets_.data()) != VK_SUCCESS)
		{
			throw std::runtime_error("Descriptor set のアロケーションに失敗しました!");
		}
		// 割り当てられた各Descriptor Set に対して設定を行う
		for (size_t index = 0; index < kMaxFramesInFlight; index++)
		{
			VkDescriptorBufferInfo bufferInfo{};
			// descriptor set に割り当てるバッファ
			bufferInfo.buffer = uniform_buffers_[index];
			bufferInfo.offset = 0;
			// setに割り当てられるバッファのデータサイズ
			bufferInfo.range = sizeof(UniformBufferObject);
			// image sampler の為のディスクリプタ設定
			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = texture_image_view_;
			imageInfo.sampler = texture_sampler_;
			// Descriptor Set に書き込む情報をまとめる VkWriteDescriptorSet に情報を格納する
			std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = descriptor_sets_[index];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &bufferInfo;

			// image samplerのdescriptorに書き込む情報の設定
			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = descriptor_sets_[index];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pImageInfo = &imageInfo;
			// 設定したデータでdescriptor set の設定を更新
			vkUpdateDescriptorSets(device_, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
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

		// 深度テクスチャのアタッチメント設定
		VkAttachmentDescription depth_attachment{};
		depth_attachment.format = FindDepthFormat();
		depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;				// どうせ描画が完了したら書き換わるので保存は気にしない
		depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depth_attachment_ref{};
		depth_attachment_ref.attachment = 1;
		depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &color_attachment_reference;
		subpass.pDepthStencilAttachment = &depth_attachment_ref;

		// サブパス依存性設定
		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		// 今はサブパスが一つしかないので0番目のインデックスを指定
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		

		std::array<VkAttachmentDescription, 2 > attachments = { color_attachment, depth_attachment };

		VkRenderPassCreateInfo render_pass_info{};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
		render_pass_info.pAttachments = attachments.data();
		render_pass_info.subpassCount = 1;
		render_pass_info.pSubpasses = &subpass;
		render_pass_info.dependencyCount = 1;
		render_pass_info.pDependencies = &dependency;

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
		auto binding_description = Vertex::GetBindingDescription();
		auto attribute_description = Vertex::GetAttributeDescriptions();

		VkPipelineVertexInputStateCreateInfo vertex_input_info{};
		vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertex_input_info.vertexBindingDescriptionCount = 1;
		vertex_input_info.pVertexBindingDescriptions = &binding_description;
		vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_description.size());
		vertex_input_info.pVertexAttributeDescriptions = attribute_description.data();

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
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
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

		// depth buffer をパイプライン上に載せるための設定
		VkPipelineDepthStencilStateCreateInfo depth_stencil{};
		depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depth_stencil.depthTestEnable = VK_TRUE;
		depth_stencil.depthWriteEnable = VK_TRUE;
		// depthが小さい = カメラに近い　なので less で比較
		depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
		// depth boundsテスト
		depth_stencil.depthBoundsTestEnable = VK_FALSE;
		depth_stencil.minDepthBounds = 0.0f;
		depth_stencil.maxDepthBounds = 1.0f;
		// stencil テスト用設定　今は使わない
		depth_stencil.stencilTestEnable = VK_FALSE;
		depth_stencil.front = {};
		depth_stencil.back = {};

		VkPipelineLayoutCreateInfo pipeline_layout_info{};
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.setLayoutCount = 1;
		pipeline_layout_info.pSetLayouts = &descriptor_set_layout_;
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
		pipeline_info.pDepthStencilState = &depth_stencil;
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
			std::array<VkImageView,2> attachments = {swap_chain_image_views_[i], depth_image_view_};
			VkFramebufferCreateInfo frame_buffer_info{};
			frame_buffer_info.renderPass = render_pass_;
			frame_buffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			frame_buffer_info.attachmentCount = static_cast<uint32_t>(attachments.size());
			frame_buffer_info.pAttachments = attachments.data();
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

		// 描画用のコマンドプールを作成
		VkCommandPoolCreateInfo pool_info{};
		pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		pool_info.queueFamilyIndex = queue_family_indices.graphics_family_.value();

		if (vkCreateCommandPool(device_, &pool_info, nullptr, &command_pool_) != VK_SUCCESS) {
			throw std::runtime_error("コマンドプールの生成に失敗しました！");
		}
		// 転送などの短命な要素を扱うためのコマンドプールを作成
		pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		if (vkCreateCommandPool(device_, &pool_info, nullptr, &transfer_command_pool_) != VK_SUCCESS) {
			throw std::runtime_error("転送コマンドプールの生成に失敗しました！");
		}
	}
	void VulkanApplication::CreateCommandBuffers() {
		command_buffers_.resize(kMaxFramesInFlight);
		VkCommandBufferAllocateInfo allocate_info{};
		allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocate_info.commandPool = command_pool_;
		allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocate_info.commandBufferCount = static_cast<uint32_t>(command_buffers_.size());

		if (vkAllocateCommandBuffers(device_, &allocate_info, command_buffers_.data()) != VK_SUCCESS) {
			throw std::runtime_error("コマンドバッファの割り当てに失敗しました！");
		}
	}

	void VulkanApplication::RecordCommandBuffer(
		VkCommandBuffer command_buffer,
		uint32_t image_index
	) {
		// コマンドの記録を開始する
		VkCommandBufferBeginInfo begin_info{};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = 0;
		begin_info.pInheritanceInfo = nullptr;

		if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS) {
			throw std::runtime_error("コマンドの記録開始に失敗しました！");
		}

		// レンダーパスを開始する。
		VkRenderPassBeginInfo render_pass_info{};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_info.renderPass = render_pass_;
		render_pass_info.framebuffer = swap_chain_frame_buffers_[image_index];
		render_pass_info.renderArea.offset = { 0, 0 };
		render_pass_info.renderArea.extent = swap_chain_extent_;
		std::array<VkClearValue, 2> clear_values{};
		clear_values[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
		// depth, stencil で指定
		// depth buffer における depth の範囲は 0.0 - 1.0 (0.0 -> near plane / 1.0 -> far plane)
		clear_values[1].depthStencil = { 1.0f, 0 };
		render_pass_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
		render_pass_info.pClearValues = clear_values.data();
		vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

		// 描画に使うグラフィックスパイプラインを指定する。
		vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline_);

		VkBuffer vertexBuffers[] = { vertex_buffer_ };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(command_buffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(command_buffer, index_buffer_, 0, VK_INDEX_TYPE_UINT32);	// 元のvectorがuint16_tなのでそれに合わせる
		// ビューポートとシザー矩形を設定する
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(swap_chain_extent_.width);
		viewport.height = static_cast<float>(swap_chain_extent_.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(command_buffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = swap_chain_extent_;
		vkCmdSetScissor(command_buffer, 0, 1, &scissor);

		vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout_, 0, 1, &descriptor_sets_[current_frame_], 0, nullptr);
		// 描画コマンドを発行する。
		vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(indices_.size()), 1, 0, 0, 0);

		// レンダーパスを閉じる
		vkCmdEndRenderPass(command_buffer);
		// コマンドバッファの記録を終了する。
		// コマンドにエラーがあった場合はここで対応する。
		if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
			throw std::runtime_error("コマンドバッファへのコマンドの書き込みに失敗しました！");
		}
	}

	void VulkanApplication::CreateSyncObjects() {
		image_available_semaphores_.resize(kMaxFramesInFlight);
		render_finished_semaphores_.resize(kMaxFramesInFlight);
		in_flight_fences_.resize(kMaxFramesInFlight);

		VkSemaphoreCreateInfo semaphore_info{};
		semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fence_info{};
		fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		// 最初のフレームは即座にシグナルを送ってほしいのでシグナルがある状態で生成する
		fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		
		// 個数分作成する。
		for (int index = 0; index < kMaxFramesInFlight; index++) {
			if (vkCreateSemaphore(device_, &semaphore_info, nullptr, &image_available_semaphores_[index]) != VK_SUCCESS ||
				vkCreateSemaphore(device_, &semaphore_info, nullptr, &render_finished_semaphores_[index]) != VK_SUCCESS ||
				vkCreateFence(device_, &fence_info, nullptr, &in_flight_fences_[index]) != VK_SUCCESS) {
				throw std::runtime_error("同期オブジェクトの生成に失敗しました！");
			}
		}
	}
	void VulkanApplication::CreateDepthResources()
	{
		VkFormat depthFormat = FindDepthFormat();
		CreateImage(
			swap_chain_extent_.width,
			swap_chain_extent_.height,
			mip_levels_,
			depthFormat,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			depth_image_,
			depth_image_device_memory_
		);
		depth_image_view_ = CreateImageView(depth_image_, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, mip_levels_);

		TransitionImageLayout(depth_image_, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, mip_levels_);

	}
	VkFormat VulkanApplication::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
	{
		for (VkFormat format : candidates)
		{
			VkFormatProperties props;
			// フォーマットの情報取得
			vkGetPhysicalDeviceFormatProperties(physical_device_, format, &props);
			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
			{
				return format;
			}
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
			{
				return format;
			}
		}
		throw std::runtime_error("引数に与えられた条件を満たすフォーマットが見つかりませんでした");
	}
	VkFormat VulkanApplication::FindDepthFormat()
	{
		return FindSupportedFormat(
			{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}
	void VulkanApplication::CreateTextureImage()
	{
		// まずはテクスチャデータを読み込む
		int textureWidth, textureHeight, textureChannels;
		stbi_uc* pixelData = stbi_load(kModelTexturePath.c_str(), &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);
		VkDeviceSize imageSize = textureWidth * textureHeight * 4;		// ピクセル数 x 4バイト (rgba)
		if (!pixelData)
		{
			throw std::runtime_error("テクスチャの読み込みに失敗しました！");
		}
		// ミップマップの数を決定
		// オリジナルの画像 + 2のn乗のn枚分がミップマップにできる (2^0(オリジナル), 2^1(半分) + ... )
		mip_levels_ = static_cast<uint32_t>(std::floor(std::log2(std::max(textureWidth, textureHeight)))) + 1; 
		// ステージングバッファを作って画像データを転送
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
		void* data;
		vkMapMemory(device_, stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, pixelData, static_cast<size_t>(imageSize));
		vkUnmapMemory(device_, stagingBufferMemory);
		// 転送したら画像データも解放
		stbi_image_free(pixelData);
		// バッファに転送
		CreateImage(textureWidth, textureHeight, mip_levels_, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture_image_, image_device_memory_);
		// 画像レイアウトをTRANSFER用に変更する
		TransitionImageLayout(texture_image_, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mip_levels_);
		// バッファからVkImageにデータをコピーする
		CopyBufferToImage(stagingBuffer, texture_image_, static_cast<uint32_t>(textureWidth), static_cast<uint32_t>(textureHeight));
		// ミップマップを作成　シェーダーがアクセスできる状態への変化は関数内で行う
		GenerateMipmaps(texture_image_, VK_FORMAT_R8G8B8A8_SRGB, textureWidth, textureHeight, mip_levels_);
		//ステージングバッファを破棄する
		vkDestroyBuffer(device_, stagingBuffer, nullptr);
		vkFreeMemory(device_, stagingBufferMemory, nullptr);
	}
	void VulkanApplication::CreateImage(uint32_t textureWidth, uint32_t textureHeight, uint32_t mipMapLevels, VkFormat imageFormat, VkImageTiling imageTiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& deviceMemory)
	{
		// まずはVkImageを生成するために必要な情報を埋めていく
		VkImageCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		createInfo.imageType = VK_IMAGE_TYPE_2D;
		createInfo.extent.width = textureWidth;
		createInfo.extent.height = textureHeight;
		createInfo.extent.depth = 1;
		createInfo.mipLevels = mipMapLevels;
		createInfo.arrayLayers = 1;
		createInfo.format = imageFormat;
		createInfo.tiling = imageTiling;
		createInfo.usage = usage;
		createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		// imageを生成
		if (vkCreateImage(device_, &createInfo, nullptr, &image) != VK_SUCCESS)
		{
			throw std::runtime_error("Imageバッファの作成に失敗しました！");
		}
		// 画像に対応するメモリのRequirementを取得し、それに沿って画像用のメモリをアロケーションする
		VkMemoryRequirements imageMemoryRequirements;
		vkGetImageMemoryRequirements(device_, image, &imageMemoryRequirements);

		VkMemoryAllocateInfo allocateInfo{};
		allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocateInfo.allocationSize = imageMemoryRequirements.size;
		allocateInfo.memoryTypeIndex = FindMemoryType(imageMemoryRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(device_, &allocateInfo, nullptr, &deviceMemory) != VK_SUCCESS)
		{
			throw std::runtime_error("画像用のメモリの確保に失敗しました！");
		}
		vkBindImageMemory(device_, image, deviceMemory, 0);

	}
	void VulkanApplication::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipMapLevels)
	{
		// コマンドバッファに実行する処理を記録する
		VkCommandBuffer commandBuffer = BeginSingleTimeCommands();
		// レイアウトを明示的に変更するためのバリアを用意する。
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		// もしキューファミリーの所持権を移動させるバリアを張るときはちゃんとインデックスを設定する必要があるが、ここでは関係ないので無視
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		// バリアを張るに当たり影響のある画像の情報及び画像の部分をバリアにも伝えておく
		barrier.image = image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = mipMapLevels;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		// どの種類の操作がバリアの前に実行される必要があるのか / どの種類の操作がバリアを待機する必要があるのかを指定する

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;
		// もし前のレイアウトが定義されていない、もしくは次のレイアウトが転送先の画像になるように変更する場合
		// 実行しなければいけない命令が発生するパイプラインはないが、Transferを実行する操作はレイアウト移行後に実行してもらう必要がある
		if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			if (HasStencilComponent(format))
			{
				barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
			}
		}
		else
		{
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED || newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		// 前のレイアウトが転送先の画像、もしくは次のレイアウトがシェーダーからの読み込みだった場合
		// 先に転送を終わらせてもらう必要があり、シェーダーからの読み取りは待ってもらう必要がある
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL || newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		else
		{
			throw std::invalid_argument("未対応のレイアウト遷移が指定されました");
		}

		vkCmdPipelineBarrier(
			commandBuffer,
			sourceStage,		// バリアの前に実行されるべき操作が発生するパイプラインステージを指定する
			destinationStage,	// バリアを待つ操作が発生するパイプラインステージを指定する
			0,					// バリアの条件 BY_REGION_BITを指定するとリージョン単位の条件に変わり、リソースの一部がすでに書き込まれている場合、その部分の読み込みをすぐに開始してもよくなる
			0, nullptr,			// メモリバリアの配列
			0, nullptr,			// バッファメモリバリアの配列
			1, &barrier			// イメージメモリバリアの配列
		);
		EndSingleTimeCommands(commandBuffer);
	}
	void VulkanApplication::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
	{
		VkCommandBuffer commandBuffer = BeginSingleTimeCommands();
		// コピーのために情報を詰める
		VkBufferImageCopy region{};
		// バッファのどこから画像データが始まるかを表す
		region.bufferOffset = 0;
		// 画像のパディング設定
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { width, height, 1 };

		vkCmdCopyBufferToImage(
			commandBuffer,
			buffer,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&region
		);
		EndSingleTimeCommands(commandBuffer);
	}
	void VulkanApplication::CreateTextureImageView()
	{
		texture_image_view_ = CreateImageView(texture_image_, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, mip_levels_);
	}
	void VulkanApplication::CreateTextureSampler()
	{
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;					// サンプリングする際にテクセルをどのように拡大するか ここでは線形的に拡大
		samplerInfo.minFilter = VK_FILTER_LINEAR;					// サンプリングする際にテクセルをどのように縮小するか ここでは線形的に縮小
		// 画像の外の各軸ごとのアドレスモードを指定する
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		// 異方性サンプリングの設定
		samplerInfo.anisotropyEnable = VK_TRUE;
		VkPhysicalDeviceProperties prop{};
		vkGetPhysicalDeviceProperties(physical_device_, &prop);
		samplerInfo.maxAnisotropy = prop.limits.maxSamplerAnisotropy;
		samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = VK_LOD_CLAMP_NONE;

		if(vkCreateSampler(device_, &samplerInfo, nullptr, &texture_sampler_) != VK_SUCCESS)
		{
			throw std::runtime_error("テクスチャサンプラーの生成に失敗しました");
		}
	}
	void VulkanApplication::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& buffer_memory)
	{
		VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = size;
		bufferCreateInfo.usage = usage;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		if (vkCreateBuffer(device_, &bufferCreateInfo, nullptr, &buffer) != VK_SUCCESS)
		{
			throw std::runtime_error("頂点バッファの作成に失敗しました");
		}
		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device_, buffer, &memRequirements);

		VkMemoryAllocateInfo allocateInfo{};
		allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocateInfo.allocationSize = memRequirements.size;
		allocateInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);
		if (vkAllocateMemory(device_, &allocateInfo, nullptr, &buffer_memory) != VK_SUCCESS) {
			throw std::runtime_error("VertexBuffer用のメモリ確保に失敗しました");
		}
		// 四番目の引数はメモリ領域内のオフセット。複数の用途で使う場合はalignmentで割り切れる値でオフセットを設定する
		vkBindBufferMemory(device_, buffer, buffer_memory, 0);
	}

	VkCommandBuffer VulkanApplication::BeginSingleTimeCommands()
	{
		// 転送用のコマンドバッファを確保
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = transfer_command_pool_;
		allocInfo.commandBufferCount = 1;
		// 転送時にしか使わないのでローカル変数
		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(device_, &allocInfo, &commandBuffer);

		// すぐにコマンドの記録を始める
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(commandBuffer, &beginInfo);
		return commandBuffer;
	}

	void VulkanApplication::EndSingleTimeCommands(VkCommandBuffer commandBuffer)
	{
		// 記録終了
		vkEndCommandBuffer(commandBuffer);
		// 記録したコマンドをキューに送ってコピーを実行
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;
		vkQueueSubmit(graphics_queue_, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphics_queue_);
		// ここにフェンスを入れてもよい
		// コマンドバッファを開放
		vkFreeCommandBuffers(device_, transfer_command_pool_, 1, &commandBuffer);
	}

	void VulkanApplication::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
	{
		VkCommandBuffer commandBuffer = BeginSingleTimeCommands();
		// コピーコマンドを記録
		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0; // srcBuffer内のコピー元オフセット
		copyRegion.dstOffset = 0; // dstBuffer内のコピー先オフセット
		copyRegion.size = size;  // コピーするバイト数
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
		EndSingleTimeCommands(commandBuffer);

	}

	void VulkanApplication::LoadModel()
	{
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string err;
		std::string warn;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, kModelPath.c_str()))
		{
			throw std::runtime_error(err);
		}
		// 頂点の重複をなくすために頂点ごとに一意なインデックスを振って管理する
		// Vertexのハッシュ値計算を実装しないとこれは実行できない (unordered_mapのキーはハッシュ値管理のため)
		std::unordered_map<Vertex, uint32_t> unique_vertices{};
		for (const auto& shape : shapes)
		{
			for (const auto& index : shape.mesh.indices)
			{
				Vertex vertex{};
				vertex.pos = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2],
				};
				vertex.texCoord = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					// Vulkanはテクスチャの上端の座標を 0 としているが、objフォーマットは画像の下端を0としているため、0-1の間で反転させる必要がある。
					1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
				};
				vertex.color = { 1.0f, 1.0f, 1.0f };
				// Vertex構造体の等価比較関数がないと動かない
				if (unique_vertices.count(vertex) == 0)
				{
					unique_vertices[vertex] = static_cast<uint32_t>(vertices_.size());
					vertices_.push_back(vertex);
				}
				indices_.push_back(unique_vertices[vertex]);
			}
		}

	}

	void VulkanApplication::CreateVertexBuffer() {
		VkDeviceSize bufferSize = sizeof(vertices_[0]) * vertices_.size();

		// Staging Bufferを用意
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,												// 転送元として使うことを指定
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,		// CPUからアクセス可能かつGPUとの同期が取られるバッファ
			stagingBuffer,
			stagingBufferMemory);
		//Staging Bufferにデータをコピー
		void* data;
		vkMapMemory(device_, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vertices_.data(), (size_t)bufferSize);
		vkUnmapMemory(device_, stagingBufferMemory);


		CreateBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,									 // GPUからのみアクセス可能なバッファとして生成する
			vertex_buffer_,
			vertex_buffer_memory_
		);

		// vertexbufferにはvkMapMemory出来ないのでStagingBufferを経由してコピーする
		CopyBuffer(stagingBuffer, vertex_buffer_, bufferSize);
		// Staging Bufferを破棄
		vkDestroyBuffer(device_, stagingBuffer, nullptr);
		vkFreeMemory(device_, stagingBufferMemory, nullptr);
	}

	void VulkanApplication::CreateIndexBuffer()
	{
		VkDeviceSize bufferSize = sizeof(indices_[0]) * indices_.size();

		// Staging Bufferを用意
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,												// 転送元として使うことを指定
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,		// CPUからアクセス可能かつGPUとの同期が取られるバッファ
			stagingBuffer,
			stagingBufferMemory);
		//Staging Bufferにデータをコピー
		void* data;
		vkMapMemory(device_, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices_.data(), (size_t)bufferSize);
		vkUnmapMemory(device_, stagingBufferMemory);


		CreateBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,									 // GPUからのみアクセス可能なバッファとして生成する
			index_buffer_,
			index_buffer_memory_
		);

		// indexbufferにはvkMapMemory出来ないのでStagingBufferを経由してコピーする
		CopyBuffer(stagingBuffer, index_buffer_, bufferSize);
		// Staging Bufferを破棄
		vkDestroyBuffer(device_, stagingBuffer, nullptr);
		vkFreeMemory(device_, stagingBufferMemory, nullptr);
	}

	void VulkanApplication::CreateUniformBuffers()
	{
		VkDeviceSize bufferSize = sizeof(UniformBufferObject);
		uniform_buffers_.resize(kMaxFramesInFlight);
		uniform_buffers_memory_.resize(kMaxFramesInFlight);
		uniform_buffers_mapped_.resize(kMaxFramesInFlight);

		for (size_t i = 0; i < kMaxFramesInFlight; i++)
		{
			CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniform_buffers_[i], uniform_buffers_memory_[i]);
			vkMapMemory(device_, uniform_buffers_memory_[i], 0, bufferSize, 0, &uniform_buffers_mapped_[i]);
		}
	}

	void VulkanApplication::UpdateUniformBuffers(uint32_t curretImageIndex)
	{
		static auto startTime = std::chrono::high_resolution_clock::now();

		// 時間で更新を入れる
		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
		UniformBufferObject ubo;
		ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));				// 行列をz軸中心に回す
		ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));	// (2, 2, 2)から (0, 0, 0) を見る
		ubo.projection = glm::perspective(glm::radians(45.0f), swap_chain_extent_.width / (float)swap_chain_extent_.height, 0.1f, 10.0f);
		// GLM は OpenGLに対して設計されているためY軸がVulkanの向きと真逆になっている。そのためprojection行列のY軸だけひっくり返す
		ubo.projection[1][1] *= -1;

		// 更新した行列をメモリにコピーする
		memcpy(uniform_buffers_mapped_[curretImageIndex], &ubo, sizeof(ubo));
	}

	uint32_t VulkanApplication::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physical_device_, &memProperties);
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties)) {
				return i;
			}
		}
		throw std::runtime_error("適切なメモリタイプを特定できませんでした");
	}

	// 実際は実行時にミップマップを生成することはしない…
	void VulkanApplication::GenerateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipmapLevels)
	{
		//ミップマップ生成に使う linear blitting がサポートされているかどうかを確認する
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(physical_device_, imageFormat, &formatProperties);
		if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
		{
			throw std::runtime_error("物理デバイスのテクスチャ画像フォーマットが linear blitting をサポートしていません");
		}

		VkCommandBuffer commandBuffer = BeginSingleTimeCommands();
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = image;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.levelCount = 1;

		int32_t mipWidth = texWidth;
		int32_t mipHeight = texHeight;
		for (uint32_t i = 1; i < mipmapLevels; i++)
		{
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);

			VkImageBlit blit{};
			blit.srcOffsets[0] = { 0, 0, 0 };
			blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.baseArrayLayer = 0;
			blit.srcSubresource.layerCount = 1;
			blit.srcSubresource.mipLevel = i - 1;

			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.baseArrayLayer = 0;
			blit.dstSubresource.layerCount = 1;
			blit.dstSubresource.mipLevel = i;
			// ミップマップ生成コマンドを記録
			vkCmdBlitImage(commandBuffer,
				image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &blit,
				VK_FILTER_LINEAR
			);
			// 生成したミップマップをシェーダーからアクセスできる状態にする
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(
				commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);

			if (mipWidth > 1) mipWidth /= 2;
			if (mipHeight > 1) mipHeight /= 2;
		}

		barrier.subresourceRange.baseMipLevel = mipmapLevels - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		EndSingleTimeCommands(commandBuffer);
	}

	void VulkanApplication::RecreateSwapChain() {
		// ウィンドウの最小化対応
		// ウィンドウが最小化されたときは再び展開されるまで待機する
		int width = 0, height = 0;
		glfwGetFramebufferSize(window_, &width, &height);
		while (width == 0 || height == 0) {
			glfwGetFramebufferSize(window_, &width, &height);
			glfwWaitEvents();
		}

		// 古いスワップチェーンを一時的に保持しておく
		old_swap_chain_ = swap_chain_;
		// 新しいスワップチェーンを作る。
		CreateSwapChain();
		// 新しいスワップチェーンを作ったので、古いスワップチェーンは破棄する
		if (old_swap_chain_ != VK_NULL_HANDLE) {
			vkDestroySwapchainKHR(device_, old_swap_chain_, nullptr);
		}
		vkDeviceWaitIdle(device_);
		// 古いスワップチェーンに依存しているオブジェクトを破棄する
		CleanUpSwapChainDependents();
		// 依存オブジェクトを再生成する
		// image view と depth resource はフレームバッファよりも前に作ろうね
		CreateImageViews();
		CreateDepthResources();
		CreateRenderPass();
		CreateGraphicsPipeline();
		CreateFramebuffers();
		CreateCommandBuffers();
	}
}