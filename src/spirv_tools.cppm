//
// Created by Роман  Тимофеев on 28.04.2026.
//

module;

#include <spirv_tools.h>

#define EXPORT_RE extern "C"

export module spirv_analyser;
export import vulkan;
import std;

export enum RE_SPVShaderTypes {
    RE_SPV_SHADER_COMPUTE,
    RE_SPV_SHADER_VERTEX,
    RE_SPV_SHADER_FRAGMENT,
};

export struct RE_SpvShader {
    std::string name;
    RE_SPVShaderTypes type;

    std::map<uint32_t, std::vector<uint32_t>> sets;

    std::vector<vk::VertexInputAttributeDescription> vertex_attributes;
    size_t color_output_count;
};

export struct RE_SpvBinding {
    std::string name;
    vk::DescriptorType type;
};

export struct RE_SpvSet {
    uint32_t set_index;
    std::map<uint32_t, RE_SpvBinding> bindings;
};

export struct RE_SpirVCode {
    std::vector<uint32_t> code {};

    std::vector<RE_SpvShader> shaders {};
    std::vector<RE_SpvSet> sets {};
};

export EXPORT_RE RE_pSpirVCode re_load_spirv_code(const char *path);

export EXPORT_RE void re_free_spirv_code(RE_pSpirVCode code);
