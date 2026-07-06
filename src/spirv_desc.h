//
// Created by Роман  Тимофеев on 28.04.2026.
//

#ifndef RENDERENGINE_SPIRV_DESC_H
#define RENDERENGINE_SPIRV_DESC_H

#include <vector>
#include <unordered_map>
#include <vulkan/vulkan.hpp>

enum RE_SPVShaderTypes {
    RE_SPV_SHADER_COMPUTE,
    RE_SPV_SHADER_VERTEX,
    RE_SPV_SHADER_FRAGMENT,
};

struct RE_SpvShader {
    std::string name;
    RE_SPVShaderTypes type;

    std::unordered_map<uint32_t, std::vector<uint32_t>> sets;

    std::vector<vk::VertexInputAttributeDescription> vertex_attributes;
    size_t color_output_count;
};

struct RE_SpvBinding {
    std::string name;
    vk::DescriptorType type;
};

struct RE_SpvSet {
    uint32_t set_index;
    std::unordered_map<uint32_t, RE_SpvBinding> bindings;
};

struct RE_SpirVCode {
    std::vector<uint32_t> code {};

    std::vector<RE_SpvShader> shaders {};
    std::vector<RE_SpvSet> sets {};
};

#endif //RENDERENGINE_SPIRV_DESC_H
