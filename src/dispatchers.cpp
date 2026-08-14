//
// Created by Onion on 12.08.2026.
//
module;
#include <VkBootstrap.h>

#ifdef __APPLE__
#include <cinttypes>
#include <memory>
#include <coroutine>
#include <vulkan/vulkan.hpp>
#include <thread>
#endif

module Dispatchers;

#ifdef __linux__
import std;
import vulkan;
#endif
import render_engine_shares;
import command_encoders;

using namespace RenderEngine;

void RenderEngine::dispatch_compute_shader(
    std::shared_ptr<ShaderModule> shader_module,
    std::string& shader_name,
    Iterator<DescriptorPool::DescriptorSet*>& sets,
    std::uint32_t group_x,
    std::uint32_t group_y,
    std::uint32_t group_z
)
{
    ResourceFrame rf = {};

    vk::Device device = vkb_device.device;

    vk::CommandBufferAllocateInfo command_buffer_allocate_info{};
    command_buffer_allocate_info.commandBufferCount = 1;
    command_buffer_allocate_info.commandPool = vk_render_command_pool;
    command_buffer_allocate_info.level = vk::CommandBufferLevel::ePrimary;

    vk_pool_lock.lock();
    vk::CommandBuffer command_buffer = device.allocateCommandBuffers(
        command_buffer_allocate_info
    )[0];
    vk_pool_lock.unlock();

    vk::CommandBufferBeginInfo begin_info{};
    begin_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    vk_pool_lock.lock();
    command_buffer.begin(begin_info);
    vk_pool_lock.unlock();

    std::vector<vk::DescriptorSet> vk_sets = rf.extractDescriptorSets(
        command_buffer,
        sets.to_generator()
    );

    rf.record_compute_shader_submit(
        command_buffer,
        shader_module.get(),
        std::string(shader_name),
        vk_sets.size(),
        vk_sets.data(),
        group_x,
        group_y,
        group_z
    );

    vk_pool_lock.lock();
    command_buffer.end();
    vk_pool_lock.unlock();

    auto semaphores = rf.get_semaphores();

    vk::SubmitInfo submit_info{};
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;
    submit_info.waitSemaphoreCount = semaphores.second.first.size();
    submit_info.pWaitSemaphores = semaphores.second.first.data();
    submit_info.pWaitDstStageMask = semaphores.second.second.data();
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &semaphores.first;

    vk::Queue queue = vkb_device.get_queue(vkb::QueueType::graphics).value();

    vk::FenceCreateInfo fence_info{};
    vkb_device_lock.lock();
    vk::Fence fence = device.createFence(fence_info);

    auto _ = queue.submit(1, &submit_info, fence);

    vkb_device_lock.unlock();

    std::thread t(
        [=]() {
            vk::Fence t_fence = fence;
            vk::CommandBuffer t_buffer = command_buffer;
            auto _ = device.waitForFences(t_fence, true, UINT64_MAX);

            vkb_device_lock.lock();
            device.destroy(t_fence);
            vk_pool_lock.lock();
            device.freeCommandBuffers(vk_render_command_pool, 1, &t_buffer);
            vk_pool_lock.unlock();
            vkb_device_lock.unlock();
        }
    );

    t.detach();
}

void RenderEngine::dispatch_graphics_pipeline(
    std::shared_ptr<ShaderModule> shader_module,
    std::string& pipeline_name,
    Iterator<Image*>& target_images,
    Iterator<RenderObject>& render_objects,
    std::shared_ptr<Image> depth_image,
    bool load_image
)
{
    ResourceFrame rf{};

    auto ppl = shader_module->get_graphics_pipeline(pipeline_name);

    vk::Device device = vkb_device.device;

    vk::CommandBufferAllocateInfo command_buffer_allocate_info{};
    command_buffer_allocate_info.commandBufferCount = 1;
    command_buffer_allocate_info.commandPool = vk_render_command_pool;
    command_buffer_allocate_info.level = vk::CommandBufferLevel::ePrimary;

    vk::CommandBufferAllocateInfo secondary_command_buffers{};
    secondary_command_buffers.level = vk::CommandBufferLevel::eSecondary;
    secondary_command_buffers.commandPool = vk_render_command_pool;
    secondary_command_buffers.commandBufferCount = 2;

    vk_pool_lock.lock();
    vk::CommandBuffer command_buffer = device.allocateCommandBuffers(command_buffer_allocate_info)[0];
    auto secondary_buffers = device.allocateCommandBuffers(secondary_command_buffers);
    vk_pool_lock.unlock();

    vk::CommandBufferBeginInfo begin_info{};
    begin_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    vk_pool_lock.lock();
    command_buffer.begin(begin_info);
    vk_pool_lock.unlock();

    std::array<vk::CommandBuffer, 3> cbs = {command_buffer, secondary_buffers[0], secondary_buffers[1]};
    rf.record_render(
        cbs,
        shader_module.get(),
        pipeline_name,
        target_images.to_generator(),
        render_objects.to_generator(),
        depth_image ? depth_image.get() : nullptr,
        load_image
    );

    vk_pool_lock.lock();
    command_buffer.end();
    vk_pool_lock.unlock();
    rf.update_image_layouts();

    auto semaphores = rf.get_semaphores();

    vk::SubmitInfo submit_info{};
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;
    submit_info.waitSemaphoreCount = semaphores.second.first.size();
    submit_info.pWaitSemaphores = semaphores.second.first.data();
    submit_info.pWaitDstStageMask = semaphores.second.second.data();
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &semaphores.first;

    vk::Queue queue = vkb_device.get_queue(vkb::QueueType::graphics).value();

    vk::FenceCreateInfo fence_info{};
    vkb_device_lock.lock();
    vk::Fence fence = device.createFence(fence_info);

    auto _ =queue.submit(1, &submit_info, fence);

    vkb_device_lock.unlock();

    std::thread t(
        [=]() {
            vk::Fence t_fence = fence;
            vk::CommandBuffer t_buffer = command_buffer;
            vk::CommandBuffer s1 = secondary_buffers[0];
            vk::CommandBuffer s2 = secondary_buffers[1];
            auto _ = device.waitForFences(t_fence, true, UINT64_MAX);

            vkb_device_lock.lock();
            device.destroy(t_fence);
            vk_pool_lock.lock();
            device.freeCommandBuffers(vk_render_command_pool, 1, &t_buffer);
            device.freeCommandBuffers(vk_render_command_pool, 1, &s1);
            device.freeCommandBuffers(vk_render_command_pool, 1, &s2);
            vk_pool_lock.unlock();
            vkb_device_lock.unlock();
        }
    );

    t.detach();
}
