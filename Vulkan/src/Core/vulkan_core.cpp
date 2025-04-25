/**
* @file vulkan_core.cpp
* @brief Vulkan�̕`��Ɋւ����{�I�ȏ�������������N���X�̒�`�t�@�C��
* @author �͂��Ƃ�
* @date 2025/4/17
*/

#include "Core/vulkan_core.h"

#include<stdexcept>
#include<optional>

namespace Core {
	/**
	* @brief
	* �L���[�t�@�~���[�̃C���f�b�N�X�������\����
	*/
	struct QueueFamilyIndices {
		std::optional<uint32_t> graphics_family_;

		/**
		* @fn
		* @brief �O���t�B�b�N�X�t�@�~���[�̏����𖞂����Ă��邩�ǂ���
		*/
		bool IsComplete() const {
			return graphics_family_.has_value();
		}
	};


	void VulkanApplication::InitWindow() {
		glfwInit();
		// GLFW��OpenGL��context����邽�߂ɐ݌v����Ă��邽�߁A�܂�����𐧌䂷��K�v������B
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		// �E�B���h�E�̃��T�C�Y�͓��ʂȑΉ����K�v�ɂȂ邽�߂�������ł��Ȃ��悤�ɂ���
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		// �l�ڂ̃p�����[�^�͉�ʂ��J�����j�^�[���w��ł��A�܂ڂ̃p�����[�^��OpenGL�ȊO�ł͕K�v�Ȃ�
		window_ = glfwCreateWindow(kWidth, kHeight, "Vulkan", nullptr, nullptr);
	}

	void VulkanApplication::InitVulkan() {
		CreateVkInstance();
		SetupDebugMessenger();
		PickPhysicalDevice();
		CreateLogicalDevice();
	}
	void VulkanApplication::MainLoop() {
		// �G���[���o��܂ł̓E�B���h�E�ɑ΂���C�x���g���ώ@��������
		while (!glfwWindowShouldClose(window_)) {
			glfwPollEvents();
		}
	}

	void VulkanApplication::CleanUp() {
		if (kEnableValidationLayers) {
			DestroyDebugUtilsMessenger(nullptr);
		}
		// �C���X�^���X�͍Ō�ɔj�����邱��
		vkDestroyDevice(device_, nullptr);
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
		if (kEnableValidationLayers && !CheckValidationLayerSupport()) {
			throw std::runtime_error("�o���f�[�V�������C���[�̐ݒ肪���߂��܂������A���p�\�ł͂���܂���");
		}
		VkApplicationInfo app_info{};
		// Vulkan�ł͍\���̂�sType�����o�[�Ŗ����I�ɍ\���̂̌^��ݒ肷��K�v������B
		// pNext �����o�Ŋg������ݒ肷�邱�Ƃ��ł��邪�A�Ƃ肠����nullptr�̂܂܂ɂ���
		app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.pApplicationName = "Triangle";
		app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		app_info.pEngineName = "No Engine";
		app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		app_info.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		create_info.pApplicationInfo = &app_info;
		auto extensions = GetRequiredExtensions();
		if (!CheckExtensionsAvailable(extensions)) {
			throw std::runtime_error("�g���@�\���T�|�[�g����Ă��܂���I");
		}

		create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
		create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		create_info.ppEnabledExtensionNames = extensions.data();

		// �C���X�^���X���� / �j�����̃G���[�f�o�b�O�Ή�
		// �C���X�^���X�������� pNext�Ƀf�o�b�O����^����
		VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
		// �o���f�[�V�������C���̐ݒ�
		if (kEnableValidationLayers) {
			create_info.enabledLayerCount = static_cast<uint32_t>(kValidationLayers.size());
			create_info.ppEnabledLayerNames = kValidationLayers.data();

			// �C���X�^���X�������̃o���f�[�V�������ݒ�
			InitializeDebugMessengerCreateInfo(debug_create_info);
			create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debug_create_info;
		}
		else {
			create_info.enabledLayerCount = 0;
			create_info.pNext = nullptr;
		}
		// ��Ԗڂ̈����̓J�X�^���A���P�[�^�[�ւ̃|�C���^
		VkResult result = vkCreateInstance(&create_info, nullptr, &instance_);
		if (result != VK_SUCCESS) {
			throw std::runtime_error("VkInstance�̐����Ɏ��s���܂����I");
		}
	}

	bool VulkanApplication::CheckExtensionsAvailable(std::vector<const char*> required_instance) {
		uint32_t extension_count = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
		// �v�f�����擾���Ă���g���@�\�z���p�ӂ��Ė��O���擾
		std::vector<VkExtensionProperties> extensions(extension_count);
		vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());

		// �܂܂�Ă��邩�ǂ������m�F����
		for (const char* instance : required_instance) {
			bool isAvailable = false;
			for (const auto& prop : extensions) {
				if (strcmp(instance, prop.extensionName) == 0) {
					isAvailable = true;
					break;
				}
			}
			if (!isAvailable) {
				return false;
			}
		}
		return true;
	}

	std::vector<const char*>  VulkanApplication::GetRequiredExtensions() {
		// global extension �̐ݒ�
		// Vulkan�̓v���b�g�t�H�[����ˑ��ł��邽�߁A�E�B���h�E�V�X�e���Ɛڑ�����ɂ͊g���@�\���K�v
		uint32_t glfw_extension_count = 0;
		// ������̔z��Ȃ̂Ń|�C���^�̃|�C���^�ɂȂ�
		const char** glfw_extensions;

		glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
		// MacOS�ł�VK_ERROR_INCOMPATIBLE_DRIVER�G���[����������
		// ����ɑΉ�����ɂ́AVK_KHR_PORTABILITY_subset �g����ǉ�����K�v������B
		std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);
		extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
		if (kEnableValidationLayers) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}
		return extensions;
	}

	bool VulkanApplication::CheckValidationLayerSupport() {
		// �g�p�\�ȃ��C���[�����ׂĎ擾
		uint32_t layer_count;
		vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

		std::vector<VkLayerProperties> available_layers(layer_count);
		vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

		// �o���f�[�V�������C���[���g�p�\���ǂ����m�F
		for (const char* layer_name : kValidationLayers) {
			bool layer_found = false;
			for (const VkLayerProperties& prop : available_layers) {
				if (strcmp(layer_name, prop.layerName) == 0) {
					layer_found = true;
					break;
				}
			}
			if (!layer_found) {
				return false;
			}
		}
		return true;
	}
	void VulkanApplication::InitializeDebugMessengerCreateInfo(
		VkDebugUtilsMessengerCreateInfoEXT& create_info				// �f�o�b�O���b�Z���W���[�������C���X�^���X�ւ̎Q��
	) {
		create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		// �t���O�̓R�[���o�b�N���Ăяo�����b�Z�[�W�̎�ނ�ݒ肷��
		create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		create_info.pfnUserCallback = DebugCallBack;
		create_info.pUserData = nullptr;
	}

	void VulkanApplication::SetupDebugMessenger() {
		if (!kEnableValidationLayers) {
			return;
		}
		VkDebugUtilsMessengerCreateInfoEXT create_info;
		InitializeDebugMessengerCreateInfo(create_info);

		if (CreateDebugUtilsMessenger(&create_info, nullptr) != VK_SUCCESS) {
			throw std::runtime_error("�f�o�b�O���b�Z���W���[�̐ݒ�Ɏ��s���܂����B");
		}
	}

	VkResult VulkanApplication::CreateDebugUtilsMessenger(
		const VkDebugUtilsMessengerCreateInfoEXT* p_create_info,	// �f�o�b�O���b�Z���W���[�̐������
		const VkAllocationCallbacks* p_allocator					// �J�X�^���A���P�[�^�[�ւ̃|�C���^
	) {
		// �f�o�b�O���b�Z���W���[�𐶐�����֐��͊g���@�\�ł��邽�߃f�t�H���g�Ń��[�h����Ȃ�
		// vkGetInstanceProcAddr �֐����g���Ċ֐��̃|�C���^����������K�v������B
		auto proc = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance_, "vkCreateDebugUtilsMessengerEXT");
		if (proc != nullptr) {
			return proc(instance_, p_create_info, p_allocator, &debug_messenger_);
		}
		else {
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	void VulkanApplication::DestroyDebugUtilsMessenger(
		VkAllocationCallbacks* p_allocator
	) {
		auto proc = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance_, "vkDestroyDebugUtilsMessengerEXT");
		if (proc != nullptr) {
			proc(instance_, debug_messenger_, p_allocator);
		}
	}

	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) {
		QueueFamilyIndices indices;
		uint32_t queue_family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
		std::vector<VkQueueFamilyProperties> queue_family_properties(queue_family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_family_properties.data());
		// �O���t�B�b�N�R�}���h���T�|�[�g���Ă���L���[�̃C���f�b�N�X���擾����B
		int graphics_queue_index = 0;
		for (const auto& prop : queue_family_properties) {
			if (prop.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphics_family_ = graphics_queue_index;
			}
			if (indices.IsComplete()) {
				break;
			}
			graphics_queue_index += 1;
		}
		return indices;
	}
	/**
	* @fn
	* @brief
	* �f�o�C�X���K�����ǂ������m�F����B
	*/
	bool IsDeviceSuitable(VkPhysicalDevice device) {
		/*
		VkPhysicalDeviceProperties device_properties;
		VkPhysicalDeviceFeatures device_features;
		vkGetPhysicalDeviceProperties(device, &device_properties);
		vkGetPhysicalDeviceFeatures(device, &device_features);

		return device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
			&& device_features.geometryShader;
		*/
		QueueFamilyIndices indices = FindQueueFamilies(device);
		return indices.IsComplete();
	}
	void VulkanApplication::PickPhysicalDevice() {
		uint32_t device_count = 0;
		vkEnumeratePhysicalDevices(instance_, &device_count, nullptr);
		if (device_count == 0) {
			throw std::runtime_error("Vulkan���T�|�[�g���Ă���f�o�C�X�����݂��܂���");
		}
		std::vector<VkPhysicalDevice> devices(device_count);
		vkEnumeratePhysicalDevices(instance_, &device_count, devices.data());
		for (const auto& device : devices) {
			if (IsDeviceSuitable(device)) {
				physical_device_ = device;
				break;
			}
		}
		if (physical_device_ == VK_NULL_HANDLE) {
			throw std::runtime_error("�����ɓK����GPU�����݂��܂���ł���");
		}
	}

	void VulkanApplication::CreateLogicalDevice() {
		// �L���[�̏����܂��܂Ƃ߂�B
		QueueFamilyIndices indices = FindQueueFamilies(physical_device_);

		VkDeviceQueueCreateInfo queue_create_info{};
		queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_info.queueFamilyIndex = indices.graphics_family_.value();
		// ���ݗ��p�\�ȃh���C�o�ł́A�L���[�t�@�~���[���Ƃɏ����̃L���[�����쐬�ł����A���ۂɂ͕����̃L���[�͕K�v�Ȃ��B
		// ����́A���ׂẴR�}���h�o�b�t�@�𕡐��̃X���b�h�ō쐬���A
		// �I�[�o�[�w�b�h�̏��Ȃ��P��̌Ăяo���Ń��C���X���b�h�Ɉꊇ���M�ł��邽�߂ł���B
		queue_create_info.queueCount = 1;
		// �L���[�̗D��x�͂��Ƃ��L���[����ł��ݒ肷��K�v������B
		float queue_priority = 1.0f;
		queue_create_info.pQueuePriorities = &queue_priority;

		// �_���f�o�C�X�̓��������ɂ܂Ƃ߂�B
		// ����͉����ݒ肵�Ȃ��B�@����l�X�ȋ@�\���g�������Ȃ������ɓK�X�t���O�𗧂ĂĂ���
		VkPhysicalDeviceFeatures device_features{};
		
		// �_���f�o�C�X�̐����������ɂ܂Ƃ߂�B
		VkDeviceCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		create_info.pQueueCreateInfos = &queue_create_info;
		create_info.queueCreateInfoCount = 1;
		create_info.pEnabledFeatures = &device_features;

		// �f�o�C�X�ŗL�̊g���@�\��o���f�[�V�������C����ݒ肷��B
		// �g���@�\��o���f�[�V�������C����VkInstance�Őݒ肷����̂Ɠ����ł���B
		create_info.enabledExtensionCount = 0;
		if (kEnableValidationLayers) {
			create_info.enabledLayerCount = static_cast<uint32_t>(kValidationLayers.size());
			create_info.ppEnabledLayerNames = kValidationLayers.data();
		}
		else {
			create_info.enabledLayerCount = 0;
		}
		if (vkCreateDevice(physical_device_, &create_info, nullptr, &device_) != VK_SUCCESS) {
			throw std::runtime_error("�_���f�o�C�X�̐����Ɏ��s���܂����I");
		}
		vkGetDeviceQueue(device_, indices.graphics_family_.value(), 0, &graphics_queue_);
	}
}