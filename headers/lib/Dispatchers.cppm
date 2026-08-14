//
// Created by Onion on 12.08.2026.
//
module;

#ifdef __APPLE__
#include <memory>
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
}