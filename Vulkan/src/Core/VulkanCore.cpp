#include "Core/VulkanCore.h"

// Vulkan�̊e��֐���\���́A�񋓌^��񋟂��Ă����w�b�_�[
#include <vulkan/vulkan.h>

namespace Core {
	void VulkanApplication::initWindow() {
		glfwInit();
		// GLFW��OpenGL��context����邽�߂ɐ݌v����Ă��邽�߁A�܂�����𐧌䂷��K�v������B
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		// �E�B���h�E�̃��T�C�Y�͓��ʂȑΉ����K�v�ɂȂ邽�߂�������ł��Ȃ��悤�ɂ���
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		// �l�ڂ̃p�����[�^�͉�ʂ��J�����j�^�[���w��ł��A�܂ڂ̃p�����[�^��OpenGL�ȊO�ł͕K�v�Ȃ�
		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	}

	void VulkanApplication::initVulkan() {

	}
	void VulkanApplication::mainLoop() {
		// �G���[���o��܂ł̓E�B���h�E�ɑ΂���C�x���g���ώ@��������
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
		}
	}

	void VulkanApplication::cleanUp() {
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void VulkanApplication::run() {
		initWindow();
		initVulkan();
		mainLoop();
		cleanUp();
	}
}