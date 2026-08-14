//
// Created by Onion on 22.07.2026.
//

module;

#include <re_typedefs.h>
#include <VkBootstrap.h>
#include "export_macro.h"

#if defined(__APPLE__)
#include <array>
#include <span>
#include <stdexcept>
#include <string>
#include <memory>
#include <thread>
#include <vector>
#include <functional>
#include <vulkan/vulkan.hpp>
#endif

module Transporters;

#if defined(__linux__)
import vulkan;
import std;
#endif
import command_encoders;
import render_engine_shares;

using namespace RenderEngine;

void RenderEngine::transfer_buffer_to_image(
    std::shared_ptr<RawBuffer>& src,
    std::shared_ptr<Image>& dst,
    std::function<void()> transfer_callback
) {
    vk::Device device = vkb_device.device;
    vk::CommandBufferAllocateInfo cb_alloc_info{};
    cb_alloc_info.commandBufferCount = 1;
    cb_alloc_info.commandPool = vk_transfer_command_pool;
    cb_alloc_info.level = vk::CommandBufferLevel::ePrimary;
    vk_pool_lock.lock();
    vk::CommandBuffer command_buffer =
            device.allocateCommandBuffers(
                cb_alloc_info
            )[0];
    vk_pool_lock.unlock();

    ResourceFrame rf{};

    vk::CommandBufferBeginInfo cb_begin_info{};
    cb_begin_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    vk_pool_lock.lock();
    command_buffer.begin(cb_begin_info);
    vk_pool_lock.unlock();

    rf.record_buffer_to_image_transport(
        command_buffer,
        src.get(),
        dst.get()
    );

    vk_pool_lock.lock();
    command_buffer.end();
    vk_pool_lock.unlock();

    auto new_semaphores = rf.get_semaphores();

    vk::Queue queue = vkb_device.get_queue(vkb::QueueType::graphics).value();

    vk::Fence fence = device.createFence(
        vk::FenceCreateInfo{}
    );

    vk::SubmitInfo submit_info{};
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;
    submit_info.waitSemaphoreCount = new_semaphores.second.first.size();
    submit_info.pWaitSemaphores = new_semaphores.second.first.data();
    submit_info.pWaitDstStageMask = new_semaphores.second.second.data();
    submit_info.pSignalSemaphores = &new_semaphores.first;
    submit_info.signalSemaphoreCount = 1;

    vkb_device_lock.lock();
    queue.submit(submit_info, fence);
    vkb_device_lock.unlock();

    rf.update_image_layouts();

    std::thread t([=]() {
        vk::Fence _fence = fence;
        auto _ = device.waitForFences(_fence, true, UINT64_MAX);

        vkb_device_lock.lock();
        device.destroyFence(_fence);
        auto _command_buffer = command_buffer;
        vk_pool_lock.lock();
        device.freeCommandBuffers(vk_transfer_command_pool, {_command_buffer});
        vk_pool_lock.unlock();
        transfer_callback();
        vkb_device_lock.unlock();
    });

    t.detach();
}

void RenderEngine::transfer_image_to_image(
     Iterator<ImageToImageInfo>& image_transfers
)
{
    vk::Device device = vkb_device.device;
    vk::CommandBufferAllocateInfo cb_alloc_info{};
    cb_alloc_info.commandBufferCount = 1;
    cb_alloc_info.commandPool = vk_transfer_command_pool;
    cb_alloc_info.level = vk::CommandBufferLevel::ePrimary;

    vk_pool_lock.lock();
    vk::CommandBuffer command_buffer =
            device.allocateCommandBuffers(
                cb_alloc_info
            )[0];
    vk_pool_lock.unlock();

    ResourceFrame rf{};

    vk::CommandBufferBeginInfo cb_begin_info{};
    cb_begin_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    vk_pool_lock.lock();
    command_buffer.begin(cb_begin_info);
    vk_pool_lock.unlock();

    while (image_transfers) {
        auto& t = image_transfers.next();
        auto src = t.from_image.lock();
        auto dst = t.to_image.lock();
        uint32_t from_w = t.from_width  ? t.from_width  : src->width()  - t.from_offset_x;
        uint32_t from_h = t.from_height ? t.from_height : src->height() - t.from_offset_y;
        uint32_t to_w   = t.to_width    ? t.to_width    : dst->width()  - t.to_offset_x;
        uint32_t to_h   = t.to_height   ? t.to_height   : dst->height() - t.to_offset_y;
        rf.record_image_transport(
            command_buffer,
            src.get(), dst.get(),
            {t.from_offset_x, t.from_offset_y},
            {from_w, from_h},
            {t.to_offset_x,   t.to_offset_y},
            {to_w,   to_h}
        );
    }

    vk_pool_lock.lock();
    command_buffer.end();
    vk_pool_lock.unlock();

    rf.update_image_layouts();

    auto new_semaphores = rf.get_semaphores();

    vk::Queue queue = vkb_device.get_queue(vkb::QueueType::graphics).value();

    vk::Fence fence = device.createFence(
        vk::FenceCreateInfo{}
    );

    vk::SubmitInfo submit_info{};
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;
    submit_info.waitSemaphoreCount = new_semaphores.second.first.size();
    submit_info.pWaitSemaphores = new_semaphores.second.first.data();
    submit_info.pWaitDstStageMask = new_semaphores.second.second.data();
    submit_info.pSignalSemaphores = &new_semaphores.first;
    submit_info.signalSemaphoreCount = 1;

    vkb_device_lock.lock();
    queue.submit(submit_info, fence);
    vkb_device_lock.unlock();

    std::thread t([=]() {
        vk::Fence _fence = fence;
        auto _ = device.waitForFences(_fence, true, UINT64_MAX);

        vkb_device_lock.lock();
        device.destroyFence(_fence);
        auto _command_buffer = command_buffer;
        vk_pool_lock.lock();
        device.freeCommandBuffers(vk_transfer_command_pool, {_command_buffer});
        vk_pool_lock.unlock();
        vkb_device_lock.unlock();
    });

    t.detach();
}

void RenderEngine::transfer_buffer_to_buffer(
    Iterator<BufferToBufferInfo>& buffer_transfers,
    std::function<void()> transfer_callback
) {
    vk::Device device = vkb_device.device;
    vk::CommandBufferAllocateInfo cb_alloc_info{};
    cb_alloc_info.commandBufferCount = 1;
    cb_alloc_info.commandPool = vk_transfer_command_pool;
    cb_alloc_info.level = vk::CommandBufferLevel::ePrimary;
    vk_pool_lock.lock();
    vk::CommandBuffer command_buffer =
            device.allocateCommandBuffers(
                cb_alloc_info
            )[0];
    vk_pool_lock.unlock();

    ResourceFrame rf{};

    vk::CommandBufferBeginInfo cb_begin_info{};
    cb_begin_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    vk_pool_lock.lock();
    command_buffer.begin(cb_begin_info);
    vk_pool_lock.unlock();

    while (buffer_transfers){
        auto ti = buffer_transfers.next();
        rf.record_buffers_transport(command_buffer, ti);
    }

    vk_pool_lock.lock();
    command_buffer.end();
    vk_pool_lock.unlock();

    auto new_semaphores = rf.get_semaphores();

    vk::Queue queue = vkb_device.get_queue(vkb::QueueType::graphics).value();

    vk::Fence fence = device.createFence(
        vk::FenceCreateInfo{}
    );

    vk::SubmitInfo submit_info{};
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;
    submit_info.waitSemaphoreCount = new_semaphores.second.first.size();
    submit_info.pWaitSemaphores = new_semaphores.second.first.data();
    submit_info.pWaitDstStageMask = new_semaphores.second.second.data();
    submit_info.pSignalSemaphores = &new_semaphores.first;
    submit_info.signalSemaphoreCount = 1;

    vkb_device_lock.lock();
    queue.submit(submit_info, fence);
    vkb_device_lock.unlock();

    std::thread t([=]() {
        vk::Fence _fence = fence;
        auto _ = device.waitForFences(_fence, true, UINT64_MAX);

        vkb_device_lock.lock();
        device.destroyFence(_fence);
        auto _command_buffer = command_buffer;
        vk_pool_lock.lock();
        device.freeCommandBuffers(vk_transfer_command_pool, {_command_buffer});
        vk_pool_lock.unlock();
        vkb_device_lock.unlock();
        transfer_callback();
    });

    t.detach();
}
