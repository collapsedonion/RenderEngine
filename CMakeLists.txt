cmake_minimum_required(VERSION 4.2)

set(CMAKE_CXX_STANDARD 26)
set(VK_BOOTSTRAP_POSITION_INDEPENDENT_CODE ON CACHE BOOL "" FORCE)

include(FetchContent)
FetchContent_Declare(
        vk_bootstrap
        GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap.git
        GIT_TAG        v1.4.350
)
FetchContent_MakeAvailable(vk_bootstrap )

FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG master
)

set(BUILD_SHARED_LIBS ON)

FetchContent_MakeAvailable(glfw)

set(BUILD_SHARED_LIBS OFF)

FetchContent_Declare(
        VulkanMemoryAllocator
        GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
        GIT_TAG master
)

FetchContent_MakeAvailable(VulkanMemoryAllocator)

FetchContent_Declare(
        spirv-reflect
        GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Reflect.git
        GIT_TAG main
)

set(SPIRV_REFLECT_STATIC_LIB ON)

FetchContent_MakeAvailable(spirv-reflect)

FetchContent_Declare(
        assimp
        GIT_REPOSITORY https://github.com/assimp/assimp.git
        GIT_TAG master
)

FetchContent_MakeAvailable(assimp)

FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG master
)

FetchContent_MakeAvailable(glm)

project(RenderEngineLIB)

find_package(Vulkan REQUIRED COMPONENTS SPIRV-Tools)

add_library(
        RenderEngineLib
        SHARED
        src/render_engine.cpp
        src/vma_impl.cpp
        src/spirv_tools.cpp
        src/shader_module.cpp
        src/synchronization.cpp
        src/command_encoders.cpp
        src/uid.h
        src/storage_buffer.cpp
        src/export_macro.h
        src/descriptor_pool.cpp
        src/dispatchers.cpp
        src/image.cpp
        src/stb_image_impl.cpp
)

target_include_directories(
        RenderEngineLib
        PUBLIC
        ${CMAKE_SOURCE_DIR}/headers/lib
)

target_link_libraries(
        RenderEngineLib
        GPUOpen::VulkanMemoryAllocator
        spirv-reflect-static
        glfw
        assimp::assimp
        Vulkan::Vulkan
        Vulkan::SPIRV-Tools
        vk-bootstrap::vk-bootstrap
)

project(ResourceLib)

add_library(
        ResourceLib
        SHARED
        headers/resource_lib/resource_lib.h
        src/resource_lib/resource_loader.cpp
)

target_include_directories(
        ResourceLib
        PUBLIC
        ${CMAKE_SOURCE_DIR}/headers/resource_lib
)

target_link_libraries(
        ResourceLib
        RenderEngineLib
)

project(RenderEngine)

add_executable(
        RenderEngine
        main.cpp
)

target_include_directories(
        RenderEngine
        PRIVATE
)

target_link_libraries(
        RenderEngine

        glm::glm
        glfw
        RenderEngineLib
        ResourceLib
)