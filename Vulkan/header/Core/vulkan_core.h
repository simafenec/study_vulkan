#pragma once
/**
* @file VulkanCore.h
* @brief Vulkan�̕`��Ɋւ����{�I�ȏ�������������N���X�̐錾�t�@�C��
* @author �͂��Ƃ�
* @date 2025/4/17
*/

#include<vector>
#include<iostream>
#include<fstream>

// �E�B���h�E�����Ɋւ���e��@�\��錾����w�b�_�[
#define GLFW_INCLUDE_VULKAN 1
// Vulkan�̊e��֐���\���́A�񋓌^��񋟂��Ă����w�b�_�[
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include<optional>

namespace Core
{
	/**
	* @brief
	* �L���[�t�@�~���[�̃C���f�b�N�X�������\����
	*/
	struct QueueFamilyIndices {
		std::optional<uint32_t> graphics_family_;
		std::optional<uint32_t> present_family_;

		/**
		* @fn
		* @brief �O���t�B�b�N�X�t�@�~���[�̏����𖞂����Ă��邩�ǂ���
		*/
		bool IsComplete() const {
			return graphics_family_.has_value() && present_family_.has_value();
		}
	};
	/**
	* @brief
	* �X���b�v�`�F�C���̏ڍׂȏ��������\����
	*/
	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities_;
		std::vector<VkSurfaceFormatKHR> formats_;
		std::vector<VkPresentModeKHR> present_modes_;
	};

	class VulkanApplication {
	public:
		const uint32_t kWidth = 800;
		const uint32_t kHeight = 600;
	private:
		const std::vector<const char*> kValidationLayers = {
			"VK_LAYER_KHRONOS_validation"
		};
		const std::vector<const char*> kDeviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
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
		* Vulkan�̊e�R�}���h���s�ɓK�؂ȃL���[�t�@�~���[��T��
		*/
		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);

		/**
		* @fn
		* @brief
		* �����ɗ^����ꂽ�f�o�C�X���v���𖞂������ǂ������m�F����
		*/
		bool IsDeviceSuitable(VkPhysicalDevice device);

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

		/**
		* @fn
		* @brief
		* �E�B���h�E�T�[�t�F�X�𐶐�����B
		*/
		void CreateSurface();

		/**
		* @fn
		* @brief
		* �����f�o�C�X���g���@�\���T�|�[�g���Ă��邩�ǂ������m�F����B
		*/
		bool CheckDeviceExtensionSupport(VkPhysicalDevice device);

		/**
		* @fn
		* @brief
		* �X���b�v�`�F�C���̃T�|�[�g�Ɋւ���ڍׂ𒲂ׂ�B
		*/
		SwapChainSupportDetails QuerySwapChainSupprot(VkPhysicalDevice device);

		/**
		* @fn
		* @brief
		* �T�|�[�g���Ă���t�H�[�}�b�g�̒�����K�؂Ȃ��̂�I������B
		*/
		VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats);

		/**
		* @fn
		* @brief
		* �T�|�[�g���Ă���\�����[�h�̒�����K�؂Ȃ��̂�I������B
		*/
		VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes);

		/**
		* @fn
		* @brief
		* �X���b�v�`�F�C�����̉摜�̉𑜓x��K�؂ɐݒ肷��B
		*/
		VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

		/**
		* @fn
		* @brief
		* �X���b�v�`�F�C���𐶐�����B
		*/
		void CreateSwapChain();

		/**
		* @fn
		* @brief
		* �X���b�v�`�F�C�����̉摜���������߂Ɏg����C���[�W�r���[�𐶐�����B
		*/
		void CreateImageViews();

		/**
		* @fn
		* @brief
		* �`��R�}���h����������O���t�B�b�N�X�p�C�v���C���𐶐�����B
		*/
		void CreateGraphicsPipeline();

		/**
		* @fn
		* @brief
		* �t�@�C����ǂݍ���
		* @todo �ʃt�@�C���ɐ؂�o��
		*/
		static std::vector<char> ReadFile(const std::string& filePath) {
			// ate : �t�@�C���̍Ōォ��ǂݎ����J�n���� �ǂݎ��ʒu���g���ăt�@�C���̃T�C�Y�����߁A�o�b�t�@�����蓖�Ă邱�Ƃ��ł���
			// binary : �t�@�C�����o�C�i���t�@�C���Ƃ��ēǂݎ��
			std::ifstream file(filePath, std::ios::ate | std::ios::binary);
			if (!file.is_open()) {
				throw std::runtime_error("�t�@�C�����J���܂���ł���");
			}
			// �������𕶎���̍ŏI�����̏ꏊ�œ���
			size_t file_size = (size_t)file.tellg();
			std::vector<char> buffer(file_size);
			// ������̐擪�ɖ߂��ēǂݍ���
			file.seekg(0);
			file.read(buffer.data(), file_size);
			file.close();
			return buffer;
		}

		/**
		* @fn
		* @brief
		* �V�F�[�_�[�R�[�h�����b�v����V�F�[�_�[���W���[���𐶐�����
		*/
		VkShaderModule CreateShaderModule(
			const std::vector<char>& code // �V�F�[�_�[�R�[�h
		);

		/**
		* @fn
		* @brief
		* �����_�[�p�X�𐶐�����
		*/
		void CreateRenderPass();

		/**
		* @fn
		* @brief
		* �t���[���o�b�t�@�𐶐�����B
		*/
		void CreateFramebuffers();

		/**
		* @fn
		* @brief
		* �R�}���h�v�[���𐶐�����B
		*/
		void CreateCommandPool();

		/**
		* @fn
		* @brief
		* �R�}���h�o�b�t�@�𐶐�����B
		*/
		void CreateCommandBuffer();

		/**
		* @fn
		* @brief
		* �R�}���h�o�b�t�@�ɕ`��̂��߂̈�A�̑�����L�^����B
		*/
		void RecordCommandBuffer(
			VkCommandBuffer command_buffer,		// �R�}���h�̋L�^�Ɏg�p����R�}���h�o�b�t�@
			uint32_t image_index				// �������ݐ�̃X���b�v�`�F�[���C���[�W�̃C���f�b�N�X
		);

	private:
		GLFWwindow* window_;
		VkInstance instance_;
		VkDebugUtilsMessengerEXT debug_messenger_;
		VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
		VkDevice device_;
		VkQueue graphics_queue_;
		VkQueue present_queue_;
		VkSurfaceKHR surface_;
		VkSwapchainKHR swap_chain_;
		std::vector<VkImage> swap_chain_images_;
		std::vector<VkImageView> swap_chain_image_views_;
		std::vector<VkFramebuffer> swap_chain_frame_buffers_;
		VkFormat swap_chain_image_format_;
		VkExtent2D swap_chain_extent_;
		VkRenderPass render_pass_;
		VkPipelineLayout pipeline_layout_;
		VkPipeline graphics_pipeline_;
		VkCommandPool command_pool_;
		VkCommandBuffer command_buffer_;
	};
}