//> includes
#include "vk_engine.h"

#include <SDL.h>
#include <SDL_vulkan.h>
#include <VkBootstrap.h>

#include <vk_initializers.h>
#include <vk_types.h>
#include <vk_images.h>

#include <chrono>
#include <thread>

VulkanEngine* loaded_engine = nullptr;
constexpr bool use_validation_layers = true;

VulkanEngine& VulkanEngine::Get() { return *loaded_engine; }
void VulkanEngine::init()
{
    // only one engine initialization is allowed with the application.
    assert(loaded_engine == nullptr);
    loaded_engine = this;

    // We initialize SDL and create a window with it.
    SDL_Init(SDL_INIT_VIDEO);

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

    m_window = SDL_CreateWindow(
        "Vulkan Engine",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        m_window_extent.width,
        m_window_extent.height,
        window_flags);

    init_vulkan();
    init_swapchain();
    init_commands();
    init_sync_structures();

    // everything went fine
    m_is_initialized = true;
}

void VulkanEngine::cleanup()
{
    if (m_is_initialized) {
        vkDeviceWaitIdle(m_device);

        for (int i = 0; i < FRAME_OVERLAP; i++) {
            vkDestroyCommandPool(m_device, m_frames[i].command_pool, nullptr);

            vkDestroyFence(m_device, m_frames[i].render_fence, nullptr);
            vkDestroySemaphore(m_device, m_frames[i].render_semaphore, nullptr);
            vkDestroySemaphore(m_device, m_frames[i].swapchain_semaphore, nullptr);
        }

        destroy_swapchain();

        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        vkDestroyDevice(m_device, nullptr);

        vkb::destroy_debug_utils_messenger(m_instance, m_debug_messenger);
        vkDestroyInstance(m_instance, nullptr);

        SDL_DestroyWindow(m_window);
    }

    // clear engine pointer
    loaded_engine = nullptr;
}

void VulkanEngine::draw()
{
    VK_CHECK(vkWaitForFences(m_device, 1, &get_current_frame().render_fence, true, 1000000000));
    VK_CHECK(vkResetFences(m_device, 1, &get_current_frame().render_fence));

    uint32_t swapchain_image_index;
    VK_CHECK(vkAcquireNextImageKHR(m_device, m_swapchain, 1000000000, get_current_frame().swapchain_semaphore, nullptr, &swapchain_image_index));

    VkCommandBuffer cmd = get_current_frame().main_command_buffer;
    VK_CHECK(vkResetCommandBuffer(cmd, 0));
    
    VkCommandBufferBeginInfo cmd_begin_info = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmd_begin_info));

    vkutil::transition_image(cmd, m_swapchain_images[swapchain_image_index], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    VkClearColorValue clear_value;
    float flash = std::abs(std::sin(m_frame_number / 120.0f));
    clear_value = { { 0.0f, 0.0f, flash, 1.0f } };

    VkImageSubresourceRange clear_range = vkinit::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);

    vkCmdClearColorImage(cmd, m_swapchain_images[swapchain_image_index], VK_IMAGE_LAYOUT_GENERAL, &clear_value, 1, &clear_range);
    vkutil::transition_image(cmd, m_swapchain_images[swapchain_image_index], VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    VK_CHECK(vkEndCommandBuffer(cmd));

    VkCommandBufferSubmitInfo cmd_info = vkinit::command_buffer_submit_info(cmd);

    VkSemaphoreSubmitInfo wait_info = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, get_current_frame().swapchain_semaphore);
    VkSemaphoreSubmitInfo signal_info = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, get_current_frame().render_semaphore);

    VkSubmitInfo2 submit = vkinit::submit_info(&cmd_info, &signal_info, &wait_info);

    VK_CHECK(vkQueueSubmit2(m_graphics_queue, 1, &submit, get_current_frame().render_fence));

    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = nullptr;
    present_info.pSwapchains = &m_swapchain;
    present_info.swapchainCount = 1;

    present_info.pWaitSemaphores = &get_current_frame().render_semaphore;
    present_info.waitSemaphoreCount = 1;

    present_info.pImageIndices = &swapchain_image_index;

    VK_CHECK(vkQueuePresentKHR(m_graphics_queue, &present_info));

    m_frame_number++;
}

void VulkanEngine::run()
{
    SDL_Event e;
    bool quit = false;

    // main loop
    while (!quit) {
        // Handle events on queue
        while (SDL_PollEvent(&e) != 0) {
            // close the window when user alt-f4s or clicks the X button
            if (e.type == SDL_QUIT)
                quit = true;

            if (e.type == SDL_WINDOWEVENT) {
                if (e.window.event == SDL_WINDOWEVENT_MINIMIZED) {
                    m_stop_rendering = true;
                }
                if (e.window.event == SDL_WINDOWEVENT_RESTORED) {
                    m_stop_rendering = false;
                }
            }

            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) {
                    m_stop_rendering = false;
                    quit = true;
                }
            }
        }

        // do not draw if we are minimized
        if (m_stop_rendering) {
            // throttle the speed to avoid the endless spinning
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        draw();
    }
}

void VulkanEngine::init_vulkan() {
    vkb::InstanceBuilder builder;

    auto inst_ret = builder
        .set_app_name("Example Vulkan Application")
        .request_validation_layers(use_validation_layers)
        .use_default_debug_messenger()
        .require_api_version(1, 3, 0)
        .build();
    
    vkb::Instance vkb_inst = inst_ret.value();

    m_instance = vkb_inst.instance;
    m_debug_messenger = vkb_inst.debug_messenger;

    SDL_Vulkan_CreateSurface(m_window, m_instance, &m_surface);

    VkPhysicalDeviceVulkan13Features features {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES
    };
    features.dynamicRendering = true;
    features.synchronization2 = true;

    VkPhysicalDeviceVulkan12Features features12 {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES
    };
    features12.bufferDeviceAddress = true;
    features12.descriptorIndexing = true;

    vkb::PhysicalDeviceSelector selector { vkb_inst };
    vkb::PhysicalDevice physical_device = selector
        .set_minimum_version(1, 3)
        .set_required_features_13(features)
        .set_required_features_12(features12)
        .set_surface(m_surface)
        .select()
        .value();
    
    vkb::DeviceBuilder device_builder { physical_device };
    vkb::Device vkb_device = device_builder.build().value();

    m_device = vkb_device.device;
    m_chosen_gpu = physical_device.physical_device;

    m_graphics_queue = vkb_device.get_queue(vkb::QueueType::graphics).value();
    m_graphics_queue_family = vkb_device.get_queue_index(vkb::QueueType::graphics).value();
}

void VulkanEngine::init_swapchain() {
    create_swapchain(m_window_extent.width, m_window_extent.height);
}

void VulkanEngine::init_commands() {
    VkCommandPoolCreateInfo command_pool_info = vkinit::command_pool_create_info(m_graphics_queue_family, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        VK_CHECK(vkCreateCommandPool(m_device, &command_pool_info, nullptr, &m_frames[i].command_pool));

        VkCommandBufferAllocateInfo command_buffer_alloc_info = vkinit::command_buffer_allocate_info(m_frames[i].command_pool, 1);

        VK_CHECK(vkAllocateCommandBuffers(m_device, &command_buffer_alloc_info, &m_frames[i].main_command_buffer));
    }
}

void VulkanEngine::init_sync_structures() {
    VkFenceCreateInfo fence_create_info = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
    VkSemaphoreCreateInfo semaphore_create_info = vkinit::semaphore_create_info();

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        VK_CHECK(vkCreateFence(m_device, &fence_create_info, nullptr, &m_frames[i].render_fence));

        VK_CHECK(vkCreateSemaphore(m_device, &semaphore_create_info, nullptr, &m_frames[i].swapchain_semaphore));
        VK_CHECK(vkCreateSemaphore(m_device, &semaphore_create_info, nullptr, &m_frames[i].render_semaphore));
    }
}

void VulkanEngine::create_swapchain(uint32_t width, uint32_t height) {
    vkb::SwapchainBuilder swapchain_builder { m_chosen_gpu, m_device, m_surface };
    m_swapchain_image_format = VK_FORMAT_B8G8R8A8_UNORM;

    vkb::Swapchain vkb_swapchain = swapchain_builder
        .set_desired_format(VkSurfaceFormatKHR { 
            .format = m_swapchain_image_format,
            .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
        })
        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
        .set_desired_extent(width, height)
        .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        .build()
        .value();
    
    m_swapchain_extent = vkb_swapchain.extent;
    m_swapchain = vkb_swapchain.swapchain;
    m_swapchain_images = vkb_swapchain.get_images().value();
    m_swapchain_image_views = vkb_swapchain.get_image_views().value();
}

void VulkanEngine::destroy_swapchain() {
    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);

    // destroy swapchain resources
    for (int i = 0; i < m_swapchain_image_views.size(); i++) {
        vkDestroyImageView(m_device, m_swapchain_image_views[i], nullptr);
    }
}

FrameData& VulkanEngine::get_current_frame() {
    return m_frames[m_frame_number % FRAME_OVERLAP];
}