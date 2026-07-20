//
// Created by Onion on 20.07.2026.
//
#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <print>
#include <GLFW/glfw3.h>
#include <render_engine.h>
#include <simple_draw.h>

#include <vulkan/vulkan.hpp>

using namespace RenderEngine;

int main()
{
    glfwInit();
    glfwInitVulkanLoader(vkGetInstanceProcAddr);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    //  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    auto window =
        glfwCreateWindow(640, 480, "Render Engine", nullptr, nullptr);

    init_render_engine(window);
    auto* simple_drawer = new SimpleDrawer(
        "./shaders/compiled/simple_draw.spv"
    );

    simple_drawer->add_scene_2d(
        "test",
        [](Scene2D& scn)
        {
            scn
            .rect()
            .color(Float3{0.8, 0.6, 0.0})
            .size(Float2(0.5))
            .position(Float2(0.0, 0.0));

            scn
            .triangle()
            .color(Float3{0.8, 0.0, 0.0})
            .size(Float2(0.5))
            .position(Float2(0.0, -0.5));
        }
    );

    RE_pImage draw_image = re_create_image(
        1067,
        800,
        RE_IMAGE_FORMAT_RGBA8,
        true,
        false,
        false
    );

    simple_drawer->render_scene_to_texture(
        "test",
        draw_image
    );

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        RE_pImage target_img = re_get_present_image();

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
