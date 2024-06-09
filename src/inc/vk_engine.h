// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vk_types.h>

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
};
