

if( CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux" )
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
endif()

set(BUILD_SHARED_LIBS ON)

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

# On the macOS SDK, <stdint.h> redefines SIZE_MAX without an #ifndef guard,
# which trips assimp's own SIZE_MAX fallback in defs.h as a "macro redefined"
# warning. assimp treats its own warnings as errors by default, which turns
# that harmless redefinition into a hard build failure.
set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)

# Arch Linux's system `minizip` package installs its headers under
# /usr/include/minizip/ (e.g. minizip/unzip.h), but assimp's
# ZipArchiveIOSystem.cpp does `#include <unzip.h>` with no subdirectory,
# and minizip's pkg-config file only exports `-I/usr/include`. That mismatch
# makes assimp fail to find unzip.h when it links against the system
# zlib/minizip. Building assimp's bundled zlib+minizip instead sidesteps the
# incompatible system layout.
set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)

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

