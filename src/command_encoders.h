//
// Created by Роман  Тимофеев on 11.05.2026.
//

#ifndef RENDERENGINE_COMMAND_ENCODERS_H
#define RENDERENGINE_COMMAND_ENCODERS_H
#include <set>
#include <unordered_map>

#include <vulkan/vulkan.hpp>

#include "descriptor_pool.h"
#include "shader_module.h"
#include "storage_buffer.h"
#include "image.h"
#include "uid.h"

enum USAGE_TYPE {
    T_READ,
    T_WRITE,
    C_STORAGE,
    C_UNIFORM,
    I_PRESENT,
    I_SAMPLED,
    G_RENDER,
    G_VERTEX,
    G_DEPTH
};

struct ResourceFrame {
    std::unordered_map<UID, USAGE_TYPE> used_buffers;
    //Usage type, changed layout
    std::unordered_map<RE_Image*, std::pair<USAGE_TYPE, vk::ImageLayout>> used_images;

    std::set<vk::Semaphore **> needed_semaphores;
};

void process_buffers_sync(
    vk::CommandBuffer command_buffer,
    ResourceFrame *resource_frame,
    std::pair<RE_Buffer *, USAGE_TYPE> *buffers, //buffer, new usage
    size_t buffer_count
);

void process_images_sync(
    vk::CommandBuffer command_buffer,
    ResourceFrame *resource_frame,
    std::pair<RE_Image *, USAGE_TYPE> *images, //image, new usage
    size_t buffer_count
);

void update_image_layouts(
    const ResourceFrame& rf
);

void record_buffers_transport(
    vk::CommandBuffer command_buffer,
    RE_Buffer *from_buffer,
    RE_Buffer *to_buffer,
    size_t size_to_copy,
    size_t from_index,
    size_t to_index
);

void record_buffer_to_image_transport(
    const ResourceFrame& rf,
    vk::CommandBuffer command_buffer,
    RE_Buffer *from_buffer,
    RE_Image *to_image
);

void record_compute_shader_submit(
    vk::CommandBuffer command_buffer,
    RE_ShaderModule *shader_module,
    const std::string &shader_name,
    size_t descriptor_set_count,
    vk::DescriptorSet *descriptor_sets,
    size_t groupCountX, size_t groupCountY,
    size_t groupCountZ
);

std::vector<vk::DescriptorSet> extractDescriptorSets(
    vk::CommandBuffer command_buffer,
    ResourceFrame *resource_frame,
    size_t set_count,
    RE_DescriptorSet **descriptor_sets
);

std::pair<
    vk::Semaphore,
    std::pair<std::vector<vk::Semaphore>, std::vector<vk::PipelineStageFlags> >
> get_semaphores(ResourceFrame *resource_frame);

#endif //RENDERENGINE_COMMAND_ENCODERS_H
