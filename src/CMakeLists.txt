file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS 
     "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
     "${CMAKE_CURRENT_SOURCE_DIR}/inc/*.h"
)

add_executable(engine "${SOURCE_FILES}")

target_compile_definitions(engine PUBLIC GLM_FORCE_DEPTH_ZERO_TO_ONE)
target_include_directories(engine PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/inc/")
target_link_libraries(engine PUBLIC VulkanMemoryAllocator glm Vulkan::Vulkan fmt::fmt stb SDL2::SDL2 vk-bootstrap imgui fastgltf::fastgltf)

target_precompile_headers(engine PUBLIC <optional> <vector> <memory> <string> <vector> <unordered_map> <glm/mat4x4.hpp>  <glm/vec4.hpp> <vulkan/vulkan.h>)

# Linux build breaks with this command, executable still works though
if(WIN32)
    add_custom_command(
        TARGET engine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:engine> $<TARGET_FILE_DIR:engine>
        COMMAND_EXPAND_LISTS
    )
endif()
