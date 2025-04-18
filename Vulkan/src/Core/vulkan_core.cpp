#include "Core/vulkan_core.h"

#include<stdexcept>
#include<iostream>

namespace Core {
	void VulkanApplication::InitWindow() {
		glfwInit();
		// GLFW��OpenGL��context����邽�߂ɐ݌v����Ă��邽�߁A�܂�����𐧌䂷��K�v������B
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		// �E�B���h�E�̃��T�C�Y�͓��ʂȑΉ����K�v�ɂȂ邽�߂�������ł��Ȃ��悤�ɂ���
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		// �l�ڂ̃p�����[�^�͉�ʂ��J�����j�^�[���w��ł��A�܂ڂ̃p�����[�^��OpenGL�ȊO�ł͕K�v�Ȃ�
		window_ = glfwCreateWindow(kWIDTH, kHEIGHT, "Vulkan", nullptr, nullptr);
	}

	void VulkanApplication::InitVulkan() {
		CreateVkInstance();
	}
	void VulkanApplication::MainLoop() {
		// �G���[���o��܂ł̓E�B���h�E�ɑ΂���C�x���g���ώ@��������
		while (!glfwWindowShouldClose(window_)) {
			glfwPollEvents();
		}
	}

	void VulkanApplication::CleanUp() {
		vkDestroyInstance(instance_, nullptr);
		glfwDestroyWindow(window_);
		glfwTerminate();
	}

	void VulkanApplication::Run() {
		InitWindow();
		InitVulkan();
		MainLoop();
		CleanUp();
	}

	void VulkanApplication::CreateVkInstance() {
		VkApplicationInfo app_info{};
		CreateVkApplicationInfo(app_info);
		VkInstanceCreateInfo create_info{};
		CreateVkInstanceCreateInfo(create_info, app_info);
		// ��Ԗڂ̈����̓J�X�^���A���P�[�^�[�ւ̃|�C���^
		if (vkCreateInstance(&create_info, nullptr, &instance_) != VK_SUCCESS) {
			throw std::runtime_error("VkInstance�̐����Ɏ��s���܂����I");
		}
	}

	/**
	* @fn
	* @brief Vulkan �A�v���P�[�V�����̊�{����ݒ肷��
	*/
	void VulkanApplication::CreateVkApplicationInfo(VkApplicationInfo& info) {
		// Vulkan�ł͍\���̂�sType�����o�[�Ŗ����I�ɍ\���̂̌^��ݒ肷��K�v������B
		// pNext �����o�Ŋg������ݒ肷�邱�Ƃ��ł��邪�A�Ƃ肠����nullptr�̂܂܂ɂ���
		info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		info.pApplicationName = "Triangle";
		info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		info.pEngineName = "No Engine";
		info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		info.apiVersion = VK_API_VERSION_1_0;
	}

	/**
	* @fn
	* @brief VkInstance�̐����ݒ���s��
	* @detail �f�o�C�X�ɂ�����炸�S�Ẵv���O�����ɓK�p����g��(extensions)�Ǝg�p����o���f�[�V�������C���̐ݒ���s���B
	*/
	void VulkanApplication::CreateVkInstanceCreateInfo(VkInstanceCreateInfo& create_info, const VkApplicationInfo& application_info) {
		create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		create_info.pApplicationInfo = &application_info;

		// global extension �̐ݒ�
		// Vulkan�̓v���b�g�t�H�[����ˑ��ł��邽�߁A�E�B���h�E�V�X�e���Ɛڑ�����ɂ͊g���@�\���K�v
		uint32_t glfw_extension_count = 0;
		// ������̔z��Ȃ̂Ń|�C���^�̃|�C���^�ɂȂ�
		const char** glfw_extensions;

		glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

		uint32_t extension_count = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
		// MacOS�ł�VK_ERROR_INCOMPATIBLE_DRIVER�G���[����������
		// ����ɑΉ�����ɂ́AVK_KHR_PORTABILITY_subset �g����ǉ�����K�v������B
		//std::vector<const char*> required_extensions;
		//for (uint32_t index = 0; index < glfw_extension_count; index++) {
		//	required_extensions.emplace_back(glfw_extensions[index]);
		//}
		//required_extensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

		//create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

		//if (!CheckExtensionsAvailable(required_extensions)) {
		//	throw std::runtime_error("�g���@�\���T�|�[�g����Ă��܂���I");
		//}
		//create_info.enabledExtensionCount = (uint32_t)required_extensions.size();
		//create_info.ppEnabledExtensionNames = required_extensions.data();
		create_info.enabledExtensionCount = glfw_extension_count;
		create_info.ppEnabledExtensionNames = glfw_extensions;

		// �o���f�[�V�������C���̐ݒ�
		create_info.enabledLayerCount = 0;
	}

	bool VulkanApplication::CheckExtensionsAvailable(std::vector<const char*> required_instance) {
		uint32_t extension_count = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
		// �v�f�����擾���Ă���g���@�\�z���p�ӂ��Ė��O���擾
		std::vector<VkExtensionProperties> extensions(extension_count);
		vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());
		// �g���@�\���̔z������
		std::vector<const char*> extension_names;
		for (const auto& property : extensions) {
			extension_names.emplace_back(property.extensionName);
		}
		// �܂܂�Ă��邩�ǂ������m�F����
		for (const auto& instance : required_instance) {
			if (std::find(extension_names.begin(), extension_names.end(), instance) == extension_names.end()) {
				return false;
			}
		}
		return true;
	}
}