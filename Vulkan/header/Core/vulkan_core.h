#pragma once
/**
* @file VulkanCore.h
* @brief Vulkanの描画に関する基本的な処理を実装するクラスの宣言ファイル
* @author はっとり
* @date 2025/4/17
*/

#include<vector>
#include<iostream>
#include<fstream>
#include<array>

// ウィンドウ生成に関する各種機能を宣言するヘッダー
#define GLFW_INCLUDE_VULKAN 1
// Vulkanの各種関数や構造体、列挙型を提供してくれるヘッダー
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include<optional>

namespace Core
{
	/**
	* @brief
	* キューファミリーのインデックスを扱う構造体
	*/
	struct QueueFamilyIndices {
		std::optional<uint32_t> graphics_family_;
		std::optional<uint32_t> present_family_;

		/**
		* @fn
		* @brief グラフィックスファミリーの条件を満たしているかどうか
		*/
		bool IsComplete() const {
			return graphics_family_.has_value() && present_family_.has_value();
		}
	};
	/**
	* @brief
	* スワップチェインの詳細な情報を扱う構造体
	*/
	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities_;
		std::vector<VkSurfaceFormatKHR> formats_;
		std::vector<VkPresentModeKHR> present_modes_;
	};

	/**
	* @brief
	* Vulkan側で頂点データを扱うために使用する構造体。
	*/
	struct Vertex {
		glm::vec2 pos;
		glm::vec3 color;
		/**
		* @fn
		* @brief
		* 頂点データのバインディング情報を取得する。
		*/
		static VkVertexInputBindingDescription GetBindingDescription() {
			VkVertexInputBindingDescription binding_description{};
			binding_description.binding = 0;
			binding_description.stride = sizeof(Vertex);
			binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			return binding_description;
		}
		/**
		* @fn
		* @brief
		* 頂点データの属性情報を取得する。
		*/
		static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 2> attribute_descriptions{};
			// 頂点データをバインディングしているバッファ 今回は0番
			attribute_descriptions[0].binding = 0;
			// 座標データをどのlocationから頂点シェーダーに与えるか？今回は0番
			// Shader の (location = 0)のデータに与えられる
			attribute_descriptions[0].location = 0;
			// 頂点はvec2なので R32G32で指定
			attribute_descriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
			attribute_descriptions[0].offset = offsetof(Vertex, pos);

			attribute_descriptions[1].binding = 0;
			// 色データをどのlocationから頂点シェーダーに与えるか？今回は1番
			// Shader の (location = 1)のデータに与えられる
			attribute_descriptions[1].location = 1;
			attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attribute_descriptions[1].offset = offsetof(Vertex, color);
			return attribute_descriptions;
		}
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
		// 一度に処理するフレームの数
		const int kMaxFramesInFlight = 2;
#ifdef NDEBUG
		const bool kEnableValidationLayers = false;
#else
		const bool kEnableValidationLayers = true;
#endif

	public:
		/**
		* @fn
		* @brief Vulkanの描画処理を起動する。
		*/
		void Run();

	private:
		/**
		* @brief GLFWのウィンドウ設定を初期化する
		*/
		void InitWindow();

		/**
		* @brief VkInstanceを生成する。
		*/
		void CreateVkInstance();
		/**
		* @fn
		* @brief Vulkanの各種コンポーネントを初期化する。
		*/
		void InitVulkan();
		/**
		* @fn
		* @brief Vulkanの描画ループを起動する。
		*/
		void MainLoop();
		/**
		* @fn
		* @brief Vulkanで確保した各種リソースを解放する。
		*/
		void CleanUp();

		/**
		* @fn
		* @brief
		* コマンドバッファに記録されたコマンドを実行し描画を行う。
		*/
		void DrawFrame();

		/**
		* @fn
		* @brief
		* 要求されている拡張機能が全てサポートされているかをチェックする
		* @detail
		* VkInstance生成時にVkInstanceCreateInfo内で設定している必要な拡張機能がサポートされていない場合、
		* VK_ERROR_EXTENSION_NOT_PRESENTエラーコードが発生する。
		* これに未然に対応するために、vkEnumerateInstanceExtensionProperties関数でサポートされている拡張機能を取得し、
		* 必要な拡張機能がサポートされているかを事前チェックする。
		* 
		*/
		bool CheckExtensionsAvailable(std::vector<const char*> required_instance);

		/**
		* @fn
		* @brief 必要な拡張機能のリストを取得する。
		*/
		std::vector<const char*> GetRequiredExtensions();

		/**
		* @fn
		* @brief
		* バリデーションレイヤがサポートされているかどうかを確認する。
		*/
		bool CheckValidationLayerSupport();

		/**
		* @fn
		* @brief
		* デバッグメッセンジャーの生成情報を初期化する
		*/
		void InitializeDebugMessengerCreateInfo(
			VkDebugUtilsMessengerCreateInfoEXT& create_info				// デバッグメッセンジャー生成情報インスタンスへの参照
		);
		/**
		* @fn
		* @brief
		* デバッグメッセージの通知システムをセットアップする
		*/
		void SetupDebugMessenger();

		/**
		* @fn
		* @brief 
		* デバッグメッセンジャーを生成する
		*/
		VkResult CreateDebugUtilsMessenger(
			const VkDebugUtilsMessengerCreateInfoEXT* p_create_info,	// デバッグメッセンジャーの生成情報
			const VkAllocationCallbacks* p_allocator					// カスタムアロケーターへのポインタ
		);

		/**
		* @fn
		* @brief
		* デバッグメッセンジャーを破棄する
		*/
		void DestroyDebugUtilsMessenger(
			VkAllocationCallbacks* p_allocator
		);
		/**
		* @fn
		* @brief
		* Vulkanの各コマンド発行に適切なキューファミリーを探す
		*/
		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);

		/**
		* @fn
		* @brief
		* 引数に与えられたデバイスが要件を満たすかどうかを確認する
		*/
		bool IsDeviceSuitable(VkPhysicalDevice device);

		/**
		* @fn
		* @brief
		* デバッグメッセージのコールバック関数
		* @return
		* Validation Layer をトリガした関数の処理をabortすべきかどうか
		*/
		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallBack(
			VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,	 // デバッグメッセージの重要度
			VkDebugUtilsMessageTypeFlagsEXT message_type,				 // 発生したメッセージの種類
			const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data, // メッセージの詳細な内容を含む構造体
			void* p_user_data											 // コールバックのセットアップ時に与えられる独自データ
		) {
			// VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: 診断メッセージ
			// VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: リソースメッセージなどの情報提供メッセージ
			// VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT : エラーではないがバグに近い警告メッセージ
			// VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT : クラッシュを引き起こしうる不正な動きを通知するメッセージ
			// 下に行けば行くほど重要度大 (割り当てられた値も大きい)
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
		* Vulkanの描画に使用する物理デバイスを選択する
		*/
		void PickPhysicalDevice();

		/**
		* @fn
		* @brief
		* 論理デバイスを作成する。
		*/
		void CreateLogicalDevice();

		/**
		* @fn
		* @brief
		* ウィンドウサーフェスを生成する。
		*/
		void CreateSurface();

		/**
		* @fn
		* @brief
		* 物理デバイスが拡張機能をサポートしているかどうかを確認する。
		*/
		bool CheckDeviceExtensionSupport(VkPhysicalDevice device);

		/**
		* @fn
		* @brief
		* スワップチェインのサポートに関する詳細を調べる。
		*/
		SwapChainSupportDetails QuerySwapChainSupprot(VkPhysicalDevice device);

		/**
		* @fn
		* @brief
		* サポートしているフォーマットの中から適切なものを選択する。
		*/
		VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats);

		/**
		* @fn
		* @brief
		* サポートしている表示モードの中から適切なものを選択する。
		*/
		VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes);

		/**
		* @fn
		* @brief
		* スワップチェイン内の画像の解像度を適切に設定する。
		*/
		VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

		/**
		* @fn
		* @brief
		* スワップチェインを生成する。
		*/
		void CreateSwapChain();

		/**
		* @fn
		* @brief
		* スワップチェーンを再生成する。
		*/
		void RecreateSwapChain();

		/**
		* @fn
		* @brief
		* 現在のスワップチェーンを破棄する。
		*/
		void CleanUpSwapChainDependents();

		/**
		* @fn
		* @brief
		* スワップチェイン内の画像を扱うために使われるイメージビューを生成する。
		*/
		void CreateImageViews();

		/**
		* @fn
		* @brief
		* 描画コマンドを処理するグラフィックスパイプラインを生成する。
		*/
		void CreateGraphicsPipeline();

		/**
		* @fn
		* @brief
		* ファイルを読み込む
		* @todo 別ファイルに切り出す
		*/
		static std::vector<char> ReadFile(const std::string& filePath) {
			// ate : ファイルの最後から読み取りを開始する 読み取り位置を使ってファイルのサイズを決め、バッファを割り当てることができる
			// binary : ファイルをバイナリファイルとして読み取る
			std::ifstream file(filePath, std::ios::ate | std::ios::binary);
			if (!file.is_open()) {
				throw std::runtime_error("ファイルを開けませんでした");
			}
			// 文字数を文字列の最終文字の場所で得る
			size_t file_size = (size_t)file.tellg();
			std::vector<char> buffer(file_size);
			// 文字列の先頭に戻って読み込む
			file.seekg(0);
			file.read(buffer.data(), file_size);
			file.close();
			return buffer;
		}

		/**
		* @fn
		* @brief
		* シェーダーコードをラップするシェーダーモジュールを生成する
		*/
		VkShaderModule CreateShaderModule(
			const std::vector<char>& code // シェーダーコード
		);

		/**
		* @fn
		* @brief
		* レンダーパスを生成する
		*/
		void CreateRenderPass();

		/**
		* @fn
		* @brief
		* フレームバッファを生成する。
		*/
		void CreateFramebuffers();

		/**
		* @fn
		* @brief
		* コマンドプールを生成する。
		*/
		void CreateCommandPool();

		/**
		* @fn
		* @brief
		* コマンドバッファを生成する。
		*/
		void CreateCommandBuffers();

		/**
		* @fn
		* @brief
		* コマンドバッファに描画のための一連の操作を記録する。
		*/
		void RecordCommandBuffer(
			VkCommandBuffer command_buffer,		// コマンドの記録に使用するコマンドバッファ
			uint32_t image_index				// 書き込み先のスワップチェーンイメージのインデックス
		);

		/**
		* @fn
		* @brief
		* 実行同期を取るために使われるオブジェクト(セマフォ / フェンス)を生成する。
		*/
		void CreateSyncObjects();

		/**
		* @fn
		* @brief
		* バッファを生成するためのヘルパー関数
		* @param size バッファのサイズ
		* @param usage バッファの使用用途
		* @param properties バッファのメモリプロパティ
		* @param buffer 生成されたバッファのハンドル格納先
		* @param buffer_memory 生成されたバッファのメモリハンドル格納先
		*/
		void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& buffer_memory);
		
		/**
		* @fn
		* @brief
		* バッファをコピーするためのヘルパー関数
		* @param srcBuffer コピー元のバッファ
		* @param dstBuffer コピー先のバッファ
		* @param size バッファの大きさ
		*/
		void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
		/**
		* @fn
		* @brief
		* 頂点バッファを生成する
		*/
		void CreateVertexBuffer();

		/**
		* @fn
		* @brief
		* バッファに適したGPUのメモリタイプを取得する。
		*/
		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
		/**
		* @fn
		* @brief
		* ウィンドウサイズが書き換わったときのコールバック
		*/
		static void FramebufferReizeCallback(GLFWwindow* window, int width, int height) {
			// framebuffer_resized_ メンバは静的変数ではないので直接値をいじれない
			// あらかじめ設定したこのクラスのインスタンス経由で変更を加える
			auto app = reinterpret_cast<VulkanApplication*>(glfwGetWindowUserPointer(window));
			app->framebuffer_resized_ = true;
		}

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
		VkSwapchainKHR old_swap_chain_;
		std::vector<VkImage> swap_chain_images_;
		std::vector<VkImageView> swap_chain_image_views_;
		std::vector<VkFramebuffer> swap_chain_frame_buffers_;
		VkFormat swap_chain_image_format_;
		VkExtent2D swap_chain_extent_;
		VkRenderPass render_pass_;
		VkPipelineLayout pipeline_layout_;
		VkPipeline graphics_pipeline_;
		VkCommandPool command_pool_;
		VkCommandPool transfer_command_pool_;
		VkBuffer vertex_buffer_;
		VkDeviceMemory vertex_buffer_memory_;
		std::vector<VkCommandBuffer> command_buffers_;
		std::vector<VkSemaphore> image_available_semaphores_;
		std::vector<VkSemaphore> render_finished_semaphores_;
		std::vector<VkFence> in_flight_fences_;
		uint32_t current_frame_ = 0;
		bool framebuffer_resized_ = false;
	};
}