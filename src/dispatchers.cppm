//
// Created by Роман  Тимофеев on 21.05.2026.
//

module;
#include <mutex>
#include <ranges>
#include <render_engine.h>
#include <thread>

#include "export_macro.h"
#include <re_typedefs.h>
#include  <vulkan/vulkan.hpp>
#include <VkBootstrap.h>


export module dispatchers;
import command_encoders;
import shader_module;
import render_engine_shares;
import storage_buffer;
import descriptor_pool;
import image;

export EXPORT_RE void re_dispatch_compute_shader(
    const char *shader_name,
    RE_pShaderModule shader_module,
    uint32_t set_count,
    RE_pDescriptorSet *sets,
    uint32_t group_x,
    uint32_t group_y,
    uint32_t group_z
) {
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
        std::span(reinterpret_cast<RE_DescriptorSet**>(sets), set_count)
    );

    rf.record_compute_shader_submit(
        command_buffer,
        reinterpret_cast<RE_ShaderModule *>(shader_module),
        std::string(shader_name),
        set_count,
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

    queue.submit(1, &submit_info, fence);

    vkb_device_lock.unlock();

    std::thread t(
        [=]() {
            vk::Fence t_fence = fence;
            vk::CommandBuffer t_buffer = command_buffer;
            device.waitForFences(t_fence, true, UINT64_MAX);

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

export EXPORT_RE void re_render(
    const char *pipeline_name,
    RE_pShaderModule shader_module,
    uint32_t image_count,
    RE_pImage *target_images,

    uint32_t render_object_count,
    RE_RenderObject *render_objects,

    RE_pImage *depth_image,

    bool load_image
) {
    ResourceFrame rf{};

    auto* _shader_module = static_cast<RE_ShaderModule*>(shader_module);
    auto& ppl = _shader_module->registered_graphics_pipelines[pipeline_name];

    if (ppl.required_image_bindings != image_count) {
        throw std::runtime_error("Invalid image count");
    }

    if (ppl.depth_enable && depth_image == nullptr) {
        throw std::runtime_error("Depth image required");
    }

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
        _shader_module,
        pipeline_name,
        std::span(reinterpret_cast<RE_Image**>(target_images), image_count),
        std::span(render_objects, render_object_count),
        depth_image ? reinterpret_cast<RE_Image*>(*depth_image) : nullptr,
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

    queue.submit(1, &submit_info, fence);

    vkb_device_lock.unlock();

    std::thread t(
        [=]() {
            vk::Fence t_fence = fence;
            vk::CommandBuffer t_buffer = command_buffer;
            vk::CommandBuffer s1 = secondary_buffers[0];
            vk::CommandBuffer s2 = secondary_buffers[1];
            device.waitForFences(t_fence, true, UINT64_MAX);

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

export EXPORT_RE void re_transfer_buffer_to_image(
    RE_pBuffer source_buffer,
    RE_pImage target_image,
    RE_OperationEndCallback end_callback,
    RE_CallbackContext context
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

    RE_Buffer *from = static_cast<RE_Buffer *>(source_buffer);
    RE_Image *to = static_cast<RE_Image *>(target_image);

    rf.record_buffer_to_image_transport(
        command_buffer,
        from,
        to
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
        device.waitForFences(_fence, true, UINT64_MAX);

        vkb_device_lock.lock();
        device.destroyFence(_fence);
        auto _command_buffer = command_buffer;
        vk_pool_lock.lock();
        device.freeCommandBuffers(vk_transfer_command_pool, {_command_buffer});
        vk_pool_lock.unlock();
        if (end_callback) {
            end_callback(context);
        }
        vkb_device_lock.unlock();
    });


    t.detach();
}

export EXPORT_RE void re_transfer_image_to_image(
    RE_ImageToImageTransfer* transfers,
    uint32_t transfer_count
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

    for (uint32_t i = 0; i < transfer_count; i++) {
        auto& t = transfers[i];
        auto* src = static_cast<RE_Image*>(t.from_image);
        auto* dst = static_cast<RE_Image*>(t.to_image);
        uint32_t from_w = t.from_width  ? t.from_width  : src->width  - t.from_offset_x;
        uint32_t from_h = t.from_height ? t.from_height : src->height - t.from_offset_y;
        uint32_t to_w   = t.to_width    ? t.to_width    : dst->width  - t.to_offset_x;
        uint32_t to_h   = t.to_height   ? t.to_height   : dst->height - t.to_offset_y;
        rf.record_image_transport(
            command_buffer,
            src, dst,
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
        device.waitForFences(_fence, true, UINT64_MAX);

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

export EXPORT_RE void re_transfer_buffers(
    RE_BufferToBufferTransfer *buffers_copies,
    size_t num_buffers_copies,
    RE_OperationEndCallback end_callback,
    RE_CallbackContext context
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

    for (size_t i = 0; i < num_buffers_copies; i++) {
        rf.record_buffers_transport(command_buffer, buffers_copies[i]);
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
        device.waitForFences(_fence, true, UINT64_MAX);

        vkb_device_lock.lock();
        device.destroyFence(_fence);
        auto _command_buffer = command_buffer;
        vk_pool_lock.lock();
        device.freeCommandBuffers(vk_transfer_command_pool, {_command_buffer});
        vk_pool_lock.unlock();
        vkb_device_lock.unlock();
        if (end_callback) {
            end_callback(context);
        }
    });


    t.detach();
}
