cmake_minimum_required(VERSION 4.4)

if( NOT CMAKE_EXPERIMENTAL_CXX_IMPORT_STD )
    if( ${CMAKE_VERSION} VERSION_GREATER_EQUAL 4.4 )
        set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD f35a9ac6-8463-4d38-8eec-5d6008153e7d)
    elseif( ${CMAKE_VERSION} VERSION_GREATER_EQUAL 4.3 )
        set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD 451f2fe2-a8a2-47c3-bc32-94786d8fc91b)
    elseif( ${CMAKE_VERSION} VERSION_GREATER_EQUAL 4.2 )
        set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD d0edc3af-4c50-42ea-a356-e2862fe7a444)
    elseif( ${CMAKE_VERSION} VERSION_GREATER_EQUAL 4.0 )
        set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD 1942b4fa-b2c5-4546-9385-83f254070067)
	elseif( ${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.30 )
        set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD 0e5b6991-d74f-4b3d-a41c-cf096e0b2508)
    endif()
endif()

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

#set(BUILD_SHARED_LIBS OFF)

FetchContent_Declare(
        VulkanMemoryAllocator
        GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
        GIT_TAG master
)

FetchContent_MakeAvailable(VulkanMemoryAllocator)

FetchContent_Declare(
        spirv-reflect
        GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Reflect.git
        GIT_TAG vulkan-sdk-1.4.350.0
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
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_MODULE_STD ON)

find_package(Vulkan 1.4.350 REQUIRED COMPONENTS SPIRV-Tools)

if( NOT TARGET Vulkan::HppModule )
    add_library( Vulkan-HppModule OBJECT )
    add_library( Vulkan::HppModule ALIAS Vulkan-HppModule )
    target_sources( Vulkan-HppModule PUBLIC
        FILE_SET CXX_MODULES
        BASE_DIRS ${Vulkan_INCLUDE_DIR}
        FILES
            ${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm
            ${Vulkan_INCLUDE_DIR}/vulkan/vulkan_video.cppm)
    target_compile_features( Vulkan-HppModule PUBLIC cxx_std_23 )
    set_target_properties( Vulkan-HppModule PROPERTIES CXX_MODULE_STD ON )
    set_target_properties( Vulkan-HppModule PROPERTIES CMAKE_POSITION_INDEPENDENT_CODE ON)
    target_include_directories( Vulkan-HppModule PUBLIC ${Vulkan_INCLUDE_DIR} )
endif()

add_library(
        RenderEngineLib SHARED
        src/render_engine.cpp
        src/vma_impl.cpp
        src/stb_image_impl.cpp
        src/command_encoders.cpp
        src/dispatchers.cpp
        src/descriptor_pool.cpp
        src/image.cpp
        src/synchronization.cpp
        src/storage_buffer.cpp
        src/spirv_tools.cpp
        src/shader_module.cpp
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
        Vulkan::HppModule
        Vulkan::SPIRV-Tools
        vk-bootstrap::vk-bootstrap
)

project(SimpleDraw)

add_library(
        SimpleDraw
        SHARED
        src/simpled_draw/simple_draw.cpp
        src/simpled_draw/scene2d.cpp
)

target_sources(
        SimpleDraw
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
            headers/simple_draw/scene2d.cppm
            headers/simple_draw/simple_draw.cppm
            headers/simple_draw/shader_structures.cppm
            headers/simple_draw/setters.cppm
)

target_include_directories(
        SimpleDraw
        PUBLIC
        headers/simple_draw
)

target_link_libraries(
        SimpleDraw
        RenderEngineLib
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
        headers/simple_draw/scene2d.cppm
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
        SimpleDraw
        ResourceLib
)
