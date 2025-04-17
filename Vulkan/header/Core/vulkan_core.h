#pragma once
/**
* @file VulkanCore.h
* @brief Vulkan�̕`��Ɋւ����{�I�ȏ�������������N���X�̐錾�t�@�C��
* @author �͂��Ƃ�
* @date 2025/4/17
*/

// �E�B���h�E�����Ɋւ���e��@�\��錾����w�b�_�[
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
		* @brief Vulkan�̕`�揈�����N������B
		*/
		void Run();

	private:
		/**
		* @brief GLFW�̃E�B���h�E�ݒ������������
		*/
		void InitWindow();
		/**
		* @fn
		* @brief Vulkan�̊e��R���|�[�l���g������������B
		*/
		void InitVulkan();
		/**
		* @fn
		* @brief Vulkan�̕`�惋�[�v���N������B
		*/
		void MainLoop();
		/**
		* @fn
		* @brief Vulkan�Ŋm�ۂ����e�탊�\�[�X���������B
		*/
		void CleanUp();

	private:
		GLFWwindow* window_;
	};
}