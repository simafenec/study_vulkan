/**
* @file main.cpp
* @brief �G���g���|�C���g (main�֐�)���`����N���X�B
* @author �͂��Ƃ�
* @date 2025/4/17
*/

// �G���[��`�B���邽�߂Ɏg�p����w�b�_�[�B
#include <iostream>
#include <stdexcept>
//EXIT_SUCCESS �� EXIT_FAILURE �}�N�����g�����߂̃w�b�_�[�B
#include <cstdlib>

// Vulkan�̊�{�@�\��錾�����w�b�_�[�t�@�C��
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