/**
* @file vulkan_core.cpp
* @brief Vulkan�̕`��Ɋւ����{�I�ȏ�������������N���X�̒�`�t�@�C��
* @author �͂��Ƃ�
* @date 2025/4/17
*/

#include "Core/vulkan_core.h"

#include<stdexcept>
#include<set>
#include<cstdint>
#include<limits>
#include<algorithm>

namespace Core {
	/**
	* �l�p�`�`��̂��߂̒��_�f�[�^
	* ���u��
	*/
	std::vector<Vertex> vertices = {
		{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},	// ���� �ԐF
		{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},	// �E�� �ΐF
		{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},		// �E�� �F
		{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}		// ���� ���F
	};
	/**
	* �l�p�`�`��̂��߂̃C���f�b�N�X�f�[�^
	* 
	*/
	std::vector<uint16_t> indices = {
		0, 1, 2,		// �E��̎O�p�`
		2, 3, 0			// �����̎O�p�`
	};
	void VulkanApplication::InitWindow() {
		glfwInit();
		// GLFW��OpenGL��context����邽�߂ɐ݌v����Ă��邽�߁A�܂�����𐧌䂷��K�v������B
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		// �E�B���h�E�̃��T�C�Y�͓��ʂȑΉ����K�v�ɂȂ邽�߂�������ł��Ȃ��悤�ɂ���
		// glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		// �l�ڂ̃p�����[�^�͉�ʂ��J�����j�^�[���w��ł��A�܂ڂ̃p�����[�^��OpenGL�ȊO�ł͕K�v�Ȃ�
		window_ = glfwCreateWindow(kWidth, kHeight, "Vulkan", nullptr, nullptr);
		glfwSetWindowUserPointer(window_, this);
		glfwSetFramebufferSizeCallback(window_, FramebufferReizeCallback);
	}

	void VulkanApplication::InitVulkan() {
		CreateVkInstance();
		SetupDebugMessenger();
		CreateSurface();
		PickPhysicalDevice();
		CreateLogicalDevice();
		CreateSwapChain();
		CreateImageViews();
		CreateRenderPass();
		CreateGraphicsPipeline();
		CreateFramebuffers();
		CreateCommandPool();
		CreateVertexBuffer();
		CreateIndexBuffer();
		CreateCommandBuffers();
		CreateSyncObjects();
	}
	void VulkanApplication::MainLoop() {
		// �G���[���o��܂ł̓E�B���h�E�ɑ΂���C�x���g���ώ@��������
		while (!glfwWindowShouldClose(window_)) {
			glfwPollEvents();
			DrawFrame();
		}
		// ���C�����[�v���I������Ƃ���Ɋe��I�u�W�F�N�g���j�������Ə������̂��̂��c���Ă���Ƃ��ɍ���̂�
		// �_���f�o�C�X��Idle�ɂȂ�܂őҋ@����B
		vkDeviceWaitIdle(device_);
	}

	void VulkanApplication::CleanUp() {
		vkDestroyCommandPool(device_, command_pool_, nullptr);
		vkDestroyCommandPool(device_, transfer_command_pool_, nullptr);
		// �C���X�^���X�͍Ō�ɔj�����邱��
		for (int index = 0; index < kMaxFramesInFlight; index++) {
			vkDestroySemaphore(device_, image_available_semaphores_[index], nullptr);
			vkDestroySemaphore(device_, render_finished_semaphores_[index], nullptr);
			vkDestroyFence(device_, in_flight_fences_[index], nullptr);
		}
		CleanUpSwapChainDependents();
		vkDestroySwapchainKHR(device_, swap_chain_, nullptr);
		vkDestroyBuffer(device_, vertex_buffer_, nullptr);
		vkFreeMemory(device_, vertex_buffer_memory_, nullptr);
		vkDestroyBuffer(device_, index_buffer_, nullptr);
		vkFreeMemory(device_, index_buffer_memory_, nullptr);
		vkDestroyDevice(device_, nullptr);

		if (kEnableValidationLayers) {
			DestroyDebugUtilsMessenger(nullptr);
		}

		vkDestroySurfaceKHR(instance_, surface_, nullptr);
		vkDestroyInstance(instance_, nullptr);
		glfwDestroyWindow(window_);
		glfwTerminate();
	}

	void VulkanApplication::CleanUpSwapChainDependents() {
		for (auto framebuffer : swap_chain_frame_buffers_) {
			vkDestroyFramebuffer(device_, framebuffer, nullptr);
		}
		for (auto image_view : swap_chain_image_views_) {
			vkDestroyImageView(device_, image_view, nullptr);
		}
		vkDestroyPipeline(device_, graphics_pipeline_, nullptr);
		vkDestroyPipelineLayout(device_, pipeline_layout_, nullptr);
		vkDestroyRenderPass(device_, render_pass_, nullptr);
	}

	void VulkanApplication::Run() {
		InitWindow();
		InitVulkan();
		MainLoop();
		CleanUp();
	}

	void VulkanApplication::DrawFrame() {
		// ��O�̃t���[����ҋ@
		vkWaitForFences(device_, 1, &in_flight_fences_[current_frame_], VK_TRUE, UINT64_MAX);
		uint32_t image_index;
		VkResult result = vkAcquireNextImageKHR(device_, swap_chain_, UINT64_MAX, image_available_semaphores_[current_frame_], VK_NULL_HANDLE, &image_index);
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			RecreateSwapChain();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("�X���b�v�`�F�[���C���[�W�̎擾�Ɏ��s���܂����I");
		}
		// �t�F���X�𖾎��I�Ƀ��Z�b�g
		vkResetFences(device_, 1, &in_flight_fences_[current_frame_]);
		
		// �R�}���h�o�b�t�@�ɕ`��R�}���h���L�^
		vkResetCommandBuffer(command_buffers_[current_frame_], 0);
		RecordCommandBuffer(command_buffers_[current_frame_], image_index);
		// �R�}���h�̒�o�^�C�~���O�Ȃǂ�ݒ�
		VkSubmitInfo submit_info{};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore wait_semaphores[] = { image_available_semaphores_[current_frame_]};
		VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		submit_info.waitSemaphoreCount = 1;
		submit_info.pWaitSemaphores = wait_semaphores;
		submit_info.pWaitDstStageMask = wait_stages;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &command_buffers_[current_frame_];
		VkSemaphore signal_semaphores[] = { render_finished_semaphores_[current_frame_]};
		submit_info.signalSemaphoreCount = 1;
		submit_info.pSignalSemaphores = signal_semaphores;
		if (vkQueueSubmit(graphics_queue_, 1, &submit_info, in_flight_fences_[current_frame_] ) != VK_SUCCESS) {
			throw std::runtime_error("�`��R�}���h�̔��s�Ɏ��s���܂����I");
		}
		VkPresentInfoKHR present_info{};
		present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present_info.waitSemaphoreCount = 1;
		present_info.pWaitSemaphores = signal_semaphores;
		VkSwapchainKHR swap_chains[] = { swap_chain_ };
		present_info.swapchainCount = 1;
		present_info.pSwapchains = swap_chains;
		present_info.pImageIndices = &image_index;
		present_info.pResults = nullptr;

		result = vkQueuePresentKHR(present_queue_, &present_info);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebuffer_resized_) {
			framebuffer_resized_ = false;
			RecreateSwapChain();
		}
		else if (result != VK_SUCCESS) {
			throw std::runtime_error("�X���b�v�`�F�[���C���[�W�̎擾�Ɏ��s���܂����I");
		}

		// ���W�����ɂ��邱�Ƃŏ�ɗ��p�\�Ȕ͈͂̃C���f�b�N�X�𔽕�����悤�ɂȂ�
		current_frame_ = (current_frame_ + 1) % kMaxFramesInFlight;
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

	QueueFamilyIndices VulkanApplication::FindQueueFamilies(VkPhysicalDevice device) {
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
			// Presentation Queue�̑��݃`�F�b�N
			VkBool32 presentSupported = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, graphics_queue_index, surface_, &presentSupported);
			if (presentSupported) {
				indices.present_family_ = graphics_queue_index;
			}
			if (indices.IsComplete()) {
				break;
			}
			graphics_queue_index += 1;
		}
		return indices;
	}

	bool VulkanApplication::IsDeviceSuitable(VkPhysicalDevice device) {
		QueueFamilyIndices indices = FindQueueFamilies(device);
		bool extension_supported = CheckDeviceExtensionSupport(device);
		bool swap_chain_adequate = false;
		if (extension_supported) {
			SwapChainSupportDetails swap_chain_support = QuerySwapChainSupprot(device);
			swap_chain_adequate = !swap_chain_support.formats_.empty() && !swap_chain_support.present_modes_.empty();
		}
		return indices.IsComplete() && extension_supported && swap_chain_adequate;
	}
	bool VulkanApplication::CheckDeviceExtensionSupport(VkPhysicalDevice device) {
		uint32_t extension_count = 0;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);
		std::vector<VkExtensionProperties> available_extensions(extension_count);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

		std::set<std::string> required_extensions(kDeviceExtensions.begin(), kDeviceExtensions.end());
		// �v������Ă���g���@�\���̃��X�g����T�|�[�g����Ă���g���@�\���������Ă����A�v�����X�g����ɂȂ��OK
		for (const auto& prop : available_extensions) {
			required_extensions.erase(prop.extensionName);
		}
		return required_extensions.empty();
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

		std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
		std::set<uint32_t> unique_queue_families = { indices.graphics_family_.value(), indices.present_family_.value() };
		float queue_priority = 1.0f;
		for (uint32_t queue_family : unique_queue_families) {
			VkDeviceQueueCreateInfo queue_create_info{};
			queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queue_create_info.queueFamilyIndex = queue_family;
			// ���ݗ��p�\�ȃh���C�o�ł́A�L���[�t�@�~���[���Ƃɏ����̃L���[�����쐬�ł����A���ۂɂ͕����̃L���[�͕K�v�Ȃ��B
			// ����́A���ׂẴR�}���h�o�b�t�@�𕡐��̃X���b�h�ō쐬���A
			// �I�[�o�[�w�b�h�̏��Ȃ��P��̌Ăяo���Ń��C���X���b�h�Ɉꊇ���M�ł��邽�߂ł���B
			queue_create_info.queueCount = 1;
			// �L���[�̗D��x�͂��Ƃ��L���[����ł��ݒ肷��K�v������B
			queue_create_info.pQueuePriorities = &queue_priority;
			queue_create_infos.push_back(queue_create_info);
		}

		// �_���f�o�C�X�̓��������ɂ܂Ƃ߂�B
		// ����͉����ݒ肵�Ȃ��B�@����l�X�ȋ@�\���g�������Ȃ������ɓK�X�t���O�𗧂ĂĂ���
		VkPhysicalDeviceFeatures device_features{};
		
		// �_���f�o�C�X�̐����������ɂ܂Ƃ߂�B
		VkDeviceCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		create_info.pQueueCreateInfos = queue_create_infos.data();
		create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
		create_info.pEnabledFeatures = &device_features;

		// �f�o�C�X�ŗL�̊g���@�\��o���f�[�V�������C����ݒ肷��B
		// �g���@�\��o���f�[�V�������C����VkInstance�Őݒ肷����̂Ɠ����ł���B
		create_info.enabledExtensionCount = static_cast<uint32_t>(kDeviceExtensions.size());
		create_info.ppEnabledExtensionNames = kDeviceExtensions.data();
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
		vkGetDeviceQueue(device_, indices.present_family_.value(), 0, &present_queue_);
	}

	void VulkanApplication::CreateSurface() {
		// glfw�̃E�B���h�E�T�[�t�F�X�������g��
		// ���Ɉˑ����������ł���
		if (glfwCreateWindowSurface(instance_, window_, nullptr, &surface_) != VK_SUCCESS) {
			throw std::runtime_error("�E�B���h�E�T�[�t�F�X�̐����Ɏ��s���܂����I");
		}
	}

	SwapChainSupportDetails VulkanApplication::QuerySwapChainSupprot(VkPhysicalDevice device) {
		SwapChainSupportDetails details;
		// �T�[�t�F�[�X�̊�{���𓾂�
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_, &details.capabilities_);
		// �t�H�[�}�b�g���𓾂�
		uint32_t format_count;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &format_count, nullptr);
		if (format_count != 0) {
			details.formats_.resize(format_count);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &format_count, details.formats_.data());
		}
		// �\�����[�h���𓾂�
		uint32_t present_mode_count;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &present_mode_count, nullptr);
		if (present_mode_count != 0) {
			details.present_modes_.resize(present_mode_count);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &present_mode_count, details.present_modes_.data());
		}
		return details;
	}

	VkSurfaceFormatKHR VulkanApplication::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats) {
		for (const auto& available_format : available_formats) {
			if (
				available_format.format == VK_FORMAT_B8G8R8A8_SRGB && 
				available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
			) {
				return available_format;
			}
		}
		// ���߂Ă�����̂��Ȃ������ꍇ�͐擪�̂��̂��Ƃ肠�����Ԃ�
		return available_formats[0];
	}
	VkPresentModeKHR VulkanApplication::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes) {
		for (const auto& available_present_mode : available_present_modes) {
			if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return available_present_mode;
			}
		}
		return VK_PRESENT_MODE_FIFO_KHR;
	}
	VkExtent2D VulkanApplication::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			return capabilities.currentExtent;
		}
		else {
			// ���݂̉�ʂ̃o�b�t�@�T�C�Y���擾���A�X���b�v�`�F�C���摜�̍ő� / �ŏ���f���Ƃ̊Ԃɓ���悤�ɐݒ肷��B
			int width, height;
			glfwGetFramebufferSize(window_, &width, &height);
			VkExtent2D actual_extent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};
			actual_extent.width = std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actual_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
			return actual_extent;
		}
	}
	void VulkanApplication::CreateSwapChain() {
		SwapChainSupportDetails swap_chain_support = QuerySwapChainSupprot(physical_device_);

		VkSurfaceFormatKHR surface_format = ChooseSwapSurfaceFormat(swap_chain_support.formats_);
		VkPresentModeKHR present_mode = ChooseSwapPresentMode(swap_chain_support.present_modes_);
		VkExtent2D extent = ChooseSwapExtent(swap_chain_support.capabilities_);

		// �X���b�v�`�F�C�������Ă�摜���͗]�T�������Ă����Ƌg
		uint32_t image_count = swap_chain_support.capabilities_.minImageCount + 1;
		if (swap_chain_support.capabilities_.maxImageCount > 0 && image_count > swap_chain_support.capabilities_.maxImageCount) {
			image_count = swap_chain_support.capabilities_.maxImageCount;
		}

		VkSwapchainCreateInfoKHR create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		create_info.surface = surface_;
		create_info.minImageCount = image_count;
		create_info.imageFormat = surface_format.format;
		create_info.imageColorSpace = surface_format.colorSpace;
		create_info.imageExtent = extent;
		create_info.imageArrayLayers = 1; // �摜���\�����郌�C���[�̗ʁB�O������3D�A�v���P�[�V����(VR�I��)�łȂ��������1
		create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // �X���b�v�`�F�C�����̉摜���ǂ̂悤�ȑ���Ɏg�������w�肷��B����͒��ڃ����_�����O����
		// �L���[�t�@�~���[�̊ԂŎg�p�����X���b�v�`�F�C���摜�̏������@���w�肷��B
		// �O���t�B�b�N�X�L���[�ƃv���[���e�[�V�����L���[���قȂ�ꍇ�͉摜���L���[�Ԃŋ��L�ł���悤�ɐݒ肷��B
		// �����I�ɏ��L��������肷�邱�Ƃŋ��L���Ȃ��悤�ɂ��邱�Ƃ��ł���
		QueueFamilyIndices indices = FindQueueFamilies(physical_device_);
		uint32_t queue_family_indices[] = { indices.graphics_family_.value(), indices.present_family_.value() };
		if (indices.graphics_family_ != indices.present_family_) {
			create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			create_info.queueFamilyIndexCount = 2;
			create_info.pQueueFamilyIndices = queue_family_indices;
		}
		else {
			create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			create_info.queueFamilyIndexCount = 0;
			create_info.pQueueFamilyIndices = nullptr;
		}
		// �摜�ɑ΂��鎖�O�̕ϊ��������w�肷��B�����ł͂��̂܂܂ɂ���
		create_info.preTransform = swap_chain_support.capabilities_.currentTransform;
		// �E�B���h�E�V�X�e�����̂ق��̃E�B���h�E�Ƃ̃u�����h�ɃA���t�@�`�����l�����g�����ǂ���
		// ����͖�������ݒ�
		create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		create_info.presentMode = present_mode;
		// ���̃E�B���h�E���O�ɂ���Ȃǂ��ĉB����Ă���s�N�Z���̐F�͍l������Ȃ�
		create_info.clipped = VK_TRUE;
		// �Â��X���b�v�`�F�C���ւ̎Q�� �E�B���h�E�̃T�C�Y���ύX�ɂȂ����ۂȂǂɐݒ肷��K�v������B
		create_info.oldSwapchain = old_swap_chain_;

		if (vkCreateSwapchainKHR(device_, &create_info, nullptr, &swap_chain_) != VK_SUCCESS) {
			throw std::runtime_error("�X���b�v�`�F�C���̐����Ɏ��s���܂���!");
		}
		// �X���b�v�`�F�C�����̉摜�ւ̃n���h�����擾����B
		vkGetSwapchainImagesKHR(device_, swap_chain_, &image_count, nullptr);
		swap_chain_images_.resize(image_count);
		vkGetSwapchainImagesKHR(device_, swap_chain_, &image_count, swap_chain_images_.data());
		// �X���b�v�`�F�C���̐ݒ���e������̎Q�Ƃ̂��߂ɕۑ����Ă���
		swap_chain_image_format_ = surface_format.format;
		swap_chain_extent_ = extent;
	}

	void VulkanApplication::CreateImageViews() {
		swap_chain_image_views_.resize(swap_chain_images_.size());
		for (size_t index = 0; index < swap_chain_images_.size(); index++) {
			VkImageViewCreateInfo create_info{};
			create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			create_info.image = swap_chain_images_[index];
			create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
			create_info.format = swap_chain_image_format_;
			create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			create_info.subresourceRange.baseMipLevel = 0;
			create_info.subresourceRange.levelCount = 1;
			create_info.subresourceRange.baseArrayLayer = 0;
			create_info.subresourceRange.layerCount = 1;

			if (vkCreateImageView(device_, &create_info, nullptr, &swap_chain_image_views_[index]) != VK_SUCCESS) {
				throw std::runtime_error("�C���[�W�r���[�̐����Ɏ��s���܂����I");
			}
		}
	}

	VkShaderModule VulkanApplication::CreateShaderModule(const std::vector<char>& code) {
		VkShaderModuleCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		create_info.codeSize = code.size();
		// uint32_t�̃|�C���^�ɃL���X�g���Ă�����K�v������B
		// vector�̓f�t�H���g�ŃA���P�[�^�����̂ŃA���C�������g�v�����C�ɂ��Ȃ��Ă��悢
		create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());
		VkShaderModule shader_module;
		if (vkCreateShaderModule(device_, &create_info, nullptr, &shader_module) != VK_SUCCESS) {
			throw std::runtime_error("�V�F�[�_�[���W���[���̐����Ɏ��s���܂����I");
		}
		return shader_module;
	}
	
	void VulkanApplication::CreateRenderPass() {
		VkAttachmentDescription color_attachment{};
		color_attachment.format = swap_chain_image_format_;
		color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference color_attachment_reference{};
		color_attachment_reference.attachment = 0;
		color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &color_attachment_reference;

		// �T�u�p�X�ˑ����ݒ�
		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		// ���̓T�u�p�X��������Ȃ��̂�0�Ԗڂ̃C���f�b�N�X���w��
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		
		VkRenderPassCreateInfo render_pass_info{};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_info.attachmentCount = 1;
		render_pass_info.pAttachments = &color_attachment;
		render_pass_info.subpassCount = 1;
		render_pass_info.pSubpasses = &subpass;
		render_pass_info.dependencyCount = 1;
		render_pass_info.pDependencies = &dependency;

		if (vkCreateRenderPass(device_, &render_pass_info, nullptr, &render_pass_) != VK_SUCCESS) {
			throw std::runtime_error("�����_�[�p�X�̐����Ɏ��s���܂���");
		}
	}

	void VulkanApplication::CreateGraphicsPipeline() {
		// ���_�V�F�[�_�[�ƃt���O�����g�V�F�[�_�[��ݒ肷��B
		auto vert_shader_code = ReadFile("shaders/vert.spv");
		auto frag_shader_code = ReadFile("shaders/frag.spv");

		VkShaderModule vert_shader_module = CreateShaderModule(vert_shader_code);
		VkShaderModule frag_shader_module = CreateShaderModule(frag_shader_code);

		VkPipelineShaderStageCreateInfo vert_shader_stage_info{};
		vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vert_shader_stage_info.module = vert_shader_module;
		// �V�F�[�_�[�̃G���g���|�C���g����ݒ肷��
		vert_shader_stage_info.pName = "main";
		
		VkPipelineShaderStageCreateInfo frag_shader_stage_info{};
		frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		frag_shader_stage_info.module = frag_shader_module;
		// �V�F�[�_�[�̃G���g���|�C���g����ݒ肷��
		frag_shader_stage_info.pName = "main";
		// pSpecializationInfo �������ł���Ɏw�肷��ƁA�p�C�v���C���쐬���ɂ��̓����ݒ肷�邱�Ƃ��ł���

		VkPipelineShaderStageCreateInfo shader_stages[] = { vert_shader_stage_info, frag_shader_stage_info };

		// ���_���͂̌`���Ɋւ������^����
		auto binding_description = Vertex::GetBindingDescription();
		auto attribute_description = Vertex::GetAttributeDescriptions();

		VkPipelineVertexInputStateCreateInfo vertex_input_info{};
		vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertex_input_info.vertexBindingDescriptionCount = 1;
		vertex_input_info.pVertexBindingDescriptions = &binding_description;
		vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_description.size());
		vertex_input_info.pVertexAttributeDescriptions = attribute_description.data();

		// Input Assembly ��ݒ肷��
		VkPipelineInputAssemblyStateCreateInfo input_assembly{};
		input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		input_assembly.primitiveRestartEnable = VK_FALSE;

		// Viewport��ݒ肷��
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(swap_chain_extent_.width);
		viewport.height = static_cast<float>(swap_chain_extent_.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		//Scissor ��`��ݒ肷��
		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = swap_chain_extent_;

		// ���I�ɕ`��ݒ��^����X�e�[�W��ݒ�

		std::vector<VkDynamicState> dynamic_states = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamic_state{};
		dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic_state.pDynamicStates = dynamic_states.data();
		dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());

		VkPipelineViewportStateCreateInfo viewport_state{};
		viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		// ���I�ɂ����͐ݒ肷��̂ŁA�J�E���g�����ݒ肵�Ă���
		viewport_state.viewportCount = 1;
		viewport_state.scissorCount = 1;

		// ���X�^���C�U�̐ݒ�
		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f;
		rasterizer.depthBiasClamp = 0.0f;
		rasterizer.depthBiasSlopeFactor = 0.0f;

		// �}���`�T���v�����O�ݒ�
		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f;
		multisampling.pSampleMask = nullptr;
		multisampling.alphaToCoverageEnable = VK_FALSE;
		multisampling.alphaToOneEnable = VK_FALSE;

		// �[�x�e�X�g�ƃX�e���V���e�X�g�̐ݒ�

		// Color Blending�ݒ�
		VkPipelineColorBlendAttachmentState color_blend_attachment{};
		color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		color_blend_attachment.blendEnable = VK_FALSE;
		color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
		color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo color_blending{};
		color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		color_blending.logicOpEnable = VK_FALSE;
		color_blending.logicOp = VK_LOGIC_OP_COPY;
		color_blending.attachmentCount = 1;
		color_blending.pAttachments = &color_blend_attachment;
		color_blending.blendConstants[0] = 0.0f;
		color_blending.blendConstants[1] = 0.0f;
		color_blending.blendConstants[2] = 0.0f;
		color_blending.blendConstants[3] = 0.0f;

		VkPipelineLayoutCreateInfo pipeline_layout_info{};
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.setLayoutCount = 0;
		pipeline_layout_info.pSetLayouts = nullptr;
		pipeline_layout_info.pushConstantRangeCount = 0;
		pipeline_layout_info.pPushConstantRanges = nullptr;

		if (vkCreatePipelineLayout(device_, &pipeline_layout_info, nullptr, &pipeline_layout_) != VK_SUCCESS) {
			throw std::runtime_error("�p�C�v���C�����C�A�E�g�����Ɏ��s���܂���!");
		}

		VkGraphicsPipelineCreateInfo pipeline_info{};
		pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_info.stageCount = 2;
		pipeline_info.pStages = shader_stages;
		pipeline_info.pVertexInputState = &vertex_input_info;
		pipeline_info.pInputAssemblyState = &input_assembly;
		pipeline_info.pViewportState = &viewport_state;
		pipeline_info.pRasterizationState = &rasterizer;
		pipeline_info.pMultisampleState = &multisampling;
		pipeline_info.pDepthStencilState = nullptr;
		pipeline_info.pColorBlendState = &color_blending;
		pipeline_info.pDynamicState = &dynamic_state;
		pipeline_info.layout = pipeline_layout_;
		pipeline_info.renderPass = render_pass_;
		pipeline_info.subpass = 0;
		// �����̃p�C�v���C������h�������ĐV�����O���t�B�b�N�X�p�C�v���C�������ۂɎg����B
		pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
		pipeline_info.basePipelineIndex = -1;

		if (vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &graphics_pipeline_) != VK_SUCCESS) {
			throw std::runtime_error("�O���t�B�b�N�X�p�C�v���C���̐����Ɏ��s���܂���!");
		}

		vkDestroyShaderModule(device_, frag_shader_module, nullptr);
		vkDestroyShaderModule(device_, vert_shader_module, nullptr);
	}

	void VulkanApplication::CreateFramebuffers() {
		// ImageView�̐��ƃt���[���o�b�t�@�̐��͓���
		swap_chain_frame_buffers_.resize(swap_chain_image_views_.size());
		// �eImageView�ɑΉ�����t���[���o�b�t�@���쐬����B
		for (size_t i = 0; i < swap_chain_image_views_.size(); i++) {
			VkImageView attachments[] = {swap_chain_image_views_[i]};
			VkFramebufferCreateInfo frame_buffer_info{};
			frame_buffer_info.renderPass = render_pass_;
			frame_buffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			frame_buffer_info.attachmentCount = 1;
			frame_buffer_info.pAttachments = attachments;
			frame_buffer_info.width = swap_chain_extent_.width;
			frame_buffer_info.height = swap_chain_extent_.height;
			frame_buffer_info.layers = 1;

			if (vkCreateFramebuffer(device_, &frame_buffer_info, nullptr, &swap_chain_frame_buffers_[i]) != VK_SUCCESS) {
				throw std::runtime_error("�t���[���o�b�t�@�̐����Ɏ��s���܂����I");
			}
		}
	}

	void VulkanApplication::CreateCommandPool() {
		QueueFamilyIndices queue_family_indices = FindQueueFamilies(physical_device_);

		// �`��p�̃R�}���h�v�[�����쐬
		VkCommandPoolCreateInfo pool_info{};
		pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		pool_info.queueFamilyIndex = queue_family_indices.graphics_family_.value();

		if (vkCreateCommandPool(device_, &pool_info, nullptr, &command_pool_) != VK_SUCCESS) {
			throw std::runtime_error("�R�}���h�v�[���̐����Ɏ��s���܂����I");
		}
		// �]���Ȃǂ̒Z���ȗv�f���������߂̃R�}���h�v�[�����쐬
		pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		if (vkCreateCommandPool(device_, &pool_info, nullptr, &transfer_command_pool_) != VK_SUCCESS) {
			throw std::runtime_error("�]���R�}���h�v�[���̐����Ɏ��s���܂����I");
		}
	}
	void VulkanApplication::CreateCommandBuffers() {
		command_buffers_.resize(kMaxFramesInFlight);
		VkCommandBufferAllocateInfo allocate_info{};
		allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocate_info.commandPool = command_pool_;
		allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocate_info.commandBufferCount = static_cast<uint32_t>(command_buffers_.size());

		if (vkAllocateCommandBuffers(device_, &allocate_info, command_buffers_.data()) != VK_SUCCESS) {
			throw std::runtime_error("�R�}���h�o�b�t�@�̊��蓖�ĂɎ��s���܂����I");
		}
	}

	void VulkanApplication::RecordCommandBuffer(
		VkCommandBuffer command_buffer,
		uint32_t image_index
	) {
		// �R�}���h�̋L�^���J�n����
		VkCommandBufferBeginInfo begin_info{};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = 0;
		begin_info.pInheritanceInfo = nullptr;

		if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS) {
			throw std::runtime_error("�R�}���h�̋L�^�J�n�Ɏ��s���܂����I");
		}

		// �����_�[�p�X���J�n����B
		VkRenderPassBeginInfo render_pass_info{};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_info.renderPass = render_pass_;
		render_pass_info.framebuffer = swap_chain_frame_buffers_[image_index];
		render_pass_info.renderArea.offset = { 0, 0 };
		render_pass_info.renderArea.extent = swap_chain_extent_;
		VkClearValue clear_color = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
		render_pass_info.clearValueCount = 1;
		render_pass_info.pClearValues = &clear_color;
		vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

		// �`��Ɏg���O���t�B�b�N�X�p�C�v���C�����w�肷��B
		vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline_);

		VkBuffer vertexBuffers[] = { vertex_buffer_ };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(command_buffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(command_buffer, index_buffer_, 0, VK_INDEX_TYPE_UINT16);	// ����vector��uint16_t�Ȃ̂ł���ɍ��킹��
		// �r���[�|�[�g�ƃV�U�[��`��ݒ肷��
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(swap_chain_extent_.width);
		viewport.height = static_cast<float>(swap_chain_extent_.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(command_buffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = swap_chain_extent_;
		vkCmdSetScissor(command_buffer, 0, 1, &scissor);

		// �`��R�}���h�𔭍s����B
		vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

		// �����_�[�p�X�����
		vkCmdEndRenderPass(command_buffer);
		// �R�}���h�o�b�t�@�̋L�^���I������B
		// �R�}���h�ɃG���[���������ꍇ�͂����őΉ�����B
		if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
			throw std::runtime_error("�R�}���h�o�b�t�@�ւ̃R�}���h�̏������݂Ɏ��s���܂����I");
		}
	}

	void VulkanApplication::CreateSyncObjects() {
		image_available_semaphores_.resize(kMaxFramesInFlight);
		render_finished_semaphores_.resize(kMaxFramesInFlight);
		in_flight_fences_.resize(kMaxFramesInFlight);

		VkSemaphoreCreateInfo semaphore_info{};
		semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fence_info{};
		fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		// �ŏ��̃t���[���͑����ɃV�O�i���𑗂��Ăق����̂ŃV�O�i���������ԂŐ�������
		fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		
		// �����쐬����B
		for (int index = 0; index < kMaxFramesInFlight; index++) {
			if (vkCreateSemaphore(device_, &semaphore_info, nullptr, &image_available_semaphores_[index]) != VK_SUCCESS ||
				vkCreateSemaphore(device_, &semaphore_info, nullptr, &render_finished_semaphores_[index]) != VK_SUCCESS ||
				vkCreateFence(device_, &fence_info, nullptr, &in_flight_fences_[index]) != VK_SUCCESS) {
				throw std::runtime_error("�����I�u�W�F�N�g�̐����Ɏ��s���܂����I");
			}
		}
	}
	void VulkanApplication::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& buffer_memory)
	{
		VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = size;
		bufferCreateInfo.usage = usage;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		if (vkCreateBuffer(device_, &bufferCreateInfo, nullptr, &buffer) != VK_SUCCESS)
		{
			throw std::runtime_error("���_�o�b�t�@�̍쐬�Ɏ��s���܂���");
		}
		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device_, buffer, &memRequirements);

		VkMemoryAllocateInfo allocateInfo{};
		allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocateInfo.allocationSize = memRequirements.size;
		allocateInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);
		if (vkAllocateMemory(device_, &allocateInfo, nullptr, &buffer_memory) != VK_SUCCESS) {
			throw std::runtime_error("VertexBuffer�p�̃������m�ۂɎ��s���܂���");
		}
		// �l�Ԗڂ̈����̓������̈���̃I�t�Z�b�g�B�����̗p�r�Ŏg���ꍇ��alignment�Ŋ���؂��l�ŃI�t�Z�b�g��ݒ肷��
		vkBindBufferMemory(device_, buffer, buffer_memory, 0);
	}

	void VulkanApplication::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
	{
		// �]���p�̃R�}���h�o�b�t�@���m��
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = transfer_command_pool_;
		allocInfo.commandBufferCount = 1;
		// �]�����ɂ����g��Ȃ��̂Ń��[�J���ϐ�
		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(device_, &allocInfo, &commandBuffer);

		// �����ɃR�}���h�̋L�^���n�߂�
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(commandBuffer, &beginInfo);
		// �R�s�[�R�}���h���L�^
		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0; // srcBuffer���̃R�s�[���I�t�Z�b�g
		copyRegion.dstOffset = 0; // dstBuffer���̃R�s�[��I�t�Z�b�g
		copyRegion.size = size;  // �R�s�[����o�C�g��
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
		// �L�^�I��
		vkEndCommandBuffer(commandBuffer);
		// �L�^�����R�}���h���L���[�ɑ����ăR�s�[�����s
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;
		vkQueueSubmit(graphics_queue_, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphics_queue_);
		// �����Ƀt�F���X�����Ă��悢
		// �R�}���h�o�b�t�@���J��
		vkFreeCommandBuffers(device_, transfer_command_pool_, 1, &commandBuffer);

	}

	void VulkanApplication::CreateVertexBuffer() {
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

		// Staging Buffer��p��
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,												// �]�����Ƃ��Ďg�����Ƃ��w��
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,		// CPU����A�N�Z�X�\����GPU�Ƃ̓����������o�b�t�@
			stagingBuffer,
			stagingBufferMemory);
		//Staging Buffer�Ƀf�[�^���R�s�[
		void* data;
		vkMapMemory(device_, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vertices.data(), (size_t)bufferSize);
		vkUnmapMemory(device_, stagingBufferMemory);


		CreateBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,									 // GPU����̂݃A�N�Z�X�\�ȃo�b�t�@�Ƃ��Đ�������
			vertex_buffer_,
			vertex_buffer_memory_
		);

		// vertexbuffer�ɂ�vkMapMemory�o���Ȃ��̂�StagingBuffer���o�R���ăR�s�[����
		CopyBuffer(stagingBuffer, vertex_buffer_, bufferSize);
		// Staging Buffer��j��
		vkDestroyBuffer(device_, stagingBuffer, nullptr);
		vkFreeMemory(device_, stagingBufferMemory, nullptr);
	}

	void VulkanApplication::CreateIndexBuffer()
	{
		VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

		// Staging Buffer��p��
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,												// �]�����Ƃ��Ďg�����Ƃ��w��
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,		// CPU����A�N�Z�X�\����GPU�Ƃ̓����������o�b�t�@
			stagingBuffer,
			stagingBufferMemory);
		//Staging Buffer�Ƀf�[�^���R�s�[
		void* data;
		vkMapMemory(device_, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices.data(), (size_t)bufferSize);
		vkUnmapMemory(device_, stagingBufferMemory);


		CreateBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,									 // GPU����̂݃A�N�Z�X�\�ȃo�b�t�@�Ƃ��Đ�������
			index_buffer_,
			index_buffer_memory_
		);

		// indexbuffer�ɂ�vkMapMemory�o���Ȃ��̂�StagingBuffer���o�R���ăR�s�[����
		CopyBuffer(stagingBuffer, index_buffer_, bufferSize);
		// Staging Buffer��j��
		vkDestroyBuffer(device_, stagingBuffer, nullptr);
		vkFreeMemory(device_, stagingBufferMemory, nullptr);
	}

	uint32_t VulkanApplication::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physical_device_, &memProperties);
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties)) {
				return i;
			}
		}
		throw std::runtime_error("�K�؂ȃ������^�C�v�����ł��܂���ł���");
	}

	void VulkanApplication::RecreateSwapChain() {
		// �E�B���h�E�̍ŏ����Ή�
		// �E�B���h�E���ŏ������ꂽ�Ƃ��͍ĂѓW�J�����܂őҋ@����
		int width = 0, height = 0;
		glfwGetFramebufferSize(window_, &width, &height);
		while (width == 0 || height == 0) {
			glfwGetFramebufferSize(window_, &width, &height);
			glfwWaitEvents();
		}

		// �Â��X���b�v�`�F�[�����ꎞ�I�ɕێ����Ă���
		old_swap_chain_ = swap_chain_;
		// �V�����X���b�v�`�F�[�������B
		CreateSwapChain();
		// �V�����X���b�v�`�F�[����������̂ŁA�Â��X���b�v�`�F�[���͔j������
		if (old_swap_chain_ != VK_NULL_HANDLE) {
			vkDestroySwapchainKHR(device_, old_swap_chain_, nullptr);
		}
		vkDeviceWaitIdle(device_);
		// �Â��X���b�v�`�F�[���Ɉˑ����Ă���I�u�W�F�N�g��j������
		CleanUpSwapChainDependents();
		// �ˑ��I�u�W�F�N�g���Đ�������
		CreateImageViews();
		CreateRenderPass();
		CreateGraphicsPipeline();
		CreateFramebuffers();
		CreateCommandBuffers();
	}
}