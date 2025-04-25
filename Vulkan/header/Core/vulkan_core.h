#pragma once
/**
* @file VulkanCore.h
* @brief Vulkanの描画に関する基本的な処理を実装するクラスの宣言ファイル
* @author はっとり
* @date 2025/4/17
*/

#include<vector>
#include<iostream>

// ウィンドウ生成に関する各種機能を宣言するヘッダー
#define GLFW_INCLUDE_VULKAN 1
// Vulkanの各種関数や構造体、列挙型を提供してくれるヘッダー
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

namespace Core
{
	class VulkanApplication {
	public:
		const uint32_t kWidth = 800;
		const uint32_t kHeight = 600;
	private:
		const std::vector<const char*> kValidationLayers = {
			"VK_LAYER_KHRONOS_validation"
		};
#ifdef NDEBUG
		const bool kEnableValidationLayers = false;
#else
		const bool kEnableValidationLayers = true;
#endif

	public:
		/**
		* @fn
		* @brief Vulkanの描画処理を起動する。
		*/
		void Run();

	private:
		/**
		* @brief GLFWのウィンドウ設定を初期化する
		*/
		void InitWindow();

		/**
		* @brief VkInstanceを生成する。
		*/
		void CreateVkInstance();
		/**
		* @fn
		* @brief Vulkanの各種コンポーネントを初期化する。
		*/
		void InitVulkan();
		/**
		* @fn
		* @brief Vulkanの描画ループを起動する。
		*/
		void MainLoop();
		/**
		* @fn
		* @brief Vulkanで確保した各種リソースを解放する。
		*/
		void CleanUp();

		/**
		* @fn
		* @brief
		* 要求されている拡張機能が全てサポートされているかをチェックする
		* @detail
		* VkInstance生成時にVkInstanceCreateInfo内で設定している必要な拡張機能がサポートされていない場合、
		* VK_ERROR_EXTENSION_NOT_PRESENTエラーコードが発生する。
		* これに未然に対応するために、vkEnumerateInstanceExtensionProperties関数でサポートされている拡張機能を取得し、
		* 必要な拡張機能がサポートされているかを事前チェックする。
		* 
		*/
		bool CheckExtensionsAvailable(std::vector<const char*> required_instance);

		/**
		* @fn
		* @brief 必要な拡張機能のリストを取得する。
		*/
		std::vector<const char*> GetRequiredExtensions();

		/**
		* @fn
		* @brief
		* バリデーションレイヤがサポートされているかどうかを確認する。
		*/
		bool CheckValidationLayerSupport();

		/**
		* @fn
		* @brief
		* デバッグメッセンジャーの生成情報を初期化する
		*/
		void InitializeDebugMessengerCreateInfo(
			VkDebugUtilsMessengerCreateInfoEXT& create_info				// デバッグメッセンジャー生成情報インスタンスへの参照
		);
		/**
		* @fn
		* @brief
		* デバッグメッセージの通知システムをセットアップする
		*/
		void SetupDebugMessenger();

		/**
		* @fn
		* @brief 
		* デバッグメッセンジャーを生成する
		*/
		VkResult CreateDebugUtilsMessenger(
			const VkDebugUtilsMessengerCreateInfoEXT* p_create_info,	// デバッグメッセンジャーの生成情報
			const VkAllocationCallbacks* p_allocator					// カスタムアロケーターへのポインタ
		);

		/**
		* @fn
		* @brief
		* デバッグメッセンジャーを破棄する
		*/
		void DestroyDebugUtilsMessenger(
			VkAllocationCallbacks* p_allocator
		);

		/**
		* @fn
		* @brief
		* デバッグメッセージのコールバック関数
		* @return
		* Validation Layer をトリガした関数の処理をabortすべきかどうか
		*/
		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallBack(
			VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,	 // デバッグメッセージの重要度
			VkDebugUtilsMessageTypeFlagsEXT message_type,				 // 発生したメッセージの種類
			const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data, // メッセージの詳細な内容を含む構造体
			void* p_user_data											 // コールバックのセットアップ時に与えられる独自データ
		) {
			// VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: 診断メッセージ
			// VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: リソースメッセージなどの情報提供メッセージ
			// VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT : エラーではないがバグに近い警告メッセージ
			// VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT : クラッシュを引き起こしうる不正な動きを通知するメッセージ
			// 下に行けば行くほど重要度大 (割り当てられた値も大きい)
			if (message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
				std::cerr << "validation layer : " << p_callback_data->pMessage << std::endl;
				
			}
			else {
				std::cout << "validation layer : " << p_callback_data->pMessage << std::endl;
			}
			return VK_FALSE;
		}
		/**
		* @fn
		* @brief
		* Vulkanの描画に使用する物理デバイスを選択する
		*/
		void PickPhysicalDevice();

		/**
		* @fn
		* @brief
		* 論理デバイスを作成する。
		*/
		void CreateLogicalDevice();

	private:
		GLFWwindow* window_;
		VkInstance instance_;
		VkDebugUtilsMessengerEXT debug_messenger_;
		VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
		VkDevice device_;
		VkQueue graphics_queue_;
	};
}