//
// Created by Роман  Тимофеев on 18.05.2026.
//

#ifndef RENDERENGINE_DESCRIPTOR_POOL_H
#define RENDERENGINE_DESCRIPTOR_POOL_H
#include <vulkan/vulkan.hpp>
#include "shader_module.h"

struct RE_DescriptorPool {
   vk::DescriptorPool descriptor_pool;
   RE_ShaderModule* shader_module;
   uint32_t set_layouts = 0;
   uint32_t allocated_layouts = 0;
};

struct RE_DescriptorSet {
   vk::DescriptorSet descriptor_set;
   size_t set_index = 0;
   RE_DescriptorPool* pool;
   std::unordered_map<uint32_t, std::pair<void*, vk::DescriptorType>> bind_resources = {};
};

#endif //RENDERENGINE_DESCRIPTOR_POOL_H
