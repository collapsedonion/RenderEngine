cmake_minimum_required(VERSION 4.4)
include(Deps.cmake)
include(RELib.cmake)
include(REEngine.cmake)
include(ResourceLib.cmake)

project(SimpleDraw)

add_library(
        SimpleDraw
        SHARED
        src/simpled_draw/simple_draw.cpp
        src/simpled_draw/scene2d.cpp
        src/simpled_draw/scene3d.cpp
)

target_sources(
        SimpleDraw
        PUBLIC
        FILE_SET CXX_MODULES
        FILES
            headers/simple_draw/scene2d.cppm
            headers/simple_draw/scene3d.cppm
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

project(RenderEngineLibTest)

add_executable(
        RenderEngineLibTest
        main.cpp
)

target_include_directories(
        RenderEngineLibTest
        PRIVATE
)

target_link_libraries(
        RenderEngineLibTest

        glm::glm
        glfw
        RenderEngineLib
        ResourceLib
)

if( APPLE )
    target_link_libraries(
            RenderEngineLibTest
            "-framework Cocoa"
            "-framework IOKit"
            "-framework CoreVideo"
    )
endif()

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

if( APPLE )
    target_link_libraries(
            RenderEngineSimple
            "-framework Cocoa"
            "-framework IOKit"
            "-framework CoreVideo"
    )
endif()


project(RenderEngine_Engine_Test)

add_executable(
        RenderEngine_Engine_Test
        engine_main.cpp
)


target_link_libraries(
        RenderEngine_Engine_Test

        glm::glm
        glfw
        RenderEngineLib
        ResourceLib
        RenderEngine_Engine
)

if( APPLE )
    target_link_libraries(
            RenderEngine_Engine_Test
            "-framework Cocoa"
            "-framework IOKit"
            "-framework CoreVideo"
    )
endif()
