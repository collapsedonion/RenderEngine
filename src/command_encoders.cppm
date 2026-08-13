//
// Created by Роман  Тимофеев on 11.05.2026.
//
module;

#include "re_typedefs.h"

#include "uid.h"

#if defined(__APPLE__)
#include <algorithm>
#include <array>
#include <concepts>
#include <mutex>
#include <ranges>
#include <set>
#include <span>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>
#include <vulkan/vulkan.hpp>
#endif

export module command_encoders;
#if defined(__linux__)
export import vulkan;
import std;
#endif
import Buffer;
import Image;
import DescriptorPool;
import ShaderModule;
import RenderObject;
import render_engine_shares;
import synchronization;
import Transporters;

export enum USAGE_TYPE
{
    T_READ,
    T_WRITE,
    C_STORAGE,
    C_UNIFORM,
    I_PRESENT,
    I_SAMPLED,
    G_RENDER,
    G_VERTEX,
    G_DEPTH
};

inline vk::AccessFlags2 getAccessBits(
    USAGE_TYPE usage
)
{
    switch (usage)
    {
    case T_READ:
        return vk::AccessFlagBits2::eTransferRead;
    case T_WRITE:
        return vk::AccessFlagBits2::eTransferWrite;
    case C_STORAGE:
        return vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite;
    case C_UNIFORM:
        return vk::AccessFlagBits2::eShaderRead;
    case I_PRESENT:
        return vk::AccessFlagBits2::eTransferRead | vk::AccessFlagBits2::eShaderRead;
    case I_SAMPLED:
        return vk::AccessFlagBits2::eShaderSampledRead | vk::AccessFlagBits2::eShaderRead;
    case G_RENDER:

    case G_VERTEX:
        return vk::AccessFlagBits2::eShaderRead;
    case G_DEPTH:
        return vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite;
    }

    return {};
}

inline vk::AccessFlags2 getImageAccessBits(
    vk::ImageLayout layout
)
{
    switch (layout)
    {
    case vk::ImageLayout::eTransferSrcOptimal:
        return vk::AccessFlagBits2::eTransferRead;
    case vk::ImageLayout::eTransferDstOptimal:
        return vk::AccessFlagBits2::eTransferWrite;
    case vk::ImageLayout::eGeneral:
        return
            vk::AccessFlagBits2::eShaderRead |
            vk::AccessFlagBits2::eShaderWrite;
    case vk::ImageLayout::eUndefined:
        return vk::AccessFlagBits2::eNone;
    case vk::ImageLayout::eShaderReadOnlyOptimal:
        return vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderSampledRead;
    case vk::ImageLayout::eColorAttachmentOptimal:
        return vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite;
    case vk::ImageLayout::eDepthAttachmentOptimal:
        return vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite;
    case vk::ImageLayout::ePresentSrcKHR:
        return vk::AccessFlagBits2::eTransferRead | vk::AccessFlagBits2::eShaderRead;
    default:
        break;
    }

    return {};
}

inline vk::ImageLayout getImageLayout(
    USAGE_TYPE usage_type
)
{
    switch (usage_type)
    {
    case T_READ:
        return vk::ImageLayout::eTransferSrcOptimal;
    case T_WRITE:
        return vk::ImageLayout::eTransferDstOptimal;
    case C_STORAGE:
        return vk::ImageLayout::eGeneral;
    case C_UNIFORM:
        break;
    case I_PRESENT:
        return vk::ImageLayout::ePresentSrcKHR;
    case I_SAMPLED:
        return vk::ImageLayout::eShaderReadOnlyOptimal;
    case G_RENDER:
        return vk::ImageLayout::eColorAttachmentOptimal;
    case G_DEPTH:
        return vk::ImageLayout::eDepthAttachmentOptimal;
    }

    return  vk::ImageLayout::eUndefined;
}

export class ResourceFrame
{
    std::unordered_map<UID, USAGE_TYPE> used_buffers = {};
    //Usage type, changed layout
    std::unordered_map<RenderEngine::Image*, std::pair<USAGE_TYPE, vk::ImageLayout>> used_images = {};

    std::set<vk::Semaphore**> needed_semaphores = {};

public:
    vk::ImageLayout get_image_actual_image_layout(
        RenderEngine::Image* pImage
    );

    void record_buffers_transport(
        vk::CommandBuffer command_buffer,
        const RenderEngine::BufferToBufferInfo& transfer
    );

    void record_compute_shader_submit(
        vk::CommandBuffer command_buffer,
        RenderEngine::ShaderModule* shader_module,
        const std::string& shader_name,
        size_t descriptor_set_count,
        vk::DescriptorSet* descriptor_sets,
        size_t groupCountX,
        size_t groupCountY,
        size_t groupCountZ
    );

    std::pair<
        vk::Semaphore,
        std::pair<std::vector<vk::Semaphore>, std::vector<vk::PipelineStageFlags>>
    > get_semaphores();

    template <std::ranges::input_range I>
        requires std::convertible_to<std::ranges::range_value_t<I>, RenderEngine::DescriptorPool::DescriptorSet*>
    std::vector<vk::DescriptorSet> extractDescriptorSets(
        vk::CommandBuffer command_buffer,
        I descriptor_sets
    )
    {
        std::vector<vk::DescriptorSet> _descriptor_sets;
        std::vector<std::pair<RenderEngine::Image*, USAGE_TYPE>> images_to_sync{};

        for (RenderEngine::DescriptorPool::DescriptorSet* descriptor_set : descriptor_sets)
        {
            for (
                auto& [_, res] :
                descriptor_set->get_bound_resources())
            {
                auto [resource, type] = res;
                switch (type)
                {
                case vk::DescriptorType::eUniformBuffer:
                    {
                        this->used_buffers[resource->get_uid()] = USAGE_TYPE::C_UNIFORM;
                        break;
                    }
                case vk::DescriptorType::eStorageBuffer:
                    {
                        this->used_buffers[resource->get_uid()] = USAGE_TYPE::C_STORAGE;
                        break;
                    }
                case vk::DescriptorType::eStorageImage:
                    {
                        if (auto* image = dynamic_cast<RenderEngine::Image*>(resource))
                        {
                            images_to_sync.emplace_back(image, USAGE_TYPE::C_STORAGE);
                        }
                        break;
                    }

                case vk::DescriptorType::eCombinedImageSampler:
                    {
                        if (auto* image = dynamic_cast<RenderEngine::Image*>(resource))
                        {
                            images_to_sync.emplace_back(image, USAGE_TYPE::I_SAMPLED);
                        }
                        break;
                    }

                default:

                    break;
                }
                this->needed_semaphores.insert(resource->get_semaphore_ref());
            }

            _descriptor_sets.push_back(descriptor_set->get_set());
        }

        process_images_sync(
            command_buffer,
            images_to_sync
        );

        return _descriptor_sets;
    }

    void update_image_layouts();

    // command_buffers: [0]=primary, [1]=secondary_sync, [2]=secondary_draw
    template <
        std::ranges::random_access_range CB,
        std::ranges::input_range TI,
        std::ranges::input_range RO
    >
        requires std::same_as<std::ranges::range_value_t<CB>, vk::CommandBuffer>
        && std::convertible_to<std::ranges::range_value_t<TI>, RenderEngine::Image*>
        && std::same_as<std::ranges::range_value_t<RO>, RenderEngine::RenderObject>
    void record_render(
        CB command_buffers,
        RenderEngine::ShaderModule* shader_module,
        const std::string& pipeline_name,
        TI target_images,
        RO render_objects,
        RenderEngine::Image* depth_image,
        bool load_image
    )
    {
        auto pl = shader_module->get_graphics_pipeline(pipeline_name);

        vk::CommandBufferBeginInfo begin_info{};
        begin_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
        vk::CommandBufferInheritanceInfo inheritance_info{};
        begin_info.pInheritanceInfo = &inheritance_info;
        {
            std::lock_guard<std::recursive_mutex> pool_guard(vk_pool_lock);
            command_buffers[1].begin(begin_info);
            command_buffers[2].begin(begin_info);
        }

        vk::RenderingInfo rendering_info{};
        vk::Rect2D rect{};
        rect.offset = vk::Offset2D(0, 0);
        uint32_t width = UINT32_MAX;
        uint32_t height = UINT32_MAX;

        std::vector<vk::RenderingAttachmentInfo> attachments;
        std::vector<std::pair<RenderEngine::Image*, USAGE_TYPE>> sync_images{};

        for (RenderEngine::Image* image : target_images)
        {
            sync_images.emplace_back(image, G_RENDER);
            height = std::min(height, image->height());
            width = std::min(width, image->width());
            vk::RenderingAttachmentInfo attachment_info{};
            attachment_info.imageView = image->get_view();
            attachment_info.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
            attachment_info.loadOp = load_image ? vk::AttachmentLoadOp::eLoad : vk::AttachmentLoadOp::eClear;
            attachment_info.storeOp = vk::AttachmentStoreOp::eStore;
            attachment_info.clearValue.color = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
            attachments.push_back(attachment_info);
        }

        vk::RenderingAttachmentInfo depth_attachment_info{};

        if (depth_image)
        {
            sync_images.emplace_back(depth_image, G_DEPTH);
            rendering_info.pDepthAttachment = &depth_attachment_info;
            depth_attachment_info.loadOp = vk::AttachmentLoadOp::eClear;
            depth_attachment_info.storeOp = vk::AttachmentStoreOp::eStore;
            depth_attachment_info.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal;
            depth_attachment_info.imageView = depth_image->get_view();
            depth_attachment_info.clearValue.depthStencil = vk::ClearDepthStencilValue(1.0, 0);
        }

        process_images_sync(command_buffers[0], sync_images);

        rendering_info.pColorAttachments = attachments.data();
        rendering_info.colorAttachmentCount = attachments.size();
        rect.extent.width = width;
        rect.extent.height = height;
        rendering_info.renderArea = rect;
        rendering_info.layerCount = 1;

        {
            std::lock_guard<std::recursive_mutex> pool_guard(vk_pool_lock);
            command_buffers[2].beginRendering(rendering_info);
            command_buffers[2].bindPipeline(vk::PipelineBindPoint::eGraphics, pl);
        }

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

            {
                std::lock_guard<std::recursive_mutex> pool_guard(vk_pool_lock);
                command_buffers[2].setViewportWithCount(1, &viewport);
                command_buffers[2].setScissorWithCount(1, &scissor);
            }
        }

        for (RenderEngine::RenderObject& render_object : render_objects)
        {
            auto& buffer = render_object.vertex_buffer;
            auto buffer_usage = std::pair{buffer.get(), G_VERTEX};
            process_buffers_sync(command_buffers[1], std::views::single(buffer_usage));

            std::vector<vk::DescriptorSet> vk_sets = extractDescriptorSets(
                command_buffers[1],
                std::views::all(render_object.sets) | std::views::transform([](std::weak_ptr<RenderEngine::DescriptorPool::DescriptorSet> set){return set.lock().get();})
            );

            vk::DeviceSize offset = 0;

            {
                std::lock_guard<std::recursive_mutex> pool_guard(vk_pool_lock);
                auto buff = buffer->get_raw_buffer();
                command_buffers[2].bindVertexBuffers(0, 1, &buff, &offset);
                for (auto& set : render_object.sets)
                {
                    auto _set = set.lock();
                    auto _vk_set = _set->get_set();
                    command_buffers[2].bindDescriptorSets(
                        vk::PipelineBindPoint::eGraphics,
                        shader_module->get_pipeline_layout(),
                        _set->get_index(),
                        1,
                        &_vk_set,
                        0,
                        nullptr
                    );
                }
                command_buffers[2].draw(buffer->size() / shader_module->get_pipeline_vertex_size(pipeline_name), 1, 0, 0);
            }
        }

        {
            std::lock_guard<std::recursive_mutex> pool_guard(vk_pool_lock);
            command_buffers[1].end();
            command_buffers[2].endRendering();
            command_buffers[2].end();
            std::array<vk::CommandBuffer, 2> secondaries = {command_buffers[1], command_buffers[2]};
            command_buffers[0].executeCommands(2, secondaries.data());
        }
    }

    void record_buffer_to_image_transport(
        vk::CommandBuffer command_buffer,
        RenderEngine::RawBuffer* from_buffer,
        RenderEngine::Image* to_image
    );

    template <std::ranges::input_range I>
        requires std::convertible_to<std::ranges::range_value_t<I>, std::pair<RenderEngine::Image*, USAGE_TYPE>>
    static vk::Semaphore transfer_images_layout(
        I images
    )
    {
        ResourceFrame rf = {};
        vk::Device device = vkb_device.device;

        vk::CommandBufferAllocateInfo command_buffer_allocate_info = {};
        command_buffer_allocate_info.commandBufferCount = 1;
        command_buffer_allocate_info.commandPool = vk_render_command_pool;
        command_buffer_allocate_info.level = vk::CommandBufferLevel::ePrimary;

        vk::CommandBuffer cb;
        {
            std::lock_guard<std::recursive_mutex> pool_guard(vk_pool_lock);
            cb = device.allocateCommandBuffers(command_buffer_allocate_info)[0];
        }

        auto command_buffer_begin_info = vk::CommandBufferBeginInfo{};
        command_buffer_begin_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

        {
            std::lock_guard<std::recursive_mutex> pool_guard(vk_pool_lock);
            cb.begin(command_buffer_begin_info);
        }
        rf.process_images_sync(cb, images);
        {
            std::lock_guard<std::recursive_mutex> pool_guard(vk_pool_lock);
            cb.end();
        }

        auto semaphores = rf.get_semaphores();

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


        rf.update_image_layouts();


        std::thread t = std::thread(
            [=](bool wait)
            {
                auto _fence = fence;
                auto _cb = cb;
                if (wait)
                {
                    auto _ = device.waitForFences({_fence}, true, UINT64_MAX);
                }

                vkb_device_lock.lock();
                device.destroy(_fence);
                vk_pool_lock.lock();
                device.freeCommandBuffers(vk_render_command_pool, {_cb});
                vk_pool_lock.unlock();
                vkb_device_lock.unlock();
            },
            true
        );

        t.detach();

        return semaphores.first;
    }

    void record_image_transport(
        vk::CommandBuffer command_buffer,
        RenderEngine::Image* src_image,
        RenderEngine::Image* dst_image,
        std::pair<uint32_t, uint32_t> src_offset,
        std::pair<uint32_t, uint32_t> src_size,
        std::pair<uint32_t, uint32_t> dst_offset,
        std::pair<uint32_t, uint32_t> dst_size
    );

private:
    template <std::ranges::forward_range I>
        requires std::convertible_to<std::ranges::range_value_t<I>, std::pair<RenderEngine::RawBuffer*, USAGE_TYPE>>
    void process_buffers_sync(
        vk::CommandBuffer command_buffer,
        I buffers //buffer, new usage
    )
    {
        std::vector<vk::BufferMemoryBarrier2> barriers{};

        for (const std::pair<RenderEngine::RawBuffer*, USAGE_TYPE>& buff : buffers)
        {
            auto [buffer, usage_type] = buff;

            if (!this->used_buffers.contains(buffer->get_uid()))
            {
                this->used_buffers.insert({buffer->get_uid(), usage_type});
            }

            auto& used_buffer = this->used_buffers[buffer->get_uid()];

            if (usage_type == T_READ && used_buffer == usage_type)
            {
                continue;
            }

            vk::BufferMemoryBarrier2 barrier = {};

            barrier.buffer = buffer->get_raw_buffer();
            barrier.offset = 0;
            barrier.size = vk::WholeSize;
            barrier.srcStageMask = vk::PipelineStageFlagBits2::eAllCommands;
            barrier.dstStageMask = vk::PipelineStageFlagBits2::eAllCommands;

            barrier.srcAccessMask = getAccessBits(used_buffer);
            barrier.dstAccessMask = getAccessBits(usage_type);

            this->used_buffers[buffer->get_uid()] = usage_type;

            barriers.push_back(barrier);

            this->needed_semaphores.insert(buffer->get_semaphore_ref());
        }

        vk::DependencyInfo dp_info = {};
        dp_info.bufferMemoryBarrierCount = barriers.size();
        dp_info.pBufferMemoryBarriers = barriers.data();

        {
            std::lock_guard<std::recursive_mutex> pool_guard(vk_pool_lock);
            command_buffer.pipelineBarrier2(dp_info);
        }
    }


    template <std::ranges::input_range I>
        requires std::convertible_to<std::ranges::range_value_t<I>, std::pair<RenderEngine::Image*, USAGE_TYPE>>
    void process_images_sync(
        vk::CommandBuffer command_buffer,
        I images
    )
    {
        std::vector<vk::ImageMemoryBarrier2> barriers{};

        for (const std::pair<RenderEngine::Image*, USAGE_TYPE>& image : images)
        {
            auto [img, type] = image;

            if (!this->used_images.contains(image.first))
            {
                this->used_images.insert({image.first, {image.second, *img->get_layout_ptr()}});
            }

            auto& used_image = this->used_images[image.first];

            vk::ImageMemoryBarrier2 barrier = {};

            barrier.image = img->get_image();
            barrier.oldLayout = this->get_image_actual_image_layout(img);
            barrier.newLayout = getImageLayout(image.second);
            barrier.srcAccessMask = getImageAccessBits(barrier.oldLayout);
            barrier.dstAccessMask = getImageAccessBits(barrier.newLayout);
            barrier.srcStageMask = vk::PipelineStageFlagBits2::eAllCommands;
            barrier.dstStageMask = vk::PipelineStageFlagBits2::eAllCommands;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.aspectMask =
                img->get_native_format() == vk::Format::eD32Sfloat
                    ? vk::ImageAspectFlagBits::eDepth
                    : vk::ImageAspectFlagBits::eColor;

            this->used_images[image.first] = {image.second, barrier.newLayout};

            barriers.push_back(barrier);

            this->needed_semaphores.insert(img->get_semaphore_ref());
        }

        vk::DependencyInfo dp_info = {};
        dp_info.imageMemoryBarrierCount = barriers.size();
        dp_info.pImageMemoryBarriers = barriers.data();

        {
            std::lock_guard<std::recursive_mutex> pool_guard(vk_pool_lock);
            command_buffer.pipelineBarrier2(dp_info);
        }
    }
};
