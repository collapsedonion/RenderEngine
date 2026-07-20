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

FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.12.0
)
FetchContent_MakeAvailable(json)

project(RenderEngineLIB)

find_package(Vulkan REQUIRED COMPONENTS SPIRV-Tools)

add_library(
        RenderEngineLib SHARED
        src/render_engine.cpp
        src/vma_impl.cpp
        src/stb_image_impl.cpp
        headers/lib/simple_draw.h
)

target_sources(
        RenderEngineLib
        PRIVATE
        FILE_SET CXX_MODULES
        FILES
            src/dispatchers.cppm
            src/synchronization.cppm
            src/command_encoders.cppm
            src/render_engine_shares.cppm
            src/descriptor_pool.cppm
            src/image.cppm
            src/shader_module.cppm
            src/spirv_tools.cppm
            src/storage_buffer.cppm
        FILE_SET HEADERS
        FILES
            headers/lib/simple_draw.h
            headers/lib/spirv_tools.h
            headers/lib/re_typedefs.h
            headers/lib/render_engine.h
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
        src/resource_lib/json_loader.cpp
)

target_include_directories(
        ResourceLib
        PUBLIC
        ${CMAKE_SOURCE_DIR}/headers/resource_lib
)

target_link_libraries(
        ResourceLib
        RenderEngineLib
        nlohmann_json::nlohmann_json
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

project(RenderEngineSimple)

add_executable(
        RenderEngineSimple
        simple_draw_main.cpp
)

target_include_directories(
        RenderEngineSimple
        PRIVATE
)

target_link_libraries(
        RenderEngineSimple

        glm::glm
        glfw
        RenderEngineLib
        ResourceLib
)
