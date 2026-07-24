//
// Created by Роман  Тимофеев on 29.04.2026.
//

module;

#include <re_typedefs.h>

#define EXPORT_RE extern "C"

export module shader_module;
export import vulkan;
import std;

export struct RE_BasePipeline {
    vk::Pipeline pipeline;
};

export struct RE_GraphicsPipeline {
    RE_BasePipeline base_ppl;
    std::vector<vk::VertexInputAttributeDescription> vertex_buffer_inputs;
    size_t bytes_per_vertex;
    size_t required_image_bindings;
    bool depth_enable;
};

export struct RE_PoolInfo {
    uint32_t storageBufferCount = 0;
    uint32_t uniformBufferCount = 0;
    uint32_t storageImageCount = 0;
    uint32_t combinedImageCount = 0;
};

export struct RE_ShaderModule {
    vk::ShaderModule module;
    vk::PipelineLayout pipeline_layout;

    std::map<uint32_t, vk::DescriptorSetLayout> set_layouts;

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

export std::vector<vk::DescriptorPoolSize> genPoolSizes(
    RE_PoolInfo poolInfo,
    size_t multiplier
);

export EXPORT_RE RE_pShaderModule re_create_shader_module(
    RE_pSpirVCode shader_code
);

export EXPORT_RE void re_register_render_pipeline(
    RE_pShaderModule shader_module,
    const char *pipeline_name,
    const char *vertex_name,
    const char *fragment_name,
    bool depth
);

export EXPORT_RE RE_pBuffer re_allocate_vertex_buffer(
    RE_pShaderModule shader_module,
    const char* pipeline_name,
    uint32_t vertex_count
);

export EXPORT_RE void re_free_shader_module(
    RE_pShaderModule shader_module
);
