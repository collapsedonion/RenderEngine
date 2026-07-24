//
// Created by Onion on 22.07.2026.
//

module;

#include <spirv_reflect.h>
#include <spirv_tools.h>

#define EXPORT_RE extern "C"

module spirv_analyser;

import vulkan;
import std;

inline size_t spv_reflect_format_to_size_t(
    SpvReflectFormat format
) {
    switch (format) {
        case SPV_REFLECT_FORMAT_R32_SFLOAT:
            return sizeof(float);
        case SPV_REFLECT_FORMAT_R32G32_SFLOAT:
            return 2 * sizeof(float);
        case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT:
            return 3 * sizeof(float);
        case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT:
            return 4 * sizeof(float);
    }
}

inline vk::Format spv_reflect_format_to_vk_format(
    SpvReflectFormat format
) {
    switch (format) {
        case SPV_REFLECT_FORMAT_R32_SFLOAT:
            return vk::Format::eR32Sfloat;
        case SPV_REFLECT_FORMAT_R32G32_SFLOAT:
            return vk::Format::eR32G32Sfloat;
        case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT:
            return vk::Format::eR32G32B32Sfloat;
        case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT:
            return vk::Format::eR32G32B32A32Sfloat;
    }
}

RE_SpvShader process_shader(
    spv_reflect::ShaderModule *spv_module,
    uint32_t index
) {
    RE_SpvShader shader = {};
    bool is_vertex = false;
    bool is_fragment = false;

    switch (spv_module->GetEntryPointShaderStage(index)) {
        case SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT:
            shader.type = RE_SPV_SHADER_COMPUTE;
            break;
        case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT:
            shader.type = RE_SPV_SHADER_VERTEX;
            is_vertex = true;
            break;
        case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT:
            shader.type = RE_SPV_SHADER_FRAGMENT;
            is_fragment = true;
            break;
        default: ;
    };

    shader.name = spv_module->GetEntryPointName(index);

    uint32_t setCount;

    spv_module->EnumerateEntryPointDescriptorSets(
        shader.name.c_str(),
        &setCount,
        nullptr
    );

    std::vector<SpvReflectDescriptorSet *> allBindings(setCount);

    spv_module->EnumerateEntryPointDescriptorSets(
        shader.name.c_str(),
        &setCount,
        allBindings.data()
    );

    for (uint32_t i = 0; i < setCount; i++) {
        shader.sets.insert({allBindings[i]->set, {}});

        for (size_t j = 0; j < allBindings[i]->binding_count; j++) {
            shader.sets[allBindings[i]->set].push_back(
                allBindings[i]->bindings[j]->binding
            );
        }
    }

    if (is_fragment) {
        uint32_t outputCount;
        spv_module->EnumerateEntryPointOutputVariables(
            shader.name.c_str(),
            &outputCount,
            nullptr
        );

        std::vector<SpvReflectInterfaceVariable *> allOutputs(outputCount);

        spv_module->EnumerateEntryPointOutputVariables(
            shader.name.c_str(),
            &outputCount,
           allOutputs.data()
        );

        shader.color_output_count = 0;

        for (auto output: allOutputs) {
            shader.color_output_count++;
        }

        return shader;
    }

    if (!is_vertex) {
        return shader;
    }

    uint32_t input_count;

    spv_module->EnumerateEntryPointInputVariables(
        shader.name.c_str(),
        &input_count,
        nullptr
    );

    std::vector<SpvReflectInterfaceVariable *> allInputs(input_count);
    spv_module->EnumerateEntryPointInputVariables(
        shader.name.c_str(),
        &input_count,
        allInputs.data()
    );

    size_t total_offset = 0;

    std::map<uint32_t, SpvReflectFormat> inputs;

    for (uint32_t i = 0; i < input_count; i++) {
        if (allInputs[i]->built_in != -1) {
            continue;
        }

        inputs.insert({allInputs[i]->location, allInputs[i]->format});
    }

    shader.vertex_attributes.reserve(input_count);

    for (auto input: inputs) {
        vk::VertexInputAttributeDescription inputAttribute;
        inputAttribute.location = input.first;
        inputAttribute.format = spv_reflect_format_to_vk_format(input.second);
        inputAttribute.binding = 0;
        inputAttribute.offset = total_offset;
        total_offset += spv_reflect_format_to_size_t(input.second);
        shader.vertex_attributes.push_back(inputAttribute);
    }

    shader.vertex_attributes.shrink_to_fit();

    return shader;
}

EXPORT_RE RE_pSpirVCode re_load_spirv_code(const char *path) {
    std::filesystem::path path_to_code(path);
    std::ifstream ifs(path_to_code, std::ios::binary | std::ios::ate);

    if (!ifs.is_open()) {
        throw std::runtime_error(std::format(
            "Failed to open spirv code file: '{}'",
            path
        ));
    }

    size_t size = ifs.tellg();

    auto *newCode = new RE_SpirVCode();

    newCode->code.resize(size / 4);

    ifs.seekg(0);

    ifs.read(
        (char *) newCode->code.data(),
        size
    );

    ifs.close();

    auto spv_module = spv_reflect::ShaderModule(newCode->code);

    for (uint32_t i = 0; i < spv_module.GetEntryPointCount(); i++) {
        newCode->shaders.push_back(
            process_shader(
                &spv_module,
                i
            )
        );
    }

    uint32_t setCount;

    spv_module.EnumerateDescriptorSets(&setCount, nullptr);

    std::vector<SpvReflectDescriptorSet *> allSets(setCount);
    spv_module.EnumerateDescriptorSets(&setCount, allSets.data());

    newCode->sets.reserve(setCount);

    for (uint32_t i = 0; i < setCount; i++) {
        newCode->sets.push_back({});
        newCode->sets[i].set_index = i;

        for (uint32_t j = 0; j < allSets[i]->binding_count; j++) {
            SpvReflectDescriptorBinding *binding = allSets[i]->bindings[j];

            vk::DescriptorType descriptorType{};

            switch (binding->descriptor_type) {
                case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                    descriptorType = vk::DescriptorType::eStorageBuffer;
                    break;
                case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                    descriptorType = vk::DescriptorType::eUniformBuffer;
                    break;
                case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                    descriptorType = vk::DescriptorType::eStorageImage;
                    break;
                case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                    descriptorType = vk::DescriptorType::eCombinedImageSampler;
                default:
                    break;
            }

            RE_SpvBinding spv_binding{};
            spv_binding.name = binding->name;
            spv_binding.type = descriptorType;

            newCode->sets[i].bindings.insert({
                j,
                spv_binding
            });
        }
    }

    return newCode;
}

EXPORT_RE void re_free_spirv_code(RE_pSpirVCode code) {
    auto *_code = reinterpret_cast<RE_SpirVCode *>(code);

    _code->code.clear();

    delete _code;
}
