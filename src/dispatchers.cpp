//
// Created by Роман  Тимофеев on 21.05.2026.
//

#include <render_engine.h>
#include <thread>

#include "../headers/lib/re_typedefs.h"
#include "export_macro.h"
#include  <vulkan/vulkan.hpp>
#include <ranges>

#include "command_encoders.h"
#include "render_engine_shares.h"

EXPORT_RE void re_dispatch_compute_shader(
    const char *shader_name,
    RE_pShaderModule shader_module,
    uint32_t set_count,
    RE_pDescriptorSet *sets,
    uint32_t group_x,
    uint32_t group_y,
    uint32_t group_z
) {
    ResourceFrame rf{};

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

    std::vector<vk::DescriptorSet> vk_sets = extractDescriptorSets(
        command_buffer,
        &rf,
        set_count,
        reinterpret_cast<RE_DescriptorSet **>(sets)
    );

    record_compute_shader_submit(
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

    auto semaphores = get_semaphores(&rf);

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

EXPORT_RE void re_render(
    const char *pipeline_name,
    RE_pShaderModule shader_module,
    uint32_t image_count,
    RE_pImage *target_images,

    uint32_t render_object_count,
    RE_RenderObject *render_objects,


    RE_pImage *depth_image
) {
    ResourceFrame rf{};

    auto &ppl = static_cast<RE_ShaderModule *>(shader_module)->registered_graphics_pipelines[pipeline_name];

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
    vk::CommandBufferInheritanceInfo inheritance_info{};

    vk_pool_lock.lock();
    vk::CommandBuffer command_buffer = device.allocateCommandBuffers(
        command_buffer_allocate_info
    )[0];
    auto secondary_buffers = device.allocateCommandBuffers(
        secondary_command_buffers
    );
    vk_pool_lock.unlock();

    vk::CommandBufferBeginInfo begin_info{};
    begin_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    vk_pool_lock.lock();
    command_buffer.begin(begin_info);
    begin_info.pInheritanceInfo = &inheritance_info;
    secondary_buffers[0].begin(begin_info);
    secondary_buffers[1].begin(begin_info);
    vk_pool_lock.unlock();

    vk::RenderingInfo rendering_info{};
    vk::Rect2D rect{};
    rect.offset = vk::Offset2D(0, 0);
    uint32_t width = UINT32_MAX;
    uint32_t height = UINT32_MAX;

    std::vector<vk::RenderingAttachmentInfo> attachments;
    std::vector<std::pair<RE_Image *, USAGE_TYPE> > sync_images{};
    sync_images.reserve(image_count);

    for (uint32_t i = 0; i < image_count; i++) {
        RE_Image *image = static_cast<RE_Image *>(target_images[i]);
        sync_images.push_back({image, G_RENDER});
        height = std::min(height, image->height);
        width = std::min(width, image->width);
        vk::RenderingAttachmentInfo attachment_info{};
        attachment_info.imageView = image->view;
        attachment_info.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
        attachment_info.loadOp = vk::AttachmentLoadOp::eClear;
        attachment_info.storeOp = vk::AttachmentStoreOp::eStore;
        attachment_info.clearValue.color = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
        attachments.push_back(attachment_info);
    }

    vk::RenderingAttachmentInfo depth_attachment_info{};

    if (ppl.depth_enable) {
        sync_images.push_back({reinterpret_cast<RE_Image*>(*depth_image), G_DEPTH});
        rendering_info.pDepthAttachment = &depth_attachment_info;
        depth_attachment_info.loadOp = vk::AttachmentLoadOp::eClear;
        depth_attachment_info.storeOp = vk::AttachmentStoreOp::eStore;
        depth_attachment_info.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal;
        depth_attachment_info.imageView = reinterpret_cast<RE_Image*>(*depth_image)->view;
        depth_attachment_info.clearValue.depthStencil = vk::ClearDepthStencilValue(1.0,0);
    }

    process_images_sync(command_buffer, &rf, sync_images.data(), sync_images.size());

    rendering_info.pColorAttachments = attachments.data();
    rendering_info.colorAttachmentCount = attachments.size();
    rect.extent.width = width;
    rect.extent.height = height;
    rendering_info.renderArea = rect;
    rendering_info.layerCount = 1;

    update_image_layouts(rf);

    vk_pool_lock.lock();

    secondary_buffers[1].beginRendering(rendering_info);
    secondary_buffers[1].bindPipeline(vk::PipelineBindPoint::eGraphics, ppl.base_ppl.pipeline);
    vk_pool_lock.unlock();

    {
        vk::Viewport viewport{};
        viewport.height = rect.extent.height;
        viewport.width = rect.extent.width;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        viewport.x = 0;
        viewport.y = 0;
        vk::Rect2D scissor{};
        scissor.offset = vk::Offset2D(0, 0);
        scissor.extent = vk::Extent2D(viewport.width, viewport.height);

        vk_pool_lock.lock();
        secondary_buffers[1].setViewportWithCount(
            1,
            &viewport
        );
        secondary_buffers[1].setScissorWithCount(
            1,
            &scissor
        );
        vk_pool_lock.unlock();
    }

    for (uint32_t i = 0; i < render_object_count; i++) {
        auto &render_object = render_objects[i];
        RE_Buffer *buffer = static_cast<RE_Buffer *>(render_object.vertex_buffer);
        std::pair<RE_Buffer *, USAGE_TYPE> buffer_usage = {buffer, G_VERTEX};
        process_buffers_sync(
            secondary_buffers[0],
            &rf,
            &buffer_usage,
            1
        );

        std::vector<vk::DescriptorSet> vk_sets =
                extractDescriptorSets(
                    secondary_buffers[0],
                    &rf,
                    render_object.descriptor_set_count,
                    reinterpret_cast<RE_DescriptorSet **>(render_object.descriptor_sets)
                );

        vk::DeviceSize offset = 0;

        vk_pool_lock.lock();
        secondary_buffers[1].bindVertexBuffers(
            0,
            1,
            &buffer->buffer,
            &offset
        );
        secondary_buffers[1].bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics,
            static_cast<RE_ShaderModule *>(shader_module)->pipeline_layout,
            0,
            vk_sets.size(),
            vk_sets.data(),
            0,
            nullptr
        );
        secondary_buffers[1].draw(
            buffer->size / ppl.bytes_per_vertex,
            1,
            0,
            0
        );
        vk_pool_lock.unlock();
    }


    vk_pool_lock.lock();
    secondary_buffers[0].end();
    secondary_buffers[1].endRendering();
    secondary_buffers[1].end();

    command_buffer.executeCommands(2, secondary_buffers.data());
    command_buffer.end();
    vk_pool_lock.unlock();

    auto semaphores = get_semaphores(&rf);

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
