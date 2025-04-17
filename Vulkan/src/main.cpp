/**
* @file main.cpp
* @brief エントリポイント (main関数)を定義するクラス。
* @author はっとり
* @date 2025/4/17
*/

// エラーを伝達するために使用するヘッダー。
#include <iostream>
#include <stdexcept>
//EXIT_SUCCESS と EXIT_FAILURE マクロを使うためのヘッダー。
#include <cstdlib>

// Vulkanの基本機能を宣言したヘッダーファイル
#include "Core/vulkan_core.h"

int main() {
	Core::VulkanApplication app;

	try {
		app.run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}