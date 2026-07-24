//
// Created by Роман  Тимофеев on 18.05.2026.
//

module;

#include <re_typedefs.h>
#include "export_macro.h"

export module descriptor_pool;
export import vulkan;
import std;
import shader_module;

export struct RE_DescriptorPool {
    vk::DescriptorPool descriptor_pool;
    RE_ShaderModule* shader_module;
    uint32_t set_layouts = 0;
    uint32_t allocated_layouts = 0;
};

export struct RE_DescriptorSet {
    vk::DescriptorSet descriptor_set;
    size_t set_index = 0;
    RE_DescriptorPool* pool;
    std::unordered_map<uint32_t, std::pair<void*, vk::DescriptorType>> bind_resources = {};
};

export EXPORT_RE RE_pDescriptorPool re_create_descriptor_pool(
    RE_pShaderModule shader_module,
    uint32_t descriptor_count
);

export EXPORT_RE void re_free_descriptor_pool(
    RE_pDescriptorPool descriptor_pool
);

export EXPORT_RE void re_create_descriptor_sets(
    RE_pDescriptorPool descriptor_pool,
    uint32_t set_count,
    uint32_t *set_index,
    RE_pDescriptorSet *sets
);

export EXPORT_RE void re_write_set_buffers(
    RE_pDescriptorSet set,
    size_t write_count,
    const char **names,
    RE_pBuffer *buffers,
    uint32_t* offsets,
    uint32_t* sizes
);

export EXPORT_RE void re_write_set_images(
    RE_pDescriptorSet set,
    size_t write_count,
    const char **names,
    RE_pImage *images
);

//TODO add several pool deallocation feature
export EXPORT_RE void re_free_descriptor_sets(
    uint32_t set_count,
    RE_pDescriptorSet *descriptor_set
);
