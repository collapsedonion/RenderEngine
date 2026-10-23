//
// Created by Роман  Тимофеев on 18.08.2026.
//

#ifdef __APPLE__
#include <functional>
#include <format>
#endif

#include "glm/matrix.hpp"
#include <glm/ext/matrix_transform.hpp>

import RenderManager;
import resource_lib;
import RenderEngineEnumeration;

using namespace RenderEngine;

int main() {
    RenderManager::init(
        "../shaders/compiled/engine_geometry_pass.spv",
        "../shaders/compiled/engine_light_pass.spv"
    );

    RenderManager::add_material(
        "../shaders/compiled/engine_default_materials.spv",
        "DEFAULT"
    );

    auto gradient = RenderManager::compute_material(
        "DEFAULT",
        "check_board_place_holder",
        400,
        400
    );

    RenderManager::add_render_target(
        "main_rt",
        1920,
        1080
    );

    ResourceLib::load_resources_from_json(
        "../engine_res.json"
    );

    RenderManager::add_scene(
        "main_scene"
    );

    RenderManager::set_scene_fov("main_scene", 90);

    for (uint32_t i = 0; i < 10; i++) {
        std::string name = std::format("teapot_{}", i);

        RenderManager::add_render_object_to_scene(
            "main_scene",
            name,
            i % 2 == 0 ? "teapot_model_0" : "pig_0"
        );

        RenderManager::set_render_objects_color(
            "main_scene",
            name,
            glm::vec3(0, 0.5, 0.5)
        );

        if (i % 2 == 0) {
            RenderManager::set_render_objects_texture(
                "main_scene",
                name,
                ResourceLib::get_loaded_texture("bricks_text")
            );
        } else {
            RenderManager::set_render_objects_texture(
                "main_scene",
                name,
                gradient
            );
        }

        RenderManager::set_render_objects_coloring_type(
            "main_scene",
            name,
            ColoringType::TEXTURED
        );
    }

    RenderManager::set_scene_aspect_source(
        "main_scene",
        SceneResolutionSource::WINDOW
    );

    float counter = 0.0;

    auto translate_mat = glm::translate(
        glm::mat4x4(1.0f),
        glm::vec3(0.0f, 1.0f, -10.0f)
    );

    auto rotate_mat = glm::rotate(
        glm::mat4x4(1.0f),
        glm::radians(90.0f),
        glm::vec3(1, 0, 0)
    );


    RenderManager::add_light_source(
        "main_scene",
        "test_dir",
        LightSourceType::DIRECTIONAL
    );

    RenderManager::add_light_source(
        "main_scene",
        "test_point",
        LightSourceType::POINT
    );


    RenderManager::set_light_source_transform(
        "main_scene",
        "test_point",
        glm::vec3(0, 0, -10.0),
        glm::vec3(0.0)
    );

    RenderManager::set_light_source_color(
        "main_scene",
        "test_dir",
        glm::vec3(0, 0, 1.0)
    );

    RenderManager::set_light_source_color(
        "main_scene",
        "test_point",
        glm::vec3(1.0, 0.0, 0.0)
    );

    RenderManager::start_loop(
        [=, &counter](double dt) {
            counter += static_cast<float>(dt);

            for (uint32_t i = 0; i < 10; i++) {
                std::string name = std::format("teapot_{}", i);

                auto transform =
                        glm::translate(
                            glm::mat4x4(1.0f),
                            glm::vec3(-4.0 + (i % 2) * 8.0f, 5.0 * sin(counter + i), -6.0 * (i / 2))) *
                        translate_mat *
                        glm::rotate(glm::mat4x4(1.0), counter, glm::vec3(0, 1, 0)) *
                        rotate_mat;

                RenderManager::set_render_objects_transform(
                    "main_scene",
                    name,
                    transform
                );
            }

            RenderManager::render_scene_to_target(
                "main_rt",
                "main_scene"
            );

            RenderManager::bake_light_to_target(
                "main_rt",
                "main_scene"
            );

            RenderManager::display_render_target(
                "main_rt",
                RenderTargetChannel::LIGHT_MAP
            );
            return false;
        }
    );

    RenderManager::stop();

    return 0;
}
