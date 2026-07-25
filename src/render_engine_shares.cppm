//
// Created by Роман  Тимофеев on 29.04.2026.
//

module;
#include <vk_mem_alloc.h>
#include <VkBootstrap.h>

#if defined(__APPLE__)
#include <mutex>
#include <vulkan/vulkan.hpp>
#endif

export module render_engine_shares;
#if defined(__linux__)
export import vulkan;
export import std;
#endif

export vkb::Instance vkb_instance;
export vkb::PhysicalDevice vkb_physical_device;
export vk::SurfaceKHR vk_surface;
export vkb::Device vkb_device;
export std::recursive_mutex vkb_device_lock {};
export vk::Queue vk_queue;
export vkb::Swapchain vkb_swap_chain;

export VmaAllocator vma_allocator;

export vk::CommandPool vk_transfer_command_pool;
export vk::CommandPool vk_render_command_pool;
export std::recursive_mutex vk_pool_lock {};

export vk::DescriptorSetLayout vk_empty_descriptor_set_layout;
