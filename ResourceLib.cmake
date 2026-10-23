project(ResourceLib)

add_library(
        ResourceLib
        SHARED
        src/resource_lib/resource_loader.cpp
)

target_sources(
        ResourceLib
        PUBLIC
        FILE_SET modules
        TYPE CXX_MODULES
        FILES
        headers/resource_lib/resource_lib.cppm
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
