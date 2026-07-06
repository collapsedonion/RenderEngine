//
// Created by Роман  Тимофеев on 11.05.2026.
//

#include "command_encoders.h"

#include "render_engine_shares.h"
#include "synchronization.h"

inline vk::AccessFlags2 getAccessBits(
    USAGE_TYPE usage
) {
    switch (usage) {
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
}

inline vk::AccessFlags2 getImageAccessBits(
    vk::ImageLayout layout
) {
    switch (layout) {
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
        default:
            break;
    }
}

inline vk::ImageLayout getImageLayout(
    USAGE_TYPE usage_type
) {
    switch (usage_type) {
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

void record_compute_shader_submit(
    vk::CommandBuffer command_buffer,
    RE_ShaderModule *shader_module,
    const std::string &shader_name,
    size_t descriptor_set_count,
    vk::DescriptorSet *descriptor_sets,
    size_t groupCountX,
    size_t groupCountY,
    size_t groupCountZ
) {
    auto &ppl = shader_module->registered_compute_pipelines[shader_name];

    vk_pool_lock.lock();
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
    vk_pool_lock.unlock();
}

void record_buffers_transport(
    vk::CommandBuffer command_buffer,
    RE_Buffer *from_buffer,
    RE_Buffer *to_buffer,
    size_t size_to_copy,
    size_t from_index,
    size_t to_index
) {
    size_t from_effective = from_index + size_to_copy;
    size_t to_effective = to_index + size_to_copy;

    if (from_effective > from_buffer->size || to_effective > to_buffer->size) {
        throw std::out_of_range("Buffer copy is out of range");
    }

    vk::CopyBufferInfo2 copy_buffer_info = {};
    copy_buffer_info.srcBuffer = from_buffer->buffer;
    copy_buffer_info.dstBuffer = to_buffer->buffer;

    copy_buffer_info.regionCount = 1;
    vk::BufferCopy2 copy_region = {};
    copy_buffer_info.pRegions = &copy_region;

    copy_region.srcOffset = from_index;
    copy_region.dstOffset = to_index;
    copy_region.size = size_to_copy;

    vk_pool_lock.lock();
    command_buffer.copyBuffer2(copy_buffer_info);
    vk_pool_lock.unlock();
}

void record_buffer_to_image_transport(
    vk::CommandBuffer command_buffer,
    RE_Buffer *from_buffer,
    RE_Image *to_image
) {

    vk::BufferImageCopy buffer_image_copy = {};
    buffer_image_copy.bufferOffset = 0;
    buffer_image_copy.bufferImageHeight = 0;
    buffer_image_copy.bufferRowLength = 0;
    buffer_image_copy.imageSubresource.mipLevel = 0;
    buffer_image_copy.imageSubresource.layerCount = 1;
    buffer_image_copy.imageSubresource.baseArrayLayer = 0;
    buffer_image_copy.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    buffer_image_copy.imageOffset = vk::Offset3D{0,0,0};
    buffer_image_copy.imageExtent = vk::Extent3D{to_image->width,to_image->height,1};

    vk_pool_lock.lock();
    command_buffer.copyBufferToImage(
        from_buffer->buffer,
        to_image->image,
        to_image->last_layout,
        1,
        &buffer_image_copy
    );
    vk_pool_lock.unlock();
}

void process_buffers_sync(
    vk::CommandBuffer command_buffer,
    ResourceFrame *resource_frame,
    std::pair<RE_Buffer *, USAGE_TYPE> *buffers, //buffer, new usage
    size_t buffer_count
) {
    std::vector<vk::BufferMemoryBarrier2> barriers{};

    for (size_t i = 0; i < buffer_count; i++) {
        auto &buffer = buffers[i];

        if (!resource_frame->used_buffers.contains(buffer.first->uid)) {
            resource_frame->used_buffers.insert({buffer.first->uid, buffer.second});
        }

        auto &used_buffer = resource_frame->used_buffers[buffer.first->uid];

        if (buffer.second == T_READ && used_buffer == buffer.second) {
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

        resource_frame->used_buffers[buffer.first->uid] = buffer.second;

        barriers.push_back(barrier);
    }

    vk::DependencyInfo dp_info = {};
    dp_info.bufferMemoryBarrierCount = barriers.size();
    dp_info.pBufferMemoryBarriers = barriers.data();

    vk_pool_lock.lock();
    command_buffer.pipelineBarrier2(dp_info);
    vk_pool_lock.unlock();
}

void process_images_sync(
    vk::CommandBuffer command_buffer,
    ResourceFrame *resource_frame,
    std::pair<RE_Image *, USAGE_TYPE> *images, //image, new usage
    size_t buffer_count
) {
    std::vector<vk::ImageMemoryBarrier2> barriers{};

    for (size_t i = 0; i < buffer_count; i++) {
        auto &image = images[i];

        if (!resource_frame->used_images.contains(image.first->uid)) {
            resource_frame->used_images.insert({image.first->uid, image.second});
        }

        auto &used_image = resource_frame->used_images[image.first->uid];

        vk::ImageMemoryBarrier2 barrier = {};

        barrier.image = image.first->image;
        barrier.oldLayout = image.first->last_layout;
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

        image.first->last_layout = barrier.newLayout;

        resource_frame->used_images[image.first->uid] = image.second;

        barriers.push_back(barrier);
    }

    vk::DependencyInfo dp_info = {};
    dp_info.imageMemoryBarrierCount = barriers.size();
    dp_info.pImageMemoryBarriers = barriers.data();

    vk_pool_lock.lock();
    command_buffer.pipelineBarrier2(dp_info);
    vk_pool_lock.unlock();
}

std::vector<vk::DescriptorSet> extractDescriptorSets(
    vk::CommandBuffer command_buffer,
    ResourceFrame *resource_frame,
    size_t set_count,
    RE_DescriptorSet **descriptor_sets
) {
    std::vector<vk::DescriptorSet> _descriptor_sets;
    _descriptor_sets.reserve(set_count);
    std::vector<std::pair<RE_Image *, USAGE_TYPE> > images_to_sync{};

    for (uint32_t i = 0; i < set_count; i++) {
        for (auto &resource: descriptor_sets[i]->bind_resources) {
            switch (resource.second.second) {
                case vk::DescriptorType::eUniformBuffer: {
                    auto *_buffer = static_cast<RE_Buffer *>(resource.second.first);
                    resource_frame->used_buffers[_buffer->uid] = USAGE_TYPE::C_UNIFORM;
                    resource_frame->needed_semaphores.insert(&_buffer->semaphore);
                    break;
                }
                case vk::DescriptorType::eStorageBuffer: {
                    auto *_buffer = static_cast<RE_Buffer *>(resource.second.first);
                    resource_frame->used_buffers[_buffer->uid] = USAGE_TYPE::C_STORAGE;
                    resource_frame->needed_semaphores.insert(&_buffer->semaphore);
                    break;
                }
                case vk::DescriptorType::eStorageImage: {
                    auto *_image = static_cast<RE_Image *>(resource.second.first);
                    images_to_sync.push_back({_image, USAGE_TYPE::C_STORAGE});
                    resource_frame->needed_semaphores.insert(&_image->semaphore);
                    break;
                }

                case vk::DescriptorType::eCombinedImageSampler: {
                    auto *_image = static_cast<RE_Image *>(resource.second.first);
                    images_to_sync.push_back({_image, USAGE_TYPE::I_SAMPLED});
                    resource_frame->needed_semaphores.insert(&_image->semaphore);
                    break;
                }

                default:


            }
        }

        _descriptor_sets.push_back(descriptor_sets[i]->descriptor_set);
    }

    process_images_sync(
        command_buffer,
        resource_frame,
        images_to_sync.data(),
        images_to_sync.size()
    );

    return _descriptor_sets;
}

std::pair<
    vk::Semaphore,
    std::pair<std::vector<vk::Semaphore>, std::vector<vk::PipelineStageFlags> >
> get_semaphores(ResourceFrame *resource_frame) {
    std::vector<vk::Semaphore> semaphores{};
    std::vector<vk::PipelineStageFlags> pipeline_stages{};
    std::vector<vk::Semaphore **> resource_semaphores{};

    for (auto semaphore: resource_frame->needed_semaphores) {
        if (*semaphore != nullptr && !is_waited(*semaphore)) {
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

    if (new_semaphore.second) {
        semaphores.push_back(new_semaphore.first);
        pipeline_stages.push_back(vk::PipelineStageFlagBits::eAllCommands);
    }

    return {new_semaphore.first, {semaphores, pipeline_stages}};
}
