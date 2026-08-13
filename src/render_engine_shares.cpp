//
// Created by Onion on 13.08.2026.
//
module;

#include <VkBootstrap.h>

module render_engine_shares;

#ifdef __linux__
import vulkan;
import std;
#endif

void init_swap_chain(bool recreate) {
    const vk::SurfaceFormatKHR desired_format = {
        vk::Format::eR8G8B8A8Srgb,
        vk::ColorSpaceKHR::eSrgbNonlinear
    };

    auto swap_chain_builder = vkb::SwapchainBuilder(
        vkb_device,
        vk_surface
    ).set_desired_format(desired_format);

    swap_chain_builder.add_image_usage_flags(VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

    if (recreate) {
        swap_chain_builder.set_old_swapchain(vkb_swap_chain);
        vkb_swap_chain.destroy_image_views(vk_swap_chain_views);
    }

    auto swap_chain = swap_chain_builder.build();

    if (!swap_chain.has_value()) {
        throw std::runtime_error(std::format("Failed to build swap chain: {}", swap_chain.error().message()));
    }

    vkb::destroy_swapchain(vkb_swap_chain);

    vkb_swap_chain = swap_chain.value();
}

void populate_swapchain() {
    auto images = vkb_swap_chain.get_images().value();
    auto image_views = vkb_swap_chain.get_image_views().value();
    vk_swap_chain_images.clear();
    vk_swap_chain_views.clear();

    vk_swap_chain_semaphores.reserve(vkb_swap_chain.image_count);


    vk::Device _device = vkb_device.device;

    for (auto semaphore: vk_swap_chain_semaphores) {
        vkb_device_lock.lock();
        _device.destroy(semaphore);
        vkb_device_lock.unlock();
    }

    vk_swap_chain_semaphores.clear();

    vk::SemaphoreCreateInfo sci{};

    for (size_t i = 0; i < vkb_swap_chain.image_count; i++) {
        vkb_device_lock.lock();
        vk_swap_chain_semaphores.emplace_back(_device.createSemaphore(sci));
        vkb_device_lock.unlock();
    }


    vk_swap_chain_images.reserve(images.size());
    vk_swap_chain_views.reserve(images.size());

    for (uint32_t i = 0; i < images.size(); i++) {
        vk_swap_chain_images.emplace_back(images[i]);
        vk_swap_chain_views.emplace_back(image_views[i]);
    }
}
