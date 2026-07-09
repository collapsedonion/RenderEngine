#define GLFW_INCLUDE_VULKAN
#include <chrono>
#include <print>
#include <GLFW/glfw3.h>
#include <render_engine.h>
#include <spirv_tools.h>
#include <vulkan/vulkan.hpp>
#include <ranges>
#include <thread>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <numbers>
#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>

#include "resource_lib.h"
#include "glm/fwd.hpp"


const std::string json_path = "resources.json";

const uint32_t teapot_count = 100;
const uint32_t teapot_per_row = 5;

RE_pBuffer teapot_position_matricies[teapot_count] = {};
RE_pDescriptorSet teapot_resources_sets[teapot_count] = {};
RE_RenderObject teapot_render_objects[teapot_count] = {};
glm::vec3 teapot_rotation_directions[teapot_count] = {};

int main()
{
    glfwInit();
    glfwInitVulkanLoader(vkGetInstanceProcAddr);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    //  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    auto window =
        glfwCreateWindow(640, 480, "Render Engine", nullptr, nullptr);

    init_render_engine(window);
    rm_init_resource_manager();

    auto* shader_code = re_load_spirv_code(
        "shaders/compiled/shaders.spv"
    );

    auto* module = re_create_shader_module(shader_code);

    re_register_render_pipeline(
        module,
        "test_gp",
        "vertex_shader",
        "fragment_shader",
        true
    );

    RE_pDescriptorPool global_descriptor_pool = re_create_descriptor_pool(
        module, 2 + teapot_count
    );

    RE_pDescriptorSet descriptorSet{};
    uint32_t set_index = 0;

    rm_load_resources_from_json(
        json_path.c_str()
    );

    re_create_descriptor_sets(global_descriptor_pool, 1, &set_index, &descriptorSet);

    uint32_t model_count = rm_get_loaded_mesh_count("teapot_model");

    const char* rw_texture_name = "image";
    const char* sampled_texture_name = "sampled_image";
    const char* matrix_buffer_name = "matrix";

    RE_pImage depth_buffer = re_create_image(
        1024,
        1024,
        RE_IMAGE_FORMAT_DEPTH,
        false,
        true,
        true
    );

    RE_pImage render_image = re_create_image(
        1024,
        1024,
        RE_IMAGE_FORMAT_RGBA8,
        false,
        true,
        true
    );

    RE_pImage brick_wall_texture = rm_get_loaded_texture(
        "bricks_texture"
    );

    //Write display function source texture
    re_write_set_images(
        descriptorSet,
        1,
        &sampled_texture_name,
        &render_image
    );

    auto teapot_vertex_buffer = rm_get_loaded_mesh("teapot_model_0");

    //init teapots data
    for (uint32_t i = 0; i < teapot_count; i++)
    {
        re_create_descriptor_sets(global_descriptor_pool, 1, &set_index, &teapot_resources_sets[i]);
        teapot_position_matricies[i] = re_create_buffer(
            sizeof(glm::fmat4),
            true,
            true);
        teapot_render_objects[i].vertex_buffer = teapot_vertex_buffer;
        teapot_render_objects[i].descriptor_set_count = 1;
        teapot_render_objects[i].descriptor_sets = &teapot_resources_sets[i];

        teapot_rotation_directions[i] = glm::vec3((float)rand() / (float)RAND_MAX, (float)rand() / (float)RAND_MAX,
                                                  (float)rand() / (float)RAND_MAX);

        re_write_set_buffers(
            teapot_resources_sets[i],
            1,
            &matrix_buffer_name,
            &teapot_position_matricies[i]
        );

        re_write_set_images(
            teapot_resources_sets[i],
            1,
            &sampled_texture_name,
            &brick_wall_texture
        );
    }

    float teapot_y_displacement = 0.0f;

    auto previous_time_point = std::chrono::high_resolution_clock::now();
    teapot_y_displacement = 0;

    auto scaling_matrix = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));
    float begin_offset = -2.0f - teapot_per_row;

    re_wait_device_free();

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        auto present_image = re_get_present_image();

        uint32_t present_width = 0;
        uint32_t present_height = 0;

        re_get_image_dimensions(present_image, &present_width, &present_height);

        glm::mat4 perspective_matrix = glm::perspective(
            glm::radians(45.0f),
            (float)present_width / (float)present_height,
            0.1f,
            100.0f
        );

        {
            for (uint32_t i = 0; i < teapot_count; i++)
            {
                auto* mat = reinterpret_cast<glm::mat4*>(re_map_buffer(teapot_position_matricies[i]));

                *mat =
                    perspective_matrix
                    *
                    glm::translate(glm::mat4(1.0f),
                                   glm::vec3(
                                       begin_offset + 4.0 * (i % teapot_per_row),
                                       sin(teapot_y_displacement * (std::numbers::pi / 1.0f) - (i / teapot_per_row) * (
                                           std::numbers::pi / 4.0f)),
                                       -10.0f - (i / teapot_per_row) * 2.0f))
                    *

                    glm::rotate(glm::mat4(1.0f), teapot_y_displacement, teapot_rotation_directions[i])
                    *
                    scaling_matrix;


                re_unmap_buffer(teapot_position_matricies[i]);
            }
        }

        {
            re_wait_device_free();
            re_write_set_images(
                descriptorSet,
                1,
                &rw_texture_name,
                &present_image
            );
        }

        re_render(
            "test_gp",
            module,
            1,
            &render_image,
            teapot_count,
            teapot_render_objects,
            &depth_buffer
        );

        re_dispatch_compute_shader(
            "texture_test",
            module,
            1,
            &descriptorSet,
            present_width / 16,
            present_height / 16,
            1
        );

        re_present_image(present_image);

        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsed_seconds = now - previous_time_point;

        teapot_y_displacement += elapsed_seconds.count();
        previous_time_point = now;
    }

    re_wait_device_free();


    re_free_descriptor_sets(
        1,
        &descriptorSet
    );

    for (uint32_t i = 0; i < teapot_count; i++)
    {
        re_free_buffer(teapot_position_matricies[i]);
    }

    rm_free_resource_manager();
    re_free_image(render_image);
    re_free_descriptor_pool(global_descriptor_pool);
    re_free_shader_module(module);
    re_free_spirv_code(shader_code);

    re_free_render_engine();

    glfwTerminate();
    return 0;
}
