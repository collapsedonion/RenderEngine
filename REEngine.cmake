project(RenderEngine_Engine)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(REE_SOURCE "./src/engine/")
set(REE_MODULES "./headers/engine")
set(SHADER_SRC ${CMAKE_SOURCE_DIR}/shaders)
set(SHADER_DST ${CMAKE_SOURCE_DIR}/shaders/compiled)

add_library(
        RenderEngine_Engine
        SHARED
        ${REE_SOURCE}/transform.cpp
        ${REE_SOURCE}/render_manager.cpp
        ${REE_SOURCE}/growing_descriptor_pool.cpp
        ${REE_SOURCE}/scene_data.cpp
)

file(GLOB REE_MODULES_FILES ${REE_MODULES}/*.cppm)
file(GLOB REE_PRIVATE_MODULE_FILES ${REE_SOURCE}/*.cppm)

target_sources(
        RenderEngine_Engine
        PUBLIC
        FILE_SET ree_public_modules
        TYPE CXX_MODULES
        FILES
        ${REE_MODULES_FILES}
)


target_sources(
        RenderEngine_Engine
        PRIVATE
        FILE_SET ree_private_modules
        TYPE CXX_MODULES
        FILES
        ${REE_PRIVATE_MODULE_FILES}
)

target_compile_definitions(
        RenderEngine_Engine
        PRIVATE
        GLM_FORCE_DEPTH_ZERO_TO_ONE
        GLM_ENABLE_EXPERIMENTAL
        GLFW_INCLUDE_VULKAN
)

target_include_directories(
        RenderEngine_Engine
        PUBLIC
        ${REE_MODULES}
)

target_link_libraries(
        RenderEngine_Engine
        PRIVATE
        RenderEngineLib
        ResourceLib
        glm::glm
)

set(SHADERS engine_default_materials engine_geometry_pass engine_light_pass)

foreach(SHADER ${SHADERS})
    add_custom_command(
            OUTPUT ${SHADER_DST}/${SHADER}.spv
            DEPENDS ${SHADER_SRC}/${SHADER}.slang
            COMMAND slangc ${SHADER_SRC}/${SHADER}.slang -fvk-use-entrypoint-name -o ${SHADER_DST}/${SHADER}.spv
    )

    list(APPEND SHADER_SPVS ${SHADER_DST}/${SHADER}.spv)
endforeach ()

add_custom_target(
        Engine_Shaders
        ALL
        DEPENDS ${SHADER_SPVS}
)

add_dependencies(
        RenderEngine_Engine
        Engine_Shaders
)