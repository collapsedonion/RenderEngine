project(RenderEngineLIB)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if( APPLE AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU" )
    # GCC defines the Clang-compat builtin __has_feature(), and CMake passes
    # -fmodules-ts to any target using FILE_SET CXX_MODULES. That combination
    # makes __has_feature(modules) evaluate to true for GCC, which is wrong -
    # it's meant to detect Clang's Objective-C/module-map modules, not the
    # C++ Modules TS. Both Apple's SDK headers (e.g. sys/_types/_rsize_t.h)
    # and GCC's own bundled <stddef.h> branch on "defined(__has_feature) &&
    # __has_feature(modules)" to pick a Clang-modules-specific code path that
    # only works with Clang's own <stddef.h>, breaking types like rsize_t
    # when compiled with GCC. Plain -U__has_feature isn't enough: it leaves
    # __has_feature(modules) as a bare, unparenthesized identifier call that
    # fails to preprocess at all in those same headers. Redefining it as a
    # function-like macro that always expands to 0 keeps it well-formed while
    # forcing the check to take the non-Clang-modules path everywhere.
    add_compile_options("-D__has_feature(x)=0")
endif()

find_package(glfw3 3.4 QUIET)

if( NOT glfw3_FOUND )
    FetchContent_Declare(
            glfw
            GIT_REPOSITORY https://github.com/glfw/glfw.git
            GIT_TAG master
    )

    FetchContent_MakeAvailable(glfw)

    #set(BUILD_SHARED_LIBS OFF)
endif()


if( LINUX )
    set(CMAKE_CXX_MODULE_STD ON)
endif()

find_package(Vulkan 1.4.350 REQUIRED COMPONENTS SPIRV-Tools)

if( LINUX )
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
endif()

add_library(
        RenderEngineLib SHARED
        src/render_engine.cpp
        src/vma_impl.cpp
        src/stb_image_impl.cpp
        src/command_encoders.cpp
        src/transporters.cpp
        src/descriptor_pool.cpp
        src/image.cpp
        src/synchronization.cpp
        src/storage_buffer.cpp
        src/spirv_tools.cpp
        src/shader_module.cpp
        #src/re_c_interface.cpp
        src/dispatchers.cpp
        src/transporters.cpp
        src/render_engine_shares.cpp
)

target_sources(
        RenderEngineLib
        PRIVATE
        FILE_SET private_module
        TYPE CXX_MODULES
        FILES
        src/synchronization.cppm
        src/command_encoders.cppm
        src/render_engine_shares.cppm
        src/spirv_tools.cppm
        FILE_SET private_headers
        TYPE HEADERS
        FILES
        headers/lib/spirv_tools.h
        headers/lib/re_typedefs.h
)

target_sources(
        RenderEngineLib
        PUBLIC
        FILE_SET public_modules
        TYPE CXX_MODULES
        FILES
        headers/lib/dynamic_dispatchable_iterator.cppm
        headers/lib/render_engine.cppm
        headers/lib/Buffer.cppm
        headers/lib/Image.cppm
        headers/lib/DescriptorPool.cppm
        headers/lib/Resource.cppm
        headers/lib/ShaderModule.cppm
        headers/lib/RenderObject.cppm
        headers/lib/Transporters.cppm
        headers/lib/Dispatchers.cppm
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

if( LINUX )
    target_link_libraries(
            RenderEngineLib
            Vulkan::HppModule
    )
endif()

if( APPLE )
    target_link_libraries(
            RenderEngineLib
            "-framework Cocoa"
            "-framework IOKit"
            "-framework CoreVideo"
    )
endif()