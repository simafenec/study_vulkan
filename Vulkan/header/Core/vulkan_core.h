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
		// ��ʂ̉���(px)
		const uint32_t WIDTH = 800;
		// ��ʂ̏c��(px)
		const uint32_t HEIGHT = 600;

	private:
		// GLFW�̃E�B���h�E�n���h���ւ̃|�C���^
		GLFWwindow* window;
	public:
		/**
		* @fn
		* Vulkan�̏��������s�����̂��A���C�����[�v���N������B
		* @brief Vulkan�̕`�揈�����N������B
		*/
		void run();
	private:
		/**
		* GLFW�̃E�B���h�E�ݒ������������
		*/
		void initWindow();
		/**
		* @fn
		* @brief Vulkan�̊e��R���|�[�l���g������������B
		*/
		void initVulkan();
		/**
		* @fn
		* @brief Vulkan�̕`�惋�[�v���N������B
		*/
		void mainLoop();
		/**
		* @fn
		* @brief Vulkan�Ŋm�ۂ����e�탊�\�[�X���������B
		*/
		void cleanUp();
	};
}