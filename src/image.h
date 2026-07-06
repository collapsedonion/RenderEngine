//
// Created by Роман  Тимофеев on 22.05.2026.
//

#ifndef RENDERENGINE_IMAGE_H
#define RENDERENGINE_IMAGE_H

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>
#include "uid.h"

struct RE_Image {
    UID uid;

    uint32_t width;
    uint32_t height;
    vk::Format format;

    bool is_swapchain = false;
    uint32_t image_index;

    vk::Image image;
    vk::ImageView view;
    vk::Sampler sampler;

    VmaAllocation allocation;

    vk::ImageLayout last_layout = vk::ImageLayout::eUndefined;

    vk::Semaphore* semaphore = nullptr;
};

#endif //RENDERENGINE_IMAGE_H
