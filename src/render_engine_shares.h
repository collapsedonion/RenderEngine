//
// Created by Роман  Тимофеев on 29.04.2026.
//

#ifndef RENDERENGINE_RENDER_ENGINE_SHARES_H
#define RENDERENGINE_RENDER_ENGINE_SHARES_H

#include <mutex>
#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>
#include <VkBootstrap.h>

extern vkb::Instance vkb_instance;
extern vkb::PhysicalDevice vkb_physical_device;
extern vk::SurfaceKHR vk_surface;
extern vkb::Device vkb_device;
extern std::mutex vkb_device_lock;
extern vk::Queue vk_queue;
extern vkb::Swapchain vkb_swap_chain;

extern VmaAllocator vma_allocator;

extern vk::CommandPool vk_transfer_command_pool;
extern vk::CommandPool vk_render_command_pool;
extern std::mutex vk_pool_lock;

extern vk::DescriptorSetLayout vk_empty_descriptor_set_layout;

#endif //RENDERENGINE_RENDER_ENGINE_SHARES_H
