#pragma once
/**
* @file VulkanCore.h
* @brief Vulkanの描画に関する基本的な処理を実装するクラスの宣言ファイル
* @author はっとり
* @date 2025/4/17
*/

#include<vector>

// ウィンドウ生成に関する各種機能を宣言するヘッダー
#define GLFW_INCLUDE_VULKAN 1
// Vulkanの各種関数や構造体、列挙型を提供してくれるヘッダー
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

namespace Core
{
	class VulkanApplication {
	public:
		const uint32_t kWIDTH = 800;
		const uint32_t kHEIGHT = 600;

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
		* @brief Vulkan アプリケーションの基本情報を設定する
		*/
		void CreateVkApplicationInfo(VkApplicationInfo& info);

		/**
		* @fn
		* @brief VkInstanceの生成設定を行う
		* @detail デバイスにかかわらず全てのプログラムに適用する拡張(extensions)と使用するバリデーションレイヤの設定を行う。
		*/
		void CreateVkInstanceCreateInfo(VkInstanceCreateInfo& create_info, const VkApplicationInfo& application_info);
		
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

	private:
		GLFWwindow* window_;
		VkInstance instance_;
	};
}