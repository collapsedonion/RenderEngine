//
// Created by Onion on 10.08.2026.
//
module;

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

export module render_engine;
#ifdef __linux__
import std;
import Image;
#endif

export namespace RenderEngine
{

    /**
     * Before using any of libraries functionality initialization function must be called.
     *
     * @param [in] window pointer to valid glfw's window handler, must be created and managed manually
     */
    void init(GLFWwindow* window);


    /**
     * After using libraries functionality all resources must be freed.
     *
     * @note All created buffers/images/descriptor pools/etc must be freed manual before freeing engine
     */
    void release();

    void wait_device_free();
}