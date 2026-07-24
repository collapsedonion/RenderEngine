//
// Created by Onion on 22.07.2026.
//

module;

#include <re_typedefs.h>
#include "export_macro.h"

module descriptor_pool;

import vulkan;
import std;
import shader_module;
import render_engine_shares;
import storage_buffer;
import image;

bool isBufferType(vk::DescriptorType type) {
    return type == vk::DescriptorType::eUniformBuffer ||
           type == vk::DescriptorType::eStorageBuffer;
}

bool isImageType(vk::DescriptorType type) {
    return type == vk::DescriptorType::eStorageImage ||
           type == vk::DescriptorType::eCombinedImageSampler;
}

EXPORT_RE RE_pDescriptorPool re_create_descriptor_pool(
    RE_pShaderModule shader_module,
    uint32_t descriptor_count
) {
    auto *shader_module_ptr = static_cast<RE_ShaderModule *>(shader_module);
    auto sizes = genPoolSizes(shader_module_ptr->pool_info, descriptor_count);

    vk::DescriptorPoolCreateInfo poolInfo = {};
    poolInfo.maxSets = descriptor_count;
    poolInfo.poolSizeCount = sizes.size();
    poolInfo.pPoolSizes = sizes.data();

    poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;

    vk::Device _device = vkb_device.device;
    vkb_device_lock.lock();
    vk::DescriptorPool _pool = _device.createDescriptorPool(poolInfo);
    vkb_device_lock.unlock();

    auto *descriptor_pool = new RE_DescriptorPool();

    descriptor_pool->shader_module = shader_module_ptr;
    descriptor_pool->descriptor_pool = _pool;
    descriptor_pool->set_layouts = descriptor_count;

    return descriptor_pool;
}

EXPORT_RE void re_free_descriptor_pool(
    RE_pDescriptorPool descriptor_pool
) {
    auto *descriptor_pool_ptr = static_cast<RE_DescriptorPool *>(descriptor_pool);

    vk::Device _device = vkb_device.device;
    vkb_device_lock.lock();
    _device.destroy(descriptor_pool_ptr->descriptor_pool);
    vkb_device_lock.unlock();
    delete descriptor_pool_ptr;
}

EXPORT_RE void re_create_descriptor_sets(
    RE_pDescriptorPool descriptor_pool,
    uint32_t set_count,
    uint32_t *set_index,
    RE_pDescriptorSet *sets
) {
    if (set_count == 0) {
        return;
    }

    auto *descriptor_pool_ptr = static_cast<RE_DescriptorPool *>(descriptor_pool);

    if (set_count + descriptor_pool_ptr->allocated_layouts > descriptor_pool_ptr->set_layouts) {
        throw std::runtime_error("Insufficient space in descriptor pool");
    }

    descriptor_pool_ptr->allocated_layouts += set_count;

    std::vector<vk::DescriptorSetLayout> set_layouts;
    set_layouts.reserve(set_count);

    for (uint32_t i = 0; i < set_count; i++) {
        set_layouts.push_back(
            descriptor_pool_ptr->shader_module->set_layouts[set_index[i]]
        );
    }

    vk::DescriptorSetAllocateInfo set_info = {};
    set_info.descriptorSetCount = set_count;
    set_info.pSetLayouts = set_layouts.data();
    set_info.descriptorPool = descriptor_pool_ptr->descriptor_pool;

    vk::Device _device = vkb_device.device;

    vkb_device_lock.lock();
    std::vector<vk::DescriptorSet> result = _device.allocateDescriptorSets(
        set_info
    );
    vkb_device_lock.unlock();

    for (uint32_t i = 0; i < result.size(); i++) {
        auto *set = new RE_DescriptorSet();
        set->pool = descriptor_pool_ptr;
        set->set_index = set_index[i];
        set->descriptor_set = result[i];
        set->bind_resources.reserve(descriptor_pool_ptr->shader_module->binding_names.size());
        sets[i] = set;
    }
}

EXPORT_RE void re_write_set_buffers(
    RE_pDescriptorSet set,
    size_t write_count,
    const char **names,
    RE_pBuffer *buffers,
    uint32_t* offsets,
    uint32_t* sizes
) {
    auto *shader_module_ptr = static_cast<RE_DescriptorSet *>(set)->pool->shader_module;
    auto *target_set = static_cast<RE_DescriptorSet *>(set);

    std::vector<vk::WriteDescriptorSet> writes;
    std::vector<vk::DescriptorBufferInfo> buffer_cache;
    writes.reserve(write_count);
    buffer_cache.reserve(write_count);

    for (uint32_t i = 0; i < write_count; i++) {
        std::string name = names[i];

        auto bindingInfo = shader_module_ptr->binding_names[name];

        if (!isBufferType(bindingInfo.second.second)) {
            throw std::runtime_error(
                "Target binding is not buffer binding"
            );
        }

        if (target_set->set_index != bindingInfo.first) {
            throw std::runtime_error(
                "Invalid set for this binding"
            );
        }

        vk::DescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = static_cast<RE_Buffer *>(buffers[i])->buffer;
        bufferInfo.range = sizes == nullptr ? vk::WholeSize : sizes[i];
        bufferInfo.offset = offsets == nullptr ? 0 : offsets[i];

        buffer_cache.push_back(bufferInfo);

        vk::WriteDescriptorSet descriptor_set_write = {};

        descriptor_set_write.dstSet = target_set->descriptor_set;
        descriptor_set_write.dstBinding = bindingInfo.second.first;
        descriptor_set_write.descriptorCount = 1;
        descriptor_set_write.descriptorType = bindingInfo.second.second;
        descriptor_set_write.pBufferInfo = &buffer_cache[i];

        writes.push_back(descriptor_set_write);
        target_set->bind_resources[bindingInfo.second.first] =
        {
            buffers[i],
            bindingInfo.second.second
        };
    }

    vkb_device_lock.lock();
    vk::Device _device = vkb_device.device;
    _device.updateDescriptorSets(
        writes.size(),
        writes.data(),
        0,
        nullptr
    );
    vkb_device_lock.unlock();
}

EXPORT_RE void re_write_set_images(
    RE_pDescriptorSet set,
    size_t write_count,
    const char **names,
    RE_pImage *images
) {
    auto *shader_module_ptr = static_cast<RE_DescriptorSet *>(set)->pool->shader_module;
    auto *target_set = static_cast<RE_DescriptorSet *>(set);

    std::vector<vk::WriteDescriptorSet> writes;
    std::vector<vk::DescriptorImageInfo> image_cache;
    writes.reserve(write_count);
    image_cache.reserve(write_count);

    for (uint32_t i = 0; i < write_count; i++) {
        std::string name = names[i];

        auto bindingInfo = shader_module_ptr->binding_names[name];

        if (!isImageType(bindingInfo.second.second)) {
            throw std::runtime_error(
                "Target binding is not image binding"
            );
        }

        if (target_set->set_index != bindingInfo.first) {
            throw std::runtime_error(
                "Invalid set for this binding"
            );
        }

        vk::DescriptorImageInfo imageInfo = {};
        imageInfo.imageView = static_cast<RE_Image *>(images[i])->view;
        imageInfo.imageLayout = bindingInfo.second.second == vk::DescriptorType::eCombinedImageSampler
                                    ? vk::ImageLayout::eShaderReadOnlyOptimal
                                    : vk::ImageLayout::eGeneral;
        if (bindingInfo.second.second == vk::DescriptorType::eCombinedImageSampler) {
            imageInfo.sampler = static_cast<RE_Image *>(images[i])->sampler;
        }

        image_cache.push_back(imageInfo);

        vk::WriteDescriptorSet descriptor_set_write = {};

        descriptor_set_write.dstSet = target_set->descriptor_set;
        descriptor_set_write.dstBinding = bindingInfo.second.first;
        descriptor_set_write.descriptorCount = 1;
        descriptor_set_write.descriptorType = bindingInfo.second.second;
        descriptor_set_write.pImageInfo = &image_cache[i];

        writes.push_back(descriptor_set_write);
        target_set->bind_resources[bindingInfo.second.first] =
        {
            images[i],
            bindingInfo.second.second
        };
    }

    vkb_device_lock.lock();
    vk::Device _device = vkb_device.device;
    _device.updateDescriptorSets(
        writes.size(),
        writes.data(),
        0,
        nullptr
    );
    vkb_device_lock.unlock();
}

//TODO add several pool deallocation feature
EXPORT_RE void re_free_descriptor_sets(
    uint32_t set_count,
    RE_pDescriptorSet *descriptor_set
) {
    if (set_count == 0) {
        return;
    }

    std::vector<vk::DescriptorSet> sets;
    sets.reserve(set_count);

    std::optional<RE_DescriptorPool *> pool = {};

    for (uint32_t i = 0; i < set_count; i++) {
        RE_DescriptorSet *set = static_cast<RE_DescriptorSet *>(descriptor_set[i]);

        if (pool.has_value() && static_cast<void *>(pool.value()) != static_cast<void *>(set->pool)) {
            throw std::invalid_argument("All sets must be within same pool");
        }

        pool = set->pool;

        sets.push_back(set->descriptor_set);
    }

    vkb_device_lock.lock();
    vk::Device _device = vkb_device.device;

    _device.freeDescriptorSets(
        pool.value()->descriptor_pool,
        set_count,
        sets.data()
    );

    pool.value()->allocated_layouts -= set_count;
    vkb_device_lock.unlock();
}
