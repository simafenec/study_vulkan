#pragma once
/**
* @file VulkanCore.h
* @brief Vulkanの描画に関する基本的な処理を実装するクラスの宣言ファイル
* @author はっとり
* @date 2025/4/17
*/

// ウィンドウ生成に関する各種機能を宣言するヘッダー
#define GLFW_INCLUDE_VULKAN
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

	private:
		GLFWwindow* window_;
	};
}