#pragma once
/**
* @file VulkanCore.h
* @brief Vulkan�̕`��Ɋւ����{�I�ȏ�������������N���X�̐錾�t�@�C��
* @author �͂��Ƃ�
* @date 2025/4/17
*/

#include<vector>

// �E�B���h�E�����Ɋւ���e��@�\��錾����w�b�_�[
#define GLFW_INCLUDE_VULKAN 1
// Vulkan�̊e��֐���\���́A�񋓌^��񋟂��Ă����w�b�_�[
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
		* @brief Vulkan�̕`�揈�����N������B
		*/
		void Run();

	private:
		/**
		* @brief GLFW�̃E�B���h�E�ݒ������������
		*/
		void InitWindow();

		/**
		* @brief VkInstance�𐶐�����B
		*/
		void CreateVkInstance();
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

		/**
		* @fn
		* @brief Vulkan �A�v���P�[�V�����̊�{����ݒ肷��
		*/
		void CreateVkApplicationInfo(VkApplicationInfo& info);

		/**
		* @fn
		* @brief VkInstance�̐����ݒ���s��
		* @detail �f�o�C�X�ɂ�����炸�S�Ẵv���O�����ɓK�p����g��(extensions)�Ǝg�p����o���f�[�V�������C���̐ݒ���s���B
		*/
		void CreateVkInstanceCreateInfo(VkInstanceCreateInfo& create_info, const VkApplicationInfo& application_info);
		
		/**
		* @fn
		* @brief
		* �v������Ă���g���@�\���S�ăT�|�[�g����Ă��邩���`�F�b�N����
		* @detail
		* VkInstance��������VkInstanceCreateInfo���Őݒ肵�Ă���K�v�Ȋg���@�\���T�|�[�g����Ă��Ȃ��ꍇ�A
		* VK_ERROR_EXTENSION_NOT_PRESENT�G���[�R�[�h����������B
		* ����ɖ��R�ɑΉ����邽�߂ɁAvkEnumerateInstanceExtensionProperties�֐��ŃT�|�[�g����Ă���g���@�\���擾���A
		* �K�v�Ȋg���@�\���T�|�[�g����Ă��邩�����O�`�F�b�N����B
		* 
		*/
		bool CheckExtensionsAvailable(std::vector<const char*> required_instance);

	private:
		GLFWwindow* window_;
		VkInstance instance_;
	};
}