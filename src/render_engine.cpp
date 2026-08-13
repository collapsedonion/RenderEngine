//
// Created by Роман  Тимофеев on 27.04.2026.
//
module;
#include <VkBootstrap.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vk_mem_alloc.h>
#include "uid.h"
#include <re_typedefs.h>

#include <memory>

#include "export_macro.h"


#if defined(__APPLE__)
#include <iostream>
#include <format>
#include <print>
#include <ranges>
#include <stdexcept>
#include <utility>
#include <vector>
#include <vulkan/vulkan.hpp>
#endif

module render_engine;

#if defined(__linux__)
import std;
import vulkan;
#endif
import command_encoders;
import synchronization;
import render_engine_shares;
import Image;

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

void RenderEngine::init(
    GLFWwindow *window
) {
    uint32_t glfw_extension_count;
    const char **glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);


    auto instance_builder =
            vkb::InstanceBuilder()
            .set_app_name("Render Engine")
            .set_minimum_instance_version(1, 4)
            .enable_extensions(glfw_extension_count, glfw_extensions)
            .enable_validation_layers();

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

void RenderEngine::wait_device_free() {
    vk::Device device = vkb_device.device;
    vkb_device_lock.lock();
    device.waitIdle();
    vkb_device_lock.unlock();
}

void RenderEngine::release() {
    RenderEngine::wait_device_free();
    free_semaphores();
    vk::Device _device = vkb_device.device;

    for (auto &sem: vk_swap_chain_semaphores) {
        _device.destroy(sem);
    }
}
