//
// Created by Роман  Тимофеев on 11.05.2026.
//
module;

#include <vulkan/vulkan.hpp>

#include <ranges>
#include <set>
#include <thread>
#include <unordered_map>
#include <mutex>
#include "re_typedefs.h"

#include "uid.h"

export module command_encoders;
import descriptor_pool;
import storage_buffer;
import render_engine_shares;
import synchronization;
import shader_module;
import image;

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
}

export class ResourceFrame
{
    std::unordered_map<UID, USAGE_TYPE> used_buffers = {};
    //Usage type, changed layout
    std::unordered_map<RE_Image*, std::pair<USAGE_TYPE, vk::ImageLayout>> used_images = {};

    std::set<vk::Semaphore**> needed_semaphores = {};

public:
    inline vk::ImageLayout get_image_actual_image_layout(
        RE_Image* pImage
    )
    {
        if (!this->used_images.contains(pImage))
        {
            return pImage->last_layout;
        }

        return this->used_images.at(pImage).second;
    }

    void record_buffers_transport(
        vk::CommandBuffer command_buffer,
        const RE_BufferToBufferTransfer& transfer
    )
    {
        auto from_buffer = static_cast<RE_Buffer*>(transfer.from_buffer);
        auto to_buffer = static_cast<RE_Buffer*>(transfer.to_buffer);

        size_t from_effective = transfer.from_index + transfer.size;
        size_t to_effective = transfer.to_index + transfer.size;

        if (from_effective > from_buffer->size || to_effective > to_buffer->size)
        {
            throw std::out_of_range("Buffer copy is out of range");
        }

        std::array<std::pair<RE_Buffer*, USAGE_TYPE>, 2> buffers = {
            std::pair{from_buffer, T_READ},
            std::pair{to_buffer, T_WRITE}
        };
        process_buffers_sync(command_buffer, buffers);

        vk::CopyBufferInfo2 copy_buffer_info = {};
        copy_buffer_info.srcBuffer = from_buffer->buffer;
        copy_buffer_info.dstBuffer = to_buffer->buffer;

        copy_buffer_info.regionCount = 1;
        vk::BufferCopy2 copy_region = {};
        copy_buffer_info.pRegions = &copy_region;

        copy_region.srcOffset = transfer.from_index;
        copy_region.dstOffset = transfer.to_index;
        copy_region.size = transfer.size ? transfer.size: std::min(to_buffer->size, from_buffer->size);

        {
            std::lock_guard<std::recursive_mutex> pool_guard(vk_pool_lock);
            command_buffer.copyBuffer2(copy_buffer_info);
        }
    }

    void record_compute_shader_submit(
        vk::CommandBuffer command_buffer,
        RE_ShaderModule* shader_module,
        const std::string& shader_name,
        size_t descriptor_set_count,
        vk::DescriptorSet* descriptor_sets,
        size_t groupCountX,
        size_t groupCountY,
        size_t groupCountZ
    )
    {
        auto& ppl = shader_module->registered_compute_pipelines[shader_name];

        {
            std::lock_guard<std::recursive_mutex> pool_guard(vk_pool_lock);
            command_buffer.bindPipeline(vk::PipelineBindPoint::eCompute, ppl.pipeline);
            command_buffer.bindDescriptorSets(
                vk::PipelineBindPoint::eCompute,
                shader_module->pipeline_layout,
                0,
                descriptor_set_count,
                descriptor_sets,
                0,
                nullptr
            );
            command_buffer.dispatch(groupCountX, groupCountY, groupCountZ);
        }
    }


    std::pair<
        vk::Semaphore,
        std::pair<std::vector<vk::Semaphore>, std::vector<vk::PipelineStageFlags>>
    > get_semaphores()
    {
        std::vector<vk::Semaphore> semaphores{};
        std::vector<vk::PipelineStageFlags> pipeline_stages{};
        std::vector<vk::Semaphore**> resource_semaphores{};

        for (auto semaphore : this->needed_semaphores)
        {
            if (*semaphore != nullptr && !is_waited(*semaphore))
            {
                free_semaphore(*semaphore);
                semaphores.push_back(**semaphore);
                pipeline_stages.push_back(vk::PipelineStageFlagBits::eAllCommands);
            }

            resource_semaphores.push_back(semaphore);
        }

        std::pair<vk::Semaphore, bool> new_semaphore = get_next_semaphore(
            resource_semaphores.size(),
            resource_semaphores.data()
        );

        if (new_semaphore.second)
        {
            semaphores.push_back(new_semaphore.first);
            pipeline_stages.push_back(vk::PipelineStageFlagBits::eAllCommands);
        }

        return {new_semaphore.first, {semaphores, pipeline_stages}};
    }

    template <std::ranges::sized_range I>
        requires std::convertible_to<std::ranges::range_value_t<I>, RE_DescriptorSet*>
    std::vector<vk::DescriptorSet> extractDescriptorSets(
        vk::CommandBuffer command_buffer,
        I descriptor_sets
    )
    {
        std::vector<vk::DescriptorSet> _descriptor_sets;
        _descriptor_sets.reserve(std::ranges::size(descriptor_sets));
        std::vector<std::pair<RE_Image*, USAGE_TYPE>> images_to_sync{};

        for (RE_DescriptorSet* descriptor_set : descriptor_sets)
        {
            for (auto& resource : descriptor_set->bind_resources)
            {
                switch (resource.second.second)
                {
                case vk::DescriptorType::eUniformBuffer:
                    {
                        auto* _buffer = static_cast<RE_Buffer*>(resource.second.first);
                        this->used_buffers[_buffer->uid] = USAGE_TYPE::C_UNIFORM;
                        this->needed_semaphores.insert(&_buffer->semaphore);
                        break;
                    }
                case vk::DescriptorType::eStorageBuffer:
                    {
                        auto* _buffer = static_cast<RE_Buffer*>(resource.second.first);
                        this->used_buffers[_buffer->uid] = USAGE_TYPE::C_STORAGE;
                        this->needed_semaphores.insert(&_buffer->semaphore);
                        break;
                    }
                case vk::DescriptorType::eStorageImage:
                    {
                        auto* _image = static_cast<RE_Image*>(resource.second.first);
                        images_to_sync.push_back({_image, USAGE_TYPE::C_STORAGE});
                        this->needed_semaphores.insert(&_image->semaphore);
                        break;
                    }

                case vk::DescriptorType::eCombinedImageSampler:
                    {
                        auto* _image = static_cast<RE_Image*>(resource.second.first);
                        images_to_sync.push_back({_image, USAGE_TYPE::I_SAMPLED});
                        this->needed_semaphores.insert(&_image->semaphore);
                        break;
                    }

                default:

                    break;
                }
            }

            _descriptor_sets.push_back(descriptor_set->descriptor_set);
        }

        process_images_sync(
            command_buffer,
            images_to_sync
        );

        return _descriptor_sets;
    }

    void update_image_layouts()
    {
        for (auto& [pImage, data] : this->used_images)
        {
            pImage->last_layout = data.second;
        }
    }

    // command_buffers: [0]=primary, [1]=secondary_sync, [2]=secondary_draw
    template <
        std::ranges::random_access_range CB,
        std::ranges::sized_range TI,
        std::ranges::forward_range RO
    >
        requires std::same_as<std::ranges::range_value_t<CB>, vk::CommandBuffer>
        && std::convertible_to<std::ranges::range_value_t<TI>, RE_Image*>
        && std::same_as<std::ranges::range_value_t<RO>, RE_RenderObject>
    void record_render(
        CB command_buffers,
        RE_ShaderModule* shader_module,
        const std::string& pipeline_name,
        TI target_images,
        RO render_objects,
        RE_Image* depth_image,
        bool load_image
    )
    {
        auto& ppl = shader_module->registered_graphics_pipelines[pipeline_name];

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
        std::vector<std::pair<RE_Image*, USAGE_TYPE>> sync_images{};
        sync_images.reserve(std::ranges::size(target_images));

        for (RE_Image* image : target_images)
        {
            sync_images.push_back({image, G_RENDER});
            height = std::min(height, image->height);
            width = std::min(width, image->width);
            vk::RenderingAttachmentInfo attachment_info{};
            attachment_info.imageView = image->view;
            attachment_info.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
            attachment_info.loadOp = load_image ? vk::AttachmentLoadOp::eLoad : vk::AttachmentLoadOp::eClear;
            attachment_info.storeOp = vk::AttachmentStoreOp::eStore;
            attachment_info.clearValue.color = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
            attachments.push_back(attachment_info);
        }

        vk::RenderingAttachmentInfo depth_attachment_info{};

        if (ppl.depth_enable)
        {
            sync_images.push_back({depth_image, G_DEPTH});
            rendering_info.pDepthAttachment = &depth_attachment_info;
            depth_attachment_info.loadOp = vk::AttachmentLoadOp::eClear;
            depth_attachment_info.storeOp = vk::AttachmentStoreOp::eStore;
            depth_attachment_info.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal;
            depth_attachment_info.imageView = depth_image->view;
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
            command_buffers[2].bindPipeline(vk::PipelineBindPoint::eGraphics, ppl.base_ppl.pipeline);
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

        for (auto& render_object : render_objects)
        {
            RE_Buffer* buffer = static_cast<RE_Buffer*>(render_object.vertex_buffer);
            std::pair<RE_Buffer*, USAGE_TYPE> buffer_usage = {buffer, G_VERTEX};
            process_buffers_sync(command_buffers[1], std::views::single(buffer_usage));

            std::vector<vk::DescriptorSet> vk_sets = extractDescriptorSets(
                command_buffers[1],
                std::span(reinterpret_cast<RE_DescriptorSet**>(render_object.descriptor_sets),
                          render_object.descriptor_set_count)
            );

            vk::DeviceSize offset = 0;

            {
                std::lock_guard<std::recursive_mutex> pool_guard(vk_pool_lock);
                command_buffers[2].bindVertexBuffers(0, 1, &buffer->buffer, &offset);
                command_buffers[2].bindDescriptorSets(
                    vk::PipelineBindPoint::eGraphics,
                    shader_module->pipeline_layout,
                    0,
                    vk_sets.size(),
                    vk_sets.data(),
                    0,
                    nullptr
                );
                command_buffers[2].draw(buffer->size / ppl.bytes_per_vertex, 1, 0, 0);
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
        RE_Buffer* from_buffer,
        RE_Image* to_image
    )
    {
        std::array<std::pair<RE_Buffer*, USAGE_TYPE>, 1> buffers = {std::pair{from_buffer, T_READ}};
        process_buffers_sync(command_buffer, buffers);

        std::array<std::pair<RE_Image*, USAGE_TYPE>, 1> images = {std::pair{to_image, T_WRITE}};
        process_images_sync(command_buffer, images);

        vk::BufferImageCopy buffer_image_copy = {};
        buffer_image_copy.bufferOffset = 0;
        buffer_image_copy.bufferImageHeight = 0;
        buffer_image_copy.bufferRowLength = 0;
        buffer_image_copy.imageSubresource.mipLevel = 0;
        buffer_image_copy.imageSubresource.layerCount = 1;
        buffer_image_copy.imageSubresource.baseArrayLayer = 0;
        buffer_image_copy.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        buffer_image_copy.imageOffset = vk::Offset3D{0, 0, 0};
        buffer_image_copy.imageExtent = vk::Extent3D{to_image->width, to_image->height, 1};

        {
            std::lock_guard<std::recursive_mutex> pool_guard(vk_pool_lock);
            command_buffer.copyBufferToImage(
                from_buffer->buffer,
                to_image->image,
                this->get_image_actual_image_layout(to_image),
                1,
                &buffer_image_copy
            );
        }
    }


    template <std::ranges::input_range I>
        requires std::convertible_to<std::ranges::range_value_t<I>, std::pair<RE_Image*, USAGE_TYPE>>
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
                    device.waitForFences({_fence}, true, UINT64_MAX);
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
        RE_Image* src_image,
        RE_Image* dst_image,
        std::pair<uint32_t, uint32_t> src_offset,
        std::pair<uint32_t, uint32_t> src_size,
        std::pair<uint32_t, uint32_t> dst_offset,
        std::pair<uint32_t, uint32_t> dst_size
    )
    {
        std::array<std::pair<RE_Image*, USAGE_TYPE>, 2> used_images = {
            std::pair{src_image, T_READ},
            std::pair{dst_image, T_WRITE}
        };

        process_images_sync(
            command_buffer,
            used_images
        );

        vk::ImageAspectFlags aspect = (src_image->format == vk::Format::eD32Sfloat)
            ? vk::ImageAspectFlagBits::eDepth
            : vk::ImageAspectFlagBits::eColor;

        vk::ImageBlit blit_info {};
        blit_info.srcSubresource = vk::ImageSubresourceLayers{aspect, 0, 0, 1};
        blit_info.srcOffsets[0]  = vk::Offset3D{(int32_t)src_offset.first, (int32_t)src_offset.second, 0};
        blit_info.srcOffsets[1]  = vk::Offset3D{(int32_t)(src_offset.first + src_size.first), (int32_t)(src_offset.second + src_size.second), 1};
        blit_info.dstSubresource = vk::ImageSubresourceLayers{aspect, 0, 0, 1};
        blit_info.dstOffsets[0]  = vk::Offset3D{(int32_t)dst_offset.first, (int32_t)dst_offset.second, 0};
        blit_info.dstOffsets[1]  = vk::Offset3D{(int32_t)(dst_offset.first + dst_size.first), (int32_t)(dst_offset.second + dst_size.second), 1};

        {
            std::lock_guard<std::recursive_mutex> pool_guard(vk_pool_lock);
            command_buffer.blitImage(
                src_image->image, vk::ImageLayout::eTransferSrcOptimal,
                dst_image->image, vk::ImageLayout::eTransferDstOptimal,
                {blit_info}, vk::Filter::eLinear
            );
        }
    }

private:
    template <std::ranges::forward_range I>
        requires std::convertible_to<std::ranges::range_value_t<I>, std::pair<RE_Buffer*, USAGE_TYPE>>
    void process_buffers_sync(
        vk::CommandBuffer command_buffer,
        I buffers //buffer, new usage
    )
    {
        std::vector<vk::BufferMemoryBarrier2> barriers{};

        for (const std::pair<RE_Buffer*, USAGE_TYPE>& buffer : buffers)
        {
            if (!this->used_buffers.contains(buffer.first->uid))
            {
                this->used_buffers.insert({buffer.first->uid, buffer.second});
            }

            auto& used_buffer = this->used_buffers[buffer.first->uid];

            if (buffer.second == T_READ && used_buffer == buffer.second)
            {
                continue;
            }

            vk::BufferMemoryBarrier2 barrier = {};

            barrier.buffer = buffer.first->buffer;
            barrier.offset = 0;
            barrier.size = VK_WHOLE_SIZE;
            barrier.srcStageMask = vk::PipelineStageFlagBits2::eAllCommands;
            barrier.dstStageMask = vk::PipelineStageFlagBits2::eAllCommands;

            barrier.srcAccessMask = getAccessBits(used_buffer);
            barrier.dstAccessMask = getAccessBits(buffer.second);

            this->used_buffers[buffer.first->uid] = buffer.second;

            barriers.push_back(barrier);

            this->needed_semaphores.insert(&buffer.first->semaphore);
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
        requires std::convertible_to<std::ranges::range_value_t<I>, std::pair<RE_Image*, USAGE_TYPE>>
    void process_images_sync(
        vk::CommandBuffer command_buffer,
        I images
    )
    {
        std::vector<vk::ImageMemoryBarrier2> barriers{};

        for (const std::pair<RE_Image*, USAGE_TYPE>& image : images)
        {
            if (!this->used_images.contains(image.first))
            {
                this->used_images.insert({image.first, {image.second, image.first->last_layout}});
            }

            auto& used_image = this->used_images[image.first];

            vk::ImageMemoryBarrier2 barrier = {};

            barrier.image = image.first->image;
            barrier.oldLayout = this->get_image_actual_image_layout(image.first);
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
                image.first->format == vk::Format::eD32Sfloat
                    ? vk::ImageAspectFlagBits::eDepth
                    : vk::ImageAspectFlagBits::eColor;

            this->used_images[image.first] = {image.second, barrier.newLayout};

            barriers.push_back(barrier);

            this->needed_semaphores.insert(&image.first->semaphore);
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
