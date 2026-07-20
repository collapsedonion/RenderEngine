//
// Created by Роман  Тимофеев on 22.05.2026.
//
module;

#include <render_engine.h>
#include "export_macro.h"
#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>
#include "uid.h"

export module image;
import render_engine_shares;

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

vk::Format image_format_re_to_vk(RE_IMAGE_FORMATS formats) {
    switch (formats) {
        case RE_IMAGE_FORMAT_R8:
            return vk::Format::eR8Srgb;
        case RE_IMAGE_FORMAT_RGB8:
            return vk::Format::eR8G8B8Srgb;
        case RE_IMAGE_FORMAT_BGR8:
            return vk::Format::eB8G8R8Srgb;
        case RE_IMAGE_FORMAT_RGBA8:
            return vk::Format::eR8G8B8A8Srgb;
        case RE_IMAGE_FORMAT_DEPTH:
            return vk::Format::eD32Sfloat;
    }
}

export EXPORT_RE RE_pImage re_create_image(
    uint32_t width,
    uint32_t height,
    RE_IMAGE_FORMATS format,
    bool linear_filtering, // true - linear; false - nearest,
    bool repeat_u, //true - repeat; false - clamp to edge,
    bool repeat_v
) {
    auto real_format = image_format_re_to_vk(format);

    vk::ImageCreateInfo image_create_info{};
    image_create_info.format = real_format;
    if (format == RE_IMAGE_FORMAT_DEPTH) {
        image_create_info.usage =
            vk::ImageUsageFlagBits::eDepthStencilAttachment |
            vk::ImageUsageFlagBits::eSampled;
    }else {
        image_create_info.usage =
        vk::ImageUsageFlagBits::eColorAttachment |
        vk::ImageUsageFlagBits::eSampled |
        vk::ImageUsageFlagBits::eTransferSrc |
        vk::ImageUsageFlagBits::eTransferDst;
    }

    image_create_info.arrayLayers = 1;
    image_create_info.mipLevels = 1;
    image_create_info.extent.width = width;
    image_create_info.extent.height = height;
    image_create_info.extent.depth = 1;
    image_create_info.imageType = vk::ImageType::e2D;
    image_create_info.initialLayout = vk::ImageLayout::eUndefined;
    image_create_info.samples = vk::SampleCountFlagBits::e1;

    VkImageCreateInfo c_image_create_info = image_create_info;

    VmaAllocationCreateInfo allocation_create_info{};
    allocation_create_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VmaAllocation allocation{};
    VkImage image;

    vkb_device_lock.lock();
    vmaCreateImage(
        vma_allocator,
        &c_image_create_info,
        &allocation_create_info,
        &image,
        &allocation,
        nullptr
    );
    vkb_device_lock.unlock();

    vk::ImageViewCreateInfo view_create_info{};
    view_create_info.image = image;
    view_create_info.format = real_format;
    view_create_info.viewType = vk::ImageViewType::e2D;
    view_create_info.subresourceRange.layerCount = 1;
    view_create_info.subresourceRange.levelCount = 1;
    view_create_info.subresourceRange.baseMipLevel = 0;
    view_create_info.subresourceRange.baseArrayLayer = 0;
    view_create_info.subresourceRange.aspectMask =
            real_format == vk::Format::eD32Sfloat ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
    view_create_info.components.r = vk::ComponentSwizzle::eR;
    view_create_info.components.g = vk::ComponentSwizzle::eG;
    view_create_info.components.b = vk::ComponentSwizzle::eB;
    view_create_info.components.a = vk::ComponentSwizzle::eA;

    vk::SamplerCreateInfo sampler_create_info{};
    sampler_create_info.minFilter = linear_filtering ? vk::Filter::eLinear : vk::Filter::eNearest;
    sampler_create_info.magFilter = linear_filtering ? vk::Filter::eLinear : vk::Filter::eNearest;
    sampler_create_info.addressModeU = repeat_u ? vk::SamplerAddressMode::eRepeat : vk::SamplerAddressMode::eClampToEdge;
    sampler_create_info.addressModeV = repeat_v ? vk::SamplerAddressMode::eRepeat : vk::SamplerAddressMode::eClampToEdge;
    sampler_create_info.anisotropyEnable = false;
    sampler_create_info.unnormalizedCoordinates = false;
    sampler_create_info.compareEnable = false;
    sampler_create_info.borderColor = vk::BorderColor::eFloatTransparentBlack;

    vk::Device device = vkb_device.device;

    auto new_image = new RE_Image{};
    new_image->format = real_format;
    new_image->height = height;
    new_image->width = width;
    new_image->image = image;
    new_image->allocation = allocation;
    new_image->uid = gen_uid();
    vkb_device_lock.lock();
    new_image->view = device.createImageView(view_create_info);
    new_image->sampler = device.createSampler(sampler_create_info);
    vkb_device_lock.unlock();

    return new_image;
}

export EXPORT_RE void re_get_image_dimensions(
    RE_pImage image,
    uint32_t *width,
    uint32_t *height
) {
    auto _image = reinterpret_cast<RE_Image*>(image);
    *width = _image->width;
    *height = _image->height;
}

export EXPORT_RE void re_free_image(
    RE_pImage image
) {
    auto* _image = reinterpret_cast<RE_Image *>(image);

    vk::Device device = vkb_device.device;

    if (_image->is_swapchain) {
        return;
    }

    vkb_device_lock.lock();
    device.destroy(_image->sampler);

    device.destroy(_image->view);

    vmaDestroyImage(
        vma_allocator,
        _image->image,
        _image->allocation
    );
    vkb_device_lock.unlock();

    delete _image;
}
