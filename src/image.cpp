//
// Created by Onion on 22.07.2026.
//

module;

#include <re_typedefs.h>
#include "export_macro.h"
#include <vk_mem_alloc.h>
#include <VkBootstrap.h>
#include "uid.h"

#if defined(__APPLE__)
#include <vulkan/vulkan.hpp>
#include <ranges>
#endif

module Image;

#if defined(__linux__)
import vulkan;
import std;
#endif
import render_engine_shares;
import render_engine;
import synchronization;
import command_encoders;

using namespace RenderEngine;

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

    return vk::Format::eR8G8B8A8Srgb;
}

Image::Image(
    uint32_t width,
    uint32_t height,
    RE_IMAGE_FORMATS format,
    bool linear_filtering,
    bool repeat_u,
    bool repeat_v
) {
    auto real_format = image_format_re_to_vk(format);

    vk::ImageCreateInfo image_create_info{};
    image_create_info.format = real_format;
    if (format == RE_IMAGE_FORMAT_DEPTH) {
        image_create_info.usage =
            vk::ImageUsageFlagBits::eDepthStencilAttachment |
            vk::ImageUsageFlagBits::eSampled;
    } else {
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

    _uid = gen_uid();
    _format = real_format;
    _height = height;
    _width = width;
    _img = image;
    _allocation = allocation;
    vkb_device_lock.lock();
    _view = device.createImageView(view_create_info);
    _sampler = device.createSampler(sampler_create_info);
    vkb_device_lock.unlock();

    _combined_img_sampler.imageView = _view;
    _combined_img_sampler.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    _combined_img_sampler.sampler = _sampler;

    _storage_img.imageView = _view;
    _storage_img.imageLayout = vk::ImageLayout::eGeneral;
}

Image::Image()
{
    _uid = gen_uid();
    _swap_chain = true;

    _combined_img_sampler.imageView = _view;
    _combined_img_sampler.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    _combined_img_sampler.sampler = _sampler;

    _storage_img.imageView = _view;
    _storage_img.imageLayout = vk::ImageLayout::eGeneral;
}

vk::WriteDescriptorSet Image::get_descriptor_set_write(vk::DescriptorType type)
{
    vk::WriteDescriptorSet set = {};
    set.descriptorType = type;
    set.descriptorCount = 1;
    set.pImageInfo =
        type == vk::DescriptorType::eCombinedImageSampler ?
        &_combined_img_sampler : &_storage_img;
    return set;
}

Image::~Image()
{
    vk::Device device = vkb_device.device;

    if (_swap_chain) {
        return;
    }

    vkb_device_lock.lock();
    device.destroy(_sampler);

    device.destroy(_view);

    vmaDestroyImage(
        vma_allocator,
        _img,
        _allocation
    );
    vkb_device_lock.unlock();
}

void Image::present()
{
    if (!_swap_chain)
    {
        return;
    }

    vk::Device device = vkb_device.device;

    auto semaphore = ResourceFrame::transfer_images_layout(
        std::views::single(std::pair{this, I_PRESENT})
    );

    vk::SwapchainKHR swapchain = vkb_swap_chain.swapchain;

    vk::PresentInfoKHR present_info = {};
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &semaphore;
    present_info.pSwapchains = &swapchain;
    present_info.swapchainCount = 1;
    present_info.pImageIndices = &_sc_index;

    vkb_device_lock.lock();

    try
    {
        auto result = vk_queue.presentKHR(present_info);
        if (result != vk::Result::eSuccess)
        {
            vkb_device_lock.unlock();
            free_semaphore(*get_semaphore_ref());
            return;
        }
    }catch (vk::OutOfDateKHRError& _)
    {
        vkb_device_lock.unlock();
        free_semaphore(*get_semaphore_ref());
        return;
    }

    vkb_device_lock.unlock();

    free_semaphore(*get_semaphore_ref());
}

std::shared_ptr<Image> Image::get_swapchain_image()
{

    vk::Device vk_device = vkb_device.device;

swap_chain_accssing:

    if (next_image_semaphore >= vk_swap_chain_semaphores.size()) {
        next_image_semaphore = 0;
    }

    int64_t id = 0;

    try
    {
        auto result = vk_device.acquireNextImageKHR(
            vkb_swap_chain.swapchain,
            UINT64_MAX,
            vk_swap_chain_semaphores[next_image_semaphore],
            {}
        );
        id = result.value;
    }catch (vk::OutOfDateKHRError& _)
    {
        id = -1;
    }

    if (id == -1) {
        wait_device_free();
        init_swap_chain(true);
        populate_swapchain();
        goto swap_chain_accssing;
    }

    auto image = std::shared_ptr<Image>(new Image());
    image->_img = vk_swap_chain_images[id];
    image->_view = vk_swap_chain_views[id];
    image->_layout = vk::ImageLayout::eUndefined;
    image->_uid = gen_uid();

    image->_semaphore = &vk_swap_chain_semaphores[next_image_semaphore];
    image->_width = vkb_swap_chain.extent.width;
    image->_height = vkb_swap_chain.extent.height;
    image->_format = static_cast<vk::Format>(vkb_swap_chain.image_format);
    image->_sc_index = id;

    next_image_semaphore++;

    return image;
}
