//
// Created by Роман  Тимофеев on 27.04.2026.
//

#include <VkBootstrap.h>
#include <format>
#include <print>
#include <vulkan/vulkan.hpp>
#include <mutex>

#define GLFW_INCLUDE_VULKAN
#include <iostream>
#include <GLFW/glfw3.h>
#include <vk_mem_alloc.h>

#include <render_engine.h>
#include <thread>

#include "command_encoders.h"
#include "synchronization.h"
#include "storage_buffer.h"

#include "export_macro.h"
#include "image.h"
#include "render_engine_shares.h"

vkb::Instance vkb_instance;
vkb::PhysicalDevice vkb_physical_device;
vk::SurfaceKHR vk_surface;
vkb::Device vkb_device;
std::mutex vkb_device_lock{};
vk::Queue vk_queue;
vkb::Swapchain vkb_swap_chain;
std::vector<vk::Image> vk_swap_chain_images;
std::vector<VkImageView> vk_swap_chain_views;
std::vector<vk::Semaphore> vk_swap_chain_semaphores{};

std::vector<RE_Image> re_swap_chain_images{};

uint32_t next_image_semaphore = 0;

VmaAllocator vma_allocator;

vk::CommandPool vk_transfer_command_pool;
vk::CommandPool vk_render_command_pool;
std::mutex vk_pool_lock{};

vk::DescriptorSetLayout vk_empty_descriptor_set_layout;

inline void init_swap_chain(bool recreate = false) {
    const vk::SurfaceFormatKHR desired_format = {
        vk::Format::eR8G8B8A8Srgb,
        vk::ColorSpaceKHR::eSrgbNonlinear
    };

    auto swap_chain_builder = vkb::SwapchainBuilder(
        vkb_device,
        vk_surface
    ).set_desired_format(desired_format);

    swap_chain_builder.add_image_usage_flags(VK_IMAGE_USAGE_STORAGE_BIT);

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

inline void init_command_pool() {
    auto device = vk::Device(vkb_device.device);

    auto command_pool_create_info = vk::CommandPoolCreateInfo{
    };

    command_pool_create_info.queueFamilyIndex = vkb_device.get_queue_index(vkb::QueueType::graphics).value();
    command_pool_create_info.flags = vk::CommandPoolCreateFlagBits::eTransient;

    auto command_pool = device.createCommandPool(
        command_pool_create_info
    );

    vk_transfer_command_pool = command_pool;

    command_pool_create_info.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

    command_pool = device.createCommandPool(
        command_pool_create_info
    );

    vk_render_command_pool = command_pool;
}

void populate_swapchain() {
    auto images = vkb_swap_chain.get_images().value();
    auto image_views = vkb_swap_chain.get_image_views().value();
    vk_swap_chain_images.clear();
    vk_swap_chain_views.clear();

    vk_swap_chain_semaphores.reserve(vkb_swap_chain.image_count);
    re_swap_chain_images.resize(vkb_swap_chain.image_count);

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

EXPORT_RE void init_render_engine(
    GLFWwindow *window
) {
    uint32_t glfw_extension_count;
    const char **glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);


    auto instance_builder =
            vkb::InstanceBuilder()
            .set_app_name("Render Engine")
            .set_minimum_instance_version(1, 4)
            .enable_extensions(glfw_extension_count, glfw_extensions)
            ;//.enable_validation_layers();

    auto instance = instance_builder.build();

    if (!instance.has_value()) {
        throw std::runtime_error(std::format(
            "failed to create instance: {}",
            instance.error().message()
        ));
    }

    vkb_instance = instance.value();

    VkSurfaceKHR surface;

    auto result = glfwCreateWindowSurface(
        vkb_instance.instance,
        window,
        nullptr,
        &surface
    );

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create surface");
    }

    vk_surface = vk::SurfaceKHR(surface);

    //auto features1_1 = VkPhysicalDeviceVulkan11Features{};

    auto features1_2 = VkPhysicalDeviceVulkan12Features{};
    features1_2.timelineSemaphore = true;

    auto features1_3 = VkPhysicalDeviceVulkan13Features{};
    features1_3.dynamicRendering = true;
    features1_3.synchronization2 = true;

    auto features1_4= VkPhysicalDeviceVulkan14Features{};
    features1_4.dynamicRenderingLocalRead = true;

    auto physical_device_selector = vkb::PhysicalDeviceSelector(
                vkb_instance)
            .add_required_extensions({
                VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
                VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,
                VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
                VK_KHR_MULTIVIEW_EXTENSION_NAME,
                VK_KHR_MAINTENANCE2_EXTENSION_NAME,
                VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
                VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME,
                VK_KHR_DYNAMIC_RENDERING_LOCAL_READ_EXTENSION_NAME
            })
            .set_required_features_12(features1_2)
            .set_required_features_13(features1_3)
            .set_required_features_14(features1_4)
            .set_surface(vk_surface);

    auto physical_device = physical_device_selector.select();

    if (!physical_device.has_value()) {
        throw std::runtime_error(std::format(
            "Failed to select physical device: {}",
            physical_device.error().message()));
    }

    vkb_physical_device = physical_device.value();

    std::print("Found GPU: {}", vkb_physical_device.name);
    std::flush(std::cout);

    auto device = vkb::DeviceBuilder(
        vkb_physical_device
    ).build();

    if (!device.has_value()) {
        throw std::runtime_error(std::format(
            "Failed to build device: {}",
            device.error().message()
        ));
    }

    vkb_device = device.value();

    vk::Device vk_device = vkb_device.device;
    VkQueue queue = vkb_device.get_queue(vkb::QueueType::graphics).value();

    vk_queue = vk::Queue(queue);

    init_swap_chain();
    populate_swapchain();
    init_command_pool();

    VmaVulkanFunctions vulkan_functions = {};
    vulkan_functions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    vulkan_functions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
    vulkan_functions.vkGetBufferMemoryRequirements2KHR =
            reinterpret_cast<PFN_vkGetBufferMemoryRequirements2KHR>(vkGetInstanceProcAddr(
                vkb_instance.instance, "vkGetBufferMemoryRequirements2KHR"));
    vulkan_functions.vkGetImageMemoryRequirements2KHR =
            reinterpret_cast<PFN_vkGetImageMemoryRequirements2KHR>(vkGetInstanceProcAddr(
                vkb_instance.instance, "vkGetImageMemoryRequirements2KHR"));

    VmaAllocatorCreateInfo vma_allocator_create_info = {};
    vma_allocator_create_info.vulkanApiVersion = VK_API_VERSION_1_4;
    vma_allocator_create_info.physicalDevice = vkb_physical_device.physical_device;
    vma_allocator_create_info.device = vkb_device.device;
    vma_allocator_create_info.instance = vkb_instance.instance;
    vma_allocator_create_info.pVulkanFunctions = &vulkan_functions;
    vma_allocator_create_info.vulkanApiVersion = VK_API_VERSION_1_4;

    vmaCreateAllocator(&vma_allocator_create_info, &vma_allocator);

    init_semaphores();

    vk::DescriptorSetLayoutCreateInfo empty_set_layout_create_info = {};
    empty_set_layout_create_info.bindingCount = 0;

    vk_empty_descriptor_set_layout = vk_device.createDescriptorSetLayout(
        empty_set_layout_create_info
    );
}

EXPORT_RE void re_free_render_engine() {
    re_wait_device_free();
    free_semaphores();
    vk::Device _device = vkb_device.device;

    for (auto &sem: vk_swap_chain_semaphores) {
        _device.destroy(sem);
    }
}

EXPORT_RE void re_wait_device_free() {
    vk::Device device = vkb_device.device;
    vkb_device_lock.lock();
    device.waitIdle();
    vkb_device_lock.unlock();
}

EXPORT_RE RE_pImage re_get_present_image() {
    vk::Device vk_device = vkb_device.device;

swap_chain_accssing:

    if (next_image_semaphore >= vk_swap_chain_semaphores.size()) {
        next_image_semaphore = 0;
    }

    int64_t id;

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
        init_swap_chain(true);
        populate_swapchain();
        goto swap_chain_accssing;
    }


    auto* image = &re_swap_chain_images[id];
    image->is_swapchain = true;
    image->image = vk_swap_chain_images[id];
    image->view = vk_swap_chain_views[id];
    image->last_layout = vk::ImageLayout::eUndefined;
    image->uid = gen_uid();

    image->semaphore = &vk_swap_chain_semaphores[next_image_semaphore];
    image->width = vkb_swap_chain.extent.width;
    image->height = vkb_swap_chain.extent.height;
    image->format = static_cast<vk::Format>(vkb_swap_chain.image_format);
    image->image_index = id;

    next_image_semaphore++;

    return image;
}

EXPORT_RE void re_present_image(
    RE_pImage image
) {
    auto *_image = reinterpret_cast<RE_Image *>(image);

    if (!_image->is_swapchain) {
        return;
    }

    vk::Device device = vkb_device.device;

    vk::CommandBufferAllocateInfo command_buffer_allocate_info = {};
    command_buffer_allocate_info.commandBufferCount = 1;
    command_buffer_allocate_info.commandPool = vk_render_command_pool;
    command_buffer_allocate_info.level = vk::CommandBufferLevel::ePrimary;

    vk_pool_lock.lock();
    auto cb = device.allocateCommandBuffers(command_buffer_allocate_info)[0];
    vk_pool_lock.unlock();

    auto command_buffer_begin_info = vk::CommandBufferBeginInfo{};
    command_buffer_begin_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    ResourceFrame rf = {};
    std::pair<RE_Image *, USAGE_TYPE> sync = {_image, I_PRESENT};

    vk_pool_lock.lock();
    cb.begin(command_buffer_begin_info);
    vk_pool_lock.unlock();
    process_images_sync(
        cb,
        &rf,
        &sync,
        1
    );
    vk_pool_lock.lock();
    cb.end();
    vk_pool_lock.unlock();

    rf.needed_semaphores.insert(&_image->semaphore);

    auto semaphores = get_semaphores(&rf);

    vk::SubmitInfo submit_info = {};
    submit_info.pWaitSemaphores = semaphores.second.first.data();
    submit_info.pWaitDstStageMask = semaphores.second.second.data();
    submit_info.waitSemaphoreCount = semaphores.second.second.size();
    submit_info.pSignalSemaphores = &semaphores.first;
    submit_info.signalSemaphoreCount = 1;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cb;

    auto fence_create_info = vk::FenceCreateInfo{};

    vkb_device_lock.lock();
    auto fence = device.createFence(fence_create_info);
    vk_queue.submit(submit_info, fence);
    vkb_device_lock.unlock();

    vk::SwapchainKHR swapchain = vkb_swap_chain.swapchain;

    vk::PresentInfoKHR present_info = {};
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &semaphores.first;
    present_info.pSwapchains = &swapchain;
    present_info.swapchainCount = 1;
    present_info.pImageIndices = &_image->image_index;

    vkb_device_lock.lock();

    auto free_call =
        [=](bool wait) {
            auto _fence = fence;
            auto _cb = cb;
            if (wait)
            {
                device.waitForFences({_fence}, true, UINT64_MAX);
            }

            vkb_device_lock.lock();
            device.destroy(_fence);
            vk_pool_lock.lock();
            device.freeCommandBuffers(vk_render_command_pool, {_cb});
            vk_pool_lock.unlock();
            vkb_device_lock.unlock();
    };

    try
    {
        auto result = vk_queue.presentKHR(present_info);
        if (result != vk::Result::eSuccess)
        {
            vkb_device_lock.unlock();
            free_call(false);
            free_semaphore(_image->semaphore);
            return;
        }
    }catch (vk::OutOfDateKHRError& _)
    {
        vkb_device_lock.unlock();
        free_call(false);
        free_semaphore(_image->semaphore);
        return;
    }


    vkb_device_lock.unlock();

    free_semaphore(_image->semaphore);

    std::thread t = std::thread(
       free_call,
       true
    );

    t.detach();
}
