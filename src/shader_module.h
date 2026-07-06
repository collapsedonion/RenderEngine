//
// Created by Роман  Тимофеев on 29.04.2026.
//

#ifndef RENDERENGINE_SHADER_MODULE_H
#define RENDERENGINE_SHADER_MODULE_H
#include <vulkan/vulkan.hpp>
#include <string>
#include <unordered_map>

struct RE_BasePipeline {
    vk::Pipeline pipeline;
};

struct RE_GraphicsPipeline {
    RE_BasePipeline base_ppl;
    std::vector<vk::VertexInputAttributeDescription> vertex_buffer_inputs;
    size_t bytes_per_vertex;
    size_t required_image_bindings;
    bool depth_enable;
};

struct RE_PoolInfo {
    uint32_t storageBufferCount = 0;
    uint32_t uniformBufferCount = 0;
    uint32_t storageImageCount = 0;
    uint32_t combinedImageCount = 0;
};

struct RE_ShaderModule {
    vk::ShaderModule module;
    vk::PipelineLayout pipeline_layout;

    std::unordered_map<uint32_t, vk::DescriptorSetLayout> set_layouts;

    //stores bindings in pairs of set/binding
    std::unordered_map<std::string, std::pair<uint32_t, std::pair<uint32_t, vk::DescriptorType> > > binding_names;

    std::unordered_map
    <
        std::string,
        std::vector<vk::VertexInputAttributeDescription>
    > loaded_vertex_shaders{};

    std::unordered_map
    <
        std::string,
        size_t
    > loaded_fragment_shaders{};

    std::unordered_map<
        std::string,
        RE_BasePipeline
    > registered_compute_pipelines;

    std::unordered_map<
        std::string,
        RE_GraphicsPipeline
    > registered_graphics_pipelines;

    RE_PoolInfo pool_info{};
};

std::vector<vk::DescriptorPoolSize> genPoolSizes(RE_PoolInfo poolInfo, size_t multiplier);

#endif //RENDERENGINE_SHADER_MODULE_H
