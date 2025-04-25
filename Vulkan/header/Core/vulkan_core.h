#pragma once
/**
* @file VulkanCore.h
* @brief Vulkan�̕`��Ɋւ����{�I�ȏ�������������N���X�̐錾�t�@�C��
* @author �͂��Ƃ�
* @date 2025/4/17
*/

#include<vector>
#include<iostream>

// �E�B���h�E�����Ɋւ���e��@�\��錾����w�b�_�[
#define GLFW_INCLUDE_VULKAN 1
// Vulkan�̊e��֐���\���́A�񋓌^��񋟂��Ă����w�b�_�[
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

		/**
		* @fn
		* @brief �K�v�Ȋg���@�\�̃��X�g���擾����B
		*/
		std::vector<const char*> GetRequiredExtensions();

		/**
		* @fn
		* @brief
		* �o���f�[�V�������C�����T�|�[�g����Ă��邩�ǂ������m�F����B
		*/
		bool CheckValidationLayerSupport();

		/**
		* @fn
		* @brief
		* �f�o�b�O���b�Z���W���[�̐�����������������
		*/
		void InitializeDebugMessengerCreateInfo(
			VkDebugUtilsMessengerCreateInfoEXT& create_info				// �f�o�b�O���b�Z���W���[�������C���X�^���X�ւ̎Q��
		);
		/**
		* @fn
		* @brief
		* �f�o�b�O���b�Z�[�W�̒ʒm�V�X�e�����Z�b�g�A�b�v����
		*/
		void SetupDebugMessenger();

		/**
		* @fn
		* @brief 
		* �f�o�b�O���b�Z���W���[�𐶐�����
		*/
		VkResult CreateDebugUtilsMessenger(
			const VkDebugUtilsMessengerCreateInfoEXT* p_create_info,	// �f�o�b�O���b�Z���W���[�̐������
			const VkAllocationCallbacks* p_allocator					// �J�X�^���A���P�[�^�[�ւ̃|�C���^
		);

		/**
		* @fn
		* @brief
		* �f�o�b�O���b�Z���W���[��j������
		*/
		void DestroyDebugUtilsMessenger(
			VkAllocationCallbacks* p_allocator
		);

		/**
		* @fn
		* @brief
		* �f�o�b�O���b�Z�[�W�̃R�[���o�b�N�֐�
		* @return
		* Validation Layer ���g���K�����֐��̏�����abort���ׂ����ǂ���
		*/
		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallBack(
			VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,	 // �f�o�b�O���b�Z�[�W�̏d�v�x
			VkDebugUtilsMessageTypeFlagsEXT message_type,				 // �����������b�Z�[�W�̎��
			const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data, // ���b�Z�[�W�̏ڍׂȓ��e���܂ލ\����
			void* p_user_data											 // �R�[���o�b�N�̃Z�b�g�A�b�v���ɗ^������Ǝ��f�[�^
		) {
			// VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: �f�f���b�Z�[�W
			// VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: ���\�[�X���b�Z�[�W�Ȃǂ̏��񋟃��b�Z�[�W
			// VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT : �G���[�ł͂Ȃ����o�O�ɋ߂��x�����b�Z�[�W
			// VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT : �N���b�V���������N��������s���ȓ�����ʒm���郁�b�Z�[�W
			// ���ɍs���΍s���قǏd�v�x�� (���蓖�Ă�ꂽ�l���傫��)
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
		* Vulkan�̕`��Ɏg�p���镨���f�o�C�X��I������
		*/
		void PickPhysicalDevice();

		/**
		* @fn
		* @brief
		* �_���f�o�C�X���쐬����B
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