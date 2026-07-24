//
// Created by Onion on 22.07.2026.
//

module;

#include <re_typedefs.h>
#define EXPORT_RE extern "C"

module shader_module;

import vulkan;
import std;
import render_engine_shares;
import spirv_analyser;
import storage_buffer;

inline size_t vk_format_to_size(vk::Format format) {
    switch (format) {
        case vk::Format::eR32Sfloat:
            return sizeof(float);
        case vk::Format::eR32G32Sfloat:
            return 2 * sizeof(float);
        case vk::Format::eR32G32B32Sfloat:
            return 3 * sizeof(float);
        case vk::Format::eR32G32B32A32Sfloat:
            return 4 * sizeof(float);
        default:
            return 0;
    }
}

std::vector<vk::DescriptorPoolSize> genPoolSizes(RE_PoolInfo poolInfo, size_t multiplier) {
    std::vector<vk::DescriptorPoolSize> poolSizes;
    poolSizes.reserve(4);

    if (poolInfo.storageBufferCount > 0) {
        poolSizes.emplace_back();
        poolSizes.back().descriptorCount = poolInfo.storageBufferCount * multiplier;
        poolSizes.back().type = vk::DescriptorType::eStorageBuffer;
    }

    if (poolInfo.uniformBufferCount > 0) {
        poolSizes.emplace_back();
        poolSizes.back().descriptorCount = poolInfo.uniformBufferCount * multiplier;
        poolSizes.back().type = vk::DescriptorType::eUniformBuffer;
    }

    if (poolInfo.storageImageCount > 0) {
        poolSizes.emplace_back();
        poolSizes.back().descriptorCount = poolInfo.storageImageCount * multiplier;
        poolSizes.back().type = vk::DescriptorType::eStorageImage;
    }

    if (poolInfo.combinedImageCount > 0) {
        poolSizes.emplace_back();
        poolSizes.back().descriptorCount = poolInfo.combinedImageCount * multiplier;
        poolSizes.back().type = vk::DescriptorType::eCombinedImageSampler;
    }

    poolSizes.shrink_to_fit();

    return poolSizes;
}

EXPORT_RE RE_pShaderModule re_create_shader_module(
    RE_pSpirVCode shader_code
) {
    auto *shader_module = new RE_ShaderModule();

    vk::Device device = vkb_device.device;

    auto *_shader_code = static_cast<RE_SpirVCode *>(shader_code);
    vk::ShaderModuleCreateInfo sm_ci{};
    sm_ci.codeSize = _shader_code->code.size() * sizeof(uint32_t);
    sm_ci.pCode = _shader_code->code.data();

    shader_module->module = device.createShaderModule(
        sm_ci
    );

    for (auto &set: _shader_code->sets) {
        vk::DescriptorSetLayoutCreateInfo newSet{};
        newSet.bindingCount = set.bindings.size();
        std::vector<vk::DescriptorSetLayoutBinding> newBindings{};
        newBindings.reserve(newSet.bindingCount);

        for (auto &binding: set.bindings) {
            vk::DescriptorSetLayoutBinding newBinding{};
            newBinding.binding = binding.first;
            newBinding.descriptorType = binding.second.type;
            newBinding.descriptorCount = 1;
            newBinding.stageFlags = vk::ShaderStageFlagBits::eAll;

            switch (newBinding.descriptorType) {
                case vk::DescriptorType::eStorageBuffer:
                    shader_module->pool_info.storageBufferCount++;
                    break;
                case vk::DescriptorType::eUniformBuffer:
                    shader_module->pool_info.uniformBufferCount++;
                    break;
                case vk::DescriptorType::eStorageImage:
                    shader_module->pool_info.storageImageCount++;
                    break;
                case vk::DescriptorType::eCombinedImageSampler:
                    shader_module->pool_info.combinedImageCount++;
                    break;

                default:

                    break;

            }

            shader_module->binding_names.insert(
                {
                    binding.second.name,
                    {set.set_index, {binding.first, newBinding.descriptorType}}
                }
            );

            newBindings.push_back(newBinding);
        }

        newSet.pBindings = newBindings.data();

        vk::DescriptorSetLayout newLayout = device.createDescriptorSetLayout(
            newSet,
            nullptr
        );
        shader_module->set_layouts.insert({
            set.set_index,
            newLayout
        });
    }

    vk::PipelineLayoutCreateInfo layout_compute_ci{};;
    std::vector<vk::DescriptorSetLayout> layouts{};
    layouts.reserve(shader_module->set_layouts.size());
    uint64_t next_set = 0;

    for (auto &[_, set]: shader_module->set_layouts) {
       /* if (next_set != set.first) {
            for (size_t i = next_set; i < set.first; i++) {
                layouts.push_back(vk_empty_descriptor_set_layout);
            }
        }*/
        layouts.push_back(set);
    }

    layout_compute_ci.setLayoutCount = layouts.size();

    layout_compute_ci.pSetLayouts = layouts.data();

    shader_module->pipeline_layout = device.createPipelineLayout(
        layout_compute_ci
    );

    for (auto &shader: _shader_code->shaders) {
        switch (shader.type) {
            case RE_SPV_SHADER_COMPUTE: {
                vk::ComputePipelineCreateInfo compute_ci{};
                compute_ci.layout = shader_module->pipeline_layout;

                vk::PipelineShaderStageCreateInfo shader_stage{};
                shader_stage.module = shader_module->module;
                shader_stage.stage = vk::ShaderStageFlagBits::eCompute;
                shader_stage.pName = shader.name.c_str();

                compute_ci.stage = shader_stage;

                vk::Pipeline compute_pl = device.createComputePipeline(
                    {},
                    compute_ci
                ).value;

                RE_BasePipeline newCP{};
                newCP.pipeline = compute_pl;

                shader_module->registered_compute_pipelines.insert({shader.name, newCP});
                break;
            }

            case RE_SPV_SHADER_VERTEX: {
                shader_module->loaded_vertex_shaders.insert({
                    shader.name,
                    shader.vertex_attributes
                });
                break;
            }

            case RE_SPV_SHADER_FRAGMENT: {
                shader_module->loaded_fragment_shaders.insert({
                    shader.name,
                    shader.color_output_count
                });
                break;
            }
        }
    }

    return shader_module;
}

EXPORT_RE void re_register_render_pipeline(
    RE_pShaderModule shader_module,
    const char *pipeline_name,
    const char *vertex_name,
    const char *fragment_name,
    bool depth
)
{
    auto *_shader_module = static_cast<RE_ShaderModule *>(shader_module);

    vk::PipelineShaderStageCreateInfo shader_stages[2] = {};
    shader_stages[0] = vk::PipelineShaderStageCreateInfo{};
    shader_stages[0].stage = vk::ShaderStageFlagBits::eVertex;
    shader_stages[0].pName = vertex_name;
    shader_stages[0].module = _shader_module->module;

    shader_stages[1] = vk::PipelineShaderStageCreateInfo{};
    shader_stages[1].stage = vk::ShaderStageFlagBits::eFragment;
    shader_stages[1].pName = fragment_name;
    shader_stages[1].module = _shader_module->module;

    vk::GraphicsPipelineCreateInfo ci{};
    ci.stageCount = 2;
    ci.pStages = shader_stages;

    vk::PipelineVertexInputStateCreateInfo vertex_input_ci{};
    auto &vertex_inputs =
        _shader_module->loaded_vertex_shaders[vertex_name];
    vertex_input_ci.pVertexAttributeDescriptions = vertex_inputs.data();
    vertex_input_ci.vertexAttributeDescriptionCount = vertex_inputs.size();
    size_t total_size = 0;

    if (!vertex_inputs.empty()) {
        auto &last_vertex_input = vertex_inputs.back();
        total_size = last_vertex_input.offset + vk_format_to_size(last_vertex_input.format);
    }

    vk::VertexInputBindingDescription vertex_binding{};
    vertex_binding.binding = 0;
    vertex_binding.stride = total_size;
    vertex_binding.inputRate = vk::VertexInputRate::eVertex;
    vertex_input_ci.vertexBindingDescriptionCount = 1;
    vertex_input_ci.pVertexBindingDescriptions = &vertex_binding;

    ci.pVertexInputState = &vertex_input_ci;

    vk::PipelineInputAssemblyStateCreateInfo input_assembly_ci{};
    input_assembly_ci.topology = vk::PrimitiveTopology::eTriangleList;
    ci.pInputAssemblyState = &input_assembly_ci;

    vk::PipelineTessellationStateCreateInfo tessellation_ci{};
    ci.pTessellationState = &tessellation_ci;

    vk::PipelineRasterizationStateCreateInfo r_ci{};
    r_ci.depthClampEnable = false;
    r_ci.rasterizerDiscardEnable = false;
    r_ci.polygonMode = vk::PolygonMode::eFill;
    r_ci.cullMode = vk::CullModeFlagBits::eBack;
    r_ci.frontFace = vk::FrontFace::eClockwise;
    r_ci.depthBiasEnable = true;
    r_ci.lineWidth = 1.0f;

    ci.pRasterizationState = &r_ci;

    vk::PipelineMultisampleStateCreateInfo multisample_ci{};
    multisample_ci.rasterizationSamples = vk::SampleCountFlagBits::e1;

    ci.pMultisampleState = &multisample_ci;

    vk::PipelineDepthStencilStateCreateInfo depth_stencil_ci{};
    depth_stencil_ci.depthTestEnable = depth;
    depth_stencil_ci.depthWriteEnable = true;
    depth_stencil_ci.depthCompareOp = vk::CompareOp::eLess;

    ci.pDepthStencilState = &depth_stencil_ci;

    vk::PipelineColorBlendAttachmentState color_blend_state{};
    color_blend_state.blendEnable = false;
    color_blend_state.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                       vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

    size_t color_output_count = _shader_module->loaded_fragment_shaders[fragment_name];

    auto color_blend_attachments =
                std::views::repeat(color_blend_state, color_output_count) |
                    std::ranges::to<std::vector<vk::PipelineColorBlendAttachmentState>>();

    auto color_formats =
            std::views::repeat(vk::Format::eR8G8B8A8Srgb, color_output_count) |
                std::ranges::to<std::vector<vk::Format>>();

    vk::PipelineColorBlendStateCreateInfo blend_ci{};
    blend_ci.pAttachments = color_blend_attachments.data();
    blend_ci.attachmentCount = color_output_count;
    blend_ci.logicOpEnable = false;

    vk::PipelineRenderingCreateInfo rendering_ci{};
    rendering_ci.depthAttachmentFormat = vk::Format::eD32Sfloat;
    rendering_ci.pColorAttachmentFormats = color_formats.data();
    rendering_ci.colorAttachmentCount = color_output_count;
    ci.pNext = &rendering_ci;

    const std::array<vk::DynamicState, 2> dynamic_states = {
        vk::DynamicState::eViewportWithCount,
        vk::DynamicState::eScissorWithCount,

    };

    ci.pColorBlendState = &blend_ci;
    vk::PipelineDynamicStateCreateInfo dynamic_state_ci = {};
    dynamic_state_ci.dynamicStateCount = 2;
    dynamic_state_ci.pDynamicStates = dynamic_states.data();

    ci.pDynamicState = &dynamic_state_ci;
    ci.layout = _shader_module->pipeline_layout;

    vk::Device _device = vkb_device.device;
    vkb_device_lock.lock();
    vk::Pipeline ppl = _device.createGraphicsPipeline({}, ci).value;
    vkb_device_lock.unlock();

    RE_GraphicsPipeline gppl{};
    gppl.vertex_buffer_inputs = vertex_inputs;
    gppl.base_ppl.pipeline = ppl;
    gppl.bytes_per_vertex = total_size;
    gppl.depth_enable = depth;
    gppl.required_image_bindings = color_output_count;

    _shader_module->registered_graphics_pipelines.insert({pipeline_name, gppl});
}

EXPORT_RE RE_pBuffer re_allocate_vertex_buffer(
    RE_pShaderModule shader_module,
    const char* pipeline_name,
    uint32_t vertex_count
) {
    auto* _shader_module = static_cast<RE_ShaderModule*>(shader_module);

    size_t buffer_size = vertex_count * _shader_module->registered_graphics_pipelines[pipeline_name].bytes_per_vertex;

    auto* new_buffer = re_create_buffer(
        buffer_size,
        false,
        false
    );

    return new_buffer;
}

EXPORT_RE void re_free_shader_module(
    RE_pShaderModule shader_module
) {
    auto *_shader_module = static_cast<RE_ShaderModule *>(shader_module);

    vk::Device device = vkb_device.device;

    for (auto &shader: _shader_module->registered_compute_pipelines) {
        device.destroy(shader.second.pipeline);
    }

    for (auto& shader : _shader_module->registered_graphics_pipelines) {
        device.destroy(shader.second.base_ppl.pipeline);
    }

    device.destroy(_shader_module->pipeline_layout);

    for (auto &set: _shader_module->set_layouts) {
        device.destroy(set.second);
    }

    device.destroyShaderModule(_shader_module->module);

    delete _shader_module;
}
