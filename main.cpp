#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <spirv_tools.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include "glm/fwd.hpp"

#ifdef __APPLE__
#include <memory>
#include <span>
#include <chrono>
#include <ranges>
#endif

#ifdef __linux__
import vulkan;
import std;
#endif

import Buffer;
import Image;
import render_engine;
import RenderObject;
import DescriptorPool;
import resource_lib;
import Transporters;
import Dispatchers;
import dynamic_dispatchable_iterator;

const std::string json_path = "resources.json";

const uint32_t teapot_count = 100;
const uint32_t teapot_per_row = 5;

using namespace RenderEngine;

std::shared_ptr<RawBuffer> teapot_position_matricies[teapot_count] = {};
std::shared_ptr<DescriptorPool::DescriptorSet> teapot_resources_sets[teapot_count] = {};
std::weak_ptr<DescriptorPool::DescriptorSet> teapot_weak_ptr[teapot_count] = {};
RenderObject teapot_render_objects[teapot_count] = {};
glm::vec3 teapot_rotation_directions[teapot_count] = {};

int main()
{
    glfwInit();
    glfwInitVulkanLoader(vkGetInstanceProcAddr);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    //  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    auto window =
        glfwCreateWindow(640, 480, "Render Engine", nullptr, nullptr);

    RenderEngine::init(window);
    ResourceLib::init_resource_manager();

    auto* shader_code = re_load_spirv_code(
        "shaders/compiled/shaders.spv"
    );

    auto module = ShaderModule::create(shader_code);

    module->register_render_pipeline(
        "test_gp",
        "vertex_shader",
        "fragment_shader",
        true
    );

    auto global_descriptor_pool = DescriptorPool::create(
        module, teapot_count
    );

    ResourceLib::load_resources_from_json(
        json_path.c_str()
    );

    uint32_t model_count = ResourceLib::get_loaded_mesh_count("teapot_model");

    std::string sampled_texture_name = "sampled_image";
    std::string matrix_buffer_name = "matrix";

    auto depth_buffer = Image::create(
        1024,
        1024,
        RE_IMAGE_FORMAT_DEPTH,
        false,
        true,
        true
    );

    auto render_image = Image::create(
        1024,
        1024,
        RE_IMAGE_FORMAT_RGBA8,
        false,
        true,
        true
    );

    auto brick_wall_texture = ResourceLib::get_loaded_texture(
        "bricks_texture"
    );

    auto teapot_vertex_buffer = ResourceLib::get_loaded_mesh("teapot_model_0");
    auto resources = std::vector<std::tuple<std::string&, Resource*>>{
        {sampled_texture_name, brick_wall_texture.get()},
        {matrix_buffer_name, nullptr}
    };

    uint32_t set_index = 0;
    //init teapots data
    for (uint32_t i = 0; i < teapot_count; i++)
    {
        auto id_set = RangedIterator(std::views::single(set_index));
        teapot_resources_sets[i] = global_descriptor_pool->create_sets(id_set)[0];

        teapot_position_matricies[i] = RawBuffer::create(
            sizeof(glm::fmat4),
            true,
            true);
        teapot_render_objects[i].vertex_buffer = teapot_vertex_buffer;
        teapot_weak_ptr[i] = std::weak_ptr(teapot_resources_sets[i]);
        teapot_render_objects[i].sets = std::span(&teapot_weak_ptr[i], 1);

        teapot_rotation_directions[i] = glm::vec3((float)rand() / (float)RAND_MAX, (float)rand() / (float)RAND_MAX,
                                                  (float)rand() / (float)RAND_MAX);

        resources.at(1) = {matrix_buffer_name, teapot_position_matricies[i].get()};
        auto bindings = RangedIterator(std::views::all(resources));
        teapot_resources_sets[i]->write_bindings(
            bindings
        );
    }

    float teapot_y_displacement = 0.0f;

    auto previous_time_point = std::chrono::high_resolution_clock::now();
    teapot_y_displacement = 0;

    auto scaling_matrix = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));
    float begin_offset = -2.0f - teapot_per_row;

    wait_device_free();

    std::string ppl_name = "test_gp";

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        auto present_image = Image::get_swapchain_image();

        glm::mat4 perspective_matrix = glm::perspective(
            glm::radians(45.0f),
            (float)present_image->width() / (float)present_image->height(),
            0.1f,
            100.0f
        );

        {
            for (uint32_t i = 0; i < teapot_count; i++)
            {
                auto* mat = reinterpret_cast<glm::mat4*>(teapot_position_matricies[i]->map());

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


                teapot_position_matricies[i]->unmap();
            }
        }

        auto target_images = RangedIterator(std::views::single(render_image.get()));
        auto ro_it = RangedIterator(std::span(teapot_render_objects, teapot_count));

        RenderEngine::dispatch_graphics_pipeline(
            module,
            ppl_name,
            target_images,
            ro_it,
            depth_buffer,
          false
        );

        ImageToImageInfo transfer_info = {
            .from_image = render_image,
            .to_image = present_image
        };

        auto transport_it = RangedIterator(std::views::single(transfer_info));

        transfer_image_to_image(transport_it);

        present_image->present();

        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsed_seconds = now - previous_time_point;

        teapot_y_displacement += elapsed_seconds.count();
        previous_time_point = now;
    }

    wait_device_free();

    for (uint32_t i = 0; i < teapot_count; i++)
    {
        teapot_position_matricies[i].reset();
        teapot_render_objects[i].vertex_buffer.reset();
    }

    teapot_vertex_buffer.reset();
    brick_wall_texture.reset();
    ResourceLib::free_resource_manager();
    render_image.reset();
    global_descriptor_pool.reset();
    module.reset();
    re_free_spirv_code(shader_code);

    RenderEngine::release();

    glfwTerminate();
    return 0;
}
