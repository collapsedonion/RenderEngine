//
// Created by Onion on 22.07.2026.
//

module;

#include <re_typedefs.h>
#include "export_macro.h"
#include "cmake-build-debug/_deps/assimp-src/contrib/zip/src/miniz.h"

#if defined(__APPLE__)
#include <ranges>
#include <string>
#include <vector>
#include <memory>
#include <vulkan/vulkan.hpp>
#endif

module DescriptorPool;

#if defined(__linux__)
import vulkan;
import std;
#endif
import render_engine_shares;
import Buffer;
import Image;

using namespace RenderEngine;

bool isBufferType(vk::DescriptorType type) {
    return type == vk::DescriptorType::eUniformBuffer ||
           type == vk::DescriptorType::eStorageBuffer;
}

bool isImageType(vk::DescriptorType type) {
    return type == vk::DescriptorType::eStorageImage ||
           type == vk::DescriptorType::eCombinedImageSampler;
}

DescriptorPool::~DescriptorPool()
{
    vk::Device _device = vkb_device.device;
    vkb_device_lock.lock();
    _device.destroy(_pool);
    vkb_device_lock.unlock();
}

DescriptorPool::DescriptorPool(
    std::shared_ptr<ShaderModule> shader_module,
    std::uint32_t descriptor_count
)
{
    _shader_module = std::move(shader_module);
    auto sizes = _shader_module->get_pool_sizes(descriptor_count);

    vk::DescriptorPoolCreateInfo poolInfo = {};
    poolInfo.maxSets = descriptor_count;
    poolInfo.poolSizeCount = sizes.size();
    poolInfo.pPoolSizes = sizes.data();

    poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;

    vk::Device _device = vkb_device.device;
    vkb_device_lock.lock();
    vk::DescriptorPool pool = _device.createDescriptorPool(poolInfo);
    vkb_device_lock.unlock();

    _pool = pool;
    _max_set= descriptor_count;
}

std::vector<std::shared_ptr<DescriptorPool::DescriptorSet>> DescriptorPool::create_sets(
    Iterator<std::uint32_t>& set_indices
    ) const
{
    std::vector<vk::DescriptorSetLayout> set_layouts;
    std::vector<std::uint32_t> id_s;
    auto min_size = std::min(10, static_cast<int>(_max_set - _allocated_layouts));
    set_layouts.reserve(min_size);
    id_s.reserve(min_size);

    while (set_indices)
    {
        auto& id = set_indices.next();
        set_layouts.push_back(
            _shader_module->get_set_layout(id)
        );
        id_s.push_back(id);
    }

    vk::DescriptorSetAllocateInfo set_info = {};
    set_info.descriptorSetCount = set_layouts.size();
    set_info.pSetLayouts = set_layouts.data();
    set_info.descriptorPool = _pool;

    vk::Device _device = vkb_device.device;

    vkb_device_lock.lock();
    std::vector<vk::DescriptorSet> result_sets = _device.allocateDescriptorSets(
        set_info
    );
    vkb_device_lock.unlock();

    std::vector<std::shared_ptr<DescriptorSet>> result = {};
    result.reserve(set_layouts.size());

    for (auto [set, id] : std::views::zip(result_sets, id_s))
    {
        auto new_set = new DescriptorSet{};
        new_set->_my_pool = _self;
        new_set->_index = id;
        new_set->_set = set;
        new_set->_bound_resources.reserve(_shader_module->get_max_binding_count());

        result.push_back(std::shared_ptr<DescriptorSet>(new_set));
    }

    return result;
}

DescriptorPool::DescriptorSet::~DescriptorSet()
{
    auto pool = _my_pool.lock();

    if (!pool)
    {
        return;
    }

    vkb_device_lock.lock();
    vk::Device device = vkb_device.device;
    device.freeDescriptorSets(
          pool->_pool,
          1,
          &_set
    );
    vkb_device_lock.unlock();
}

void DescriptorPool::DescriptorSet::write_bindings(
    Iterator<std::tuple<std::string&, Resource*>>& resources
)
{
    std::vector<vk::WriteDescriptorSet> set_writes;

    while (resources)
    {
        auto [name, resource] = resources.next();

        auto ptr = _my_pool.lock();
        if (!ptr)
        {
            return;
        }

        auto [set, binding_index, type] = ptr->_shader_module->get_binding_info(name);
        auto write_info = resource->get_descriptor_set_write(type);

        write_info.dstSet = _set;
        write_info.dstBinding = binding_index;

        set_writes.push_back(write_info);
        _bound_resources[binding_index] = {resource, type};
    }

    vkb_device_lock.lock();
    vk::Device device = vkb_device.device;
    device.updateDescriptorSets(
        set_writes.size(),
        set_writes.data(),
        0,
        nullptr
    );
    vkb_device_lock.unlock();
}
