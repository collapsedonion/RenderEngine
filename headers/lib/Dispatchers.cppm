//
// Created by Onion on 12.08.2026.
//
module;

#ifdef __APPLE__
#include <memory>
#include <vulkan/vulkan.hpp>
#include <vector>
#include <functional>
#endif

export module Dispatchers;

#ifdef __linux__
export import std;
#endif

import ShaderModule;
import DescriptorPool;
import Image;
import RenderObject;
import dynamic_dispatchable_iterator;

export namespace RenderEngine
{

    void dispatch_compute_shader(
        std::shared_ptr<ShaderModule> shader_module,
        std::string& shader_name,
        Iterator<DescriptorPool::DescriptorSet*>& sets,
        std::uint32_t group_x,
        std::uint32_t group_y,
        std::uint32_t group_z
    );

    void dispatch_graphics_pipeline(
        std::shared_ptr<ShaderModule> shader_module,
        std::string& pipeline_name,
        Iterator<Image*>& target_images,
        Iterator<RenderObject>& render_objects,
        std::shared_ptr<Image> depth_image,
        bool load_image
    );

    class RenderRecorder {
    private:
        void* _rf;
        vk::CommandBuffer _main_command_buffer;
        std::vector<vk::CommandBuffer> _command_buffers = {};

    private:
        RenderRecorder();
        ~RenderRecorder();

    public:
        void render(
            std::shared_ptr<ShaderModule> shader_module,
            std::string& pipeline_name,
            Iterator<Image*>& target_images,
            Iterator<RenderObject>& render_objects,
            std::shared_ptr<Image> depth_image,
            bool load_image
        );

        static void record(
            std::function<void(RenderRecorder*)> commands
        );
    };


}