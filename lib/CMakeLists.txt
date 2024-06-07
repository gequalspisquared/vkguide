find_package(Vulkan REQUIRED)

add_library(glm INTERFACE)
add_library(VulkanMemoryAllocator INTERFACE)
add_library(stb INTERFACE)

target_include_directories(VulkanMemoryAllocator INTERFACE VulkanMemoryAllocator/include/)
target_include_directories(glm INTERFACE glm)
target_include_directories(stb INTERFACE stb)

add_subdirectory(fastgltf)
add_subdirectory(fmt EXCLUDE_FROM_ALL)
add_subdirectory(SDL EXCLUDE_FROM_ALL)

add_library(vk-bootstrap STATIC)

target_sources(vk-bootstrap PRIVATE 
    vk-bootstrap/src/VkBootstrap.h
    vk-bootstrap/src/VkBootstrap.cpp
)

target_include_directories(vk-bootstrap PUBLIC vk-bootstrap/src/)
target_link_libraries(vk-bootstrap PUBLIC Vulkan::Vulkan $<$<BOOL:UNIX>:${CMAKE_DL_LIBS}>)

add_library(imgui STATIC)

target_include_directories(imgui PUBLIC imgui)

target_sources(imgui PRIVATE 
    imgui/imgui.h
    imgui/imgui.cpp

    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_tables.cpp
    imgui/backends/imgui_impl_vulkan.cpp
    imgui/backends/imgui_impl_sdl2.cpp
)

target_link_libraries(imgui PUBLIC Vulkan::Vulkan SDL2::SDL2)