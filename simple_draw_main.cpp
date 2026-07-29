//
// Created by Onion on 20.07.2026.
//
#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <print>
#include <GLFW/glfw3.h>
#include <render_engine.h>

#include <vulkan/vulkan.hpp>

import simple_draw;
import scene2d;
import scene3d;

using namespace RenderEngine;

const uint32_t HEIGHT = 600;
const float RATIO = 1.33;

int main()
{
    glfwInit();
    glfwInitVulkanLoader(vkGetInstanceProcAddr);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    //  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    auto window =
        glfwCreateWindow(RATIO * HEIGHT, HEIGHT, "Render Engine", nullptr, nullptr);

    init_render_engine(window);
    auto* simple_drawer = new SimpleDrawer(
        "./shaders/compiled/simple_draw.spv"
    );

    auto& scene_3d = simple_drawer->add_scene<Scene3D>(
        "test",
        [](Scene3D& scn)
        {
            scn.init_resolution({800, 600});
            scn
                .add_item<Rectangle3D>()
                .color(Color{0.8, 0.6, 0.0})
                .position_matrix(Mat4x4::position({0, 0, -10.0f}))
                .name("rect1");

            scn
                .add_item<Rectangle3D>()
                .color(Color{0.6, 0.8, 0.0})
                .position_matrix(Mat4x4::position({.5, .5, 0}))
                .name("rect2");
        }
    );

    RE_pImage draw_image = re_create_image(
        800 * RATIO,
        800,
        RE_IMAGE_FORMAT_RGBA8,
        true,
        false,
        false
    );

    auto rect_ptr = scene_3d.get_item_by_name("rect1");
    auto rect_ptr2 = scene_3d.get_item_by_name("rect2");
    rect_ptr2.lock()->set_parent(rect_ptr.lock());

    auto previous = std::chrono::high_resolution_clock::now();

    constexpr float freq = std::numbers::pi / 3.0;

    float counter = 0.0;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        RE_pImage target_img = re_get_present_image();

        auto now = std::chrono::high_resolution_clock::now();
        auto dt = std::chrono::duration_cast<std::chrono::duration<float>>(now - previous);
        previous = now;

        re_wait_device_free();

        counter += dt.count();
        rect_ptr.lock()->set_position_matrix(Mat4x4::position({
            2 * std::sin(counter * freq), 2 * std::cos(counter * freq), -10.0f
        }));

        simple_drawer->render_scene_to_texture(
            "test",
            draw_image
        );

        RE_ImageToImageTransfer itit{
            .from_image = draw_image,
            .to_image = target_img
        };

        re_transfer_image_to_image(
            &itit,
            1
        );

        re_present_image(target_img);
    }

    re_wait_device_free();

    re_free_image(draw_image);
    delete simple_drawer;
    re_free_render_engine();

    glfwTerminate();
    return 0;
}
