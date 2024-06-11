// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vk_types.h>

struct FrameData {
	VkCommandPool command_pool;
	VkCommandBuffer main_command_buffer;

	VkSemaphore swapchain_semaphore;
	VkSemaphore render_semaphore;
	VkFence render_fence;
};

constexpr unsigned int FRAME_OVERLAP = 2;

class VulkanEngine {
public:
	static VulkanEngine& Get();

	//initializes everything in the engine
	void init();

	//shuts down the engine
	void cleanup();

	//draw loop
	void draw();

	//run main loop
	void run();

private:
	bool m_is_initialized { false };
	int m_frame_number { 0 };
	bool m_stop_rendering { false };
	VkExtent2D m_window_extent { 1700 , 900 };
	struct SDL_Window* m_window { nullptr };

	VkInstance m_instance;
	VkDebugUtilsMessengerEXT m_debug_messenger;
	VkPhysicalDevice m_chosen_gpu;
	VkDevice m_device;
	VkSurfaceKHR m_surface;

	VkSwapchainKHR m_swapchain;
	VkFormat m_swapchain_image_format;
	std::vector<VkImage> m_swapchain_images;
	std::vector<VkImageView> m_swapchain_image_views;
	VkExtent2D m_swapchain_extent;

	FrameData m_frames[FRAME_OVERLAP];
	VkQueue m_graphics_queue;
	uint32_t m_graphics_queue_family;

	void init_vulkan();
	void init_swapchain();
	void init_commands();
	void init_sync_structures();

	void create_swapchain(uint32_t width, uint32_t height);
	void destroy_swapchain();

	FrameData& get_current_frame();
};
