//
// Created by Роман  Тимофеев on 22.05.2026.
//
module;

#include <re_typedefs.h>
#include "export_macro.h"
#include <vk_mem_alloc.h>
#include "uid.h"

export module image;
export import vulkan;

export struct RE_Image {
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

export EXPORT_RE RE_pImage re_create_image(
    uint32_t width,
    uint32_t height,
    RE_IMAGE_FORMATS format,
    bool linear_filtering,
    bool repeat_u,
    bool repeat_v
);

export EXPORT_RE void re_get_image_dimensions(
    RE_pImage image,
    uint32_t *width,
    uint32_t *height
);

export EXPORT_RE void re_free_image(
    RE_pImage image
);
