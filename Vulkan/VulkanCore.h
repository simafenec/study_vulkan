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
		// 画面の横幅(px)
		const uint32_t WIDTH = 800;
		// 画面の縦幅(px)
		const uint32_t HEIGHT = 600;

	private:
		// GLFWのウィンドウハンドラへのポインタ
		GLFWwindow* window;
	public:
		/**
		* @fn
		* Vulkanの初期化を行ったのち、メインループを起動する。
		* @brief Vulkanの描画処理を起動する。
		*/
		void run();
	private:
		/**
		* GLFWのウィンドウ設定を初期化する
		*/
		void initWindow();
		/**
		* @fn
		* @brief Vulkanの各種コンポーネントを初期化する。
		*/
		void initVulkan();
		/**
		* @fn
		* @brief Vulkanの描画ループを起動する。
		*/
		void mainLoop();
		/**
		* @fn
		* @brief Vulkanで確保した各種リソースを解放する。
		*/
		void cleanUp();
	};
}