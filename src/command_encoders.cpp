module;

#include "re_typedefs.h"

#include "uid.h"

#if defined(__APPLE__)
#include <algorithm>
#include <array>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <vulkan/vulkan.hpp>
#endif

module command_encoders;

#if defined(__linux__)
import vulkan;
import std;
#endif
import render_engine_shares;
import synchronization;

vk::ImageLayout ResourceFrame::get_image_actual_image_layout(
    RenderEngine::Image* pImage
)
{
    if (!this->used_images.contains(pImage))
    {
        return *pImage->get_layout_ptr();
    }

    return this->used_images.at(pImage).second;
}

void ResourceFrame::record_buffers_transport(
    vk::CommandBuffer command_buffer,
    const RenderEngine::BufferToBufferInfo& transfer
)
{
    size_t from_effective = transfer.from_index + transfer.size;
    size_t to_effective = transfer.to_index + transfer.size;

    auto from = transfer.from.lock();
    auto to = transfer.to.lock();

    if (from_effective > from->size() || to_effective > to->size())
    {
        throw std::out_of_range("Buffer copy is out of range");
    }

    std::array<std::pair<RenderEngine::RawBuffer*, USAGE_TYPE>, 2> buffers = {
        std::pair{from.get(), T_READ},
        std::pair{to.get(), T_WRITE}
    };
    process_buffers_sync(command_buffer, buffers);

    vk::CopyBufferInfo2 copy_buffer_info = {};
    copy_buffer_info.srcBuffer = from->get_raw_buffer();
    copy_buffer_info.dstBuffer = to->get_raw_buffer();

    copy_buffer_info.regionCount = 1;
    vk::BufferCopy2 copy_region = {};
    copy_buffer_info.pRegions = &copy_region;

    copy_region.srcOffset = transfer.from_index;
    copy_region.dstOffset = transfer.to_index;
    copy_region.size = transfer.size ? transfer.size:
        std::min(to->size(), from->size()
    );

    {
        std::lock_guard<std::recursive_mutex> pool_guard(vk_pool_lock);
        command_buffer.copyBuffer2(copy_buffer_info);
    }
}

void ResourceFrame::record_compute_shader_submit(
    vk::CommandBuffer command_buffer,
    RenderEngine::ShaderModule* shader_module,
    const std::string& shader_name,
    size_t descriptor_set_count,
    vk::DescriptorSet* descriptor_sets,
    size_t groupCountX,
    size_t groupCountY,
    size_t groupCountZ
)
{

    auto ppl = shader_module->get_compute_pipeline(shader_name);

    {
        std::lock_guard<std::recursive_mutex> pool_guard(vk_pool_lock);
        command_buffer.bindPipeline(vk::PipelineBindPoint::eCompute, ppl);
        command_buffer.bindDescriptorSets(
            vk::PipelineBindPoint::eCompute,
            shader_module->get_pipeline_layout(),
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
> ResourceFrame::get_semaphores()
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

void ResourceFrame::update_image_layouts()
{
    for (auto& [pImage, data] : this->used_images)
    {
        *pImage->get_layout_ptr() = data.second;
    }
}

void ResourceFrame::record_buffer_to_image_transport(
    vk::CommandBuffer command_buffer,
    RenderEngine::RawBuffer* from_buffer,
    RenderEngine::Image* to_image
)
{
    std::array<std::pair<RenderEngine::RawBuffer*, USAGE_TYPE>, 1> buffers = {std::pair{from_buffer, T_READ}};
    process_buffers_sync(command_buffer, buffers);

    std::array<std::pair<RenderEngine::Image*, USAGE_TYPE>, 1> images = {std::pair{to_image, T_WRITE}};
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
    buffer_image_copy.imageExtent = vk::Extent3D{to_image->width(), to_image->height(), 1};

    {
        std::lock_guard<std::recursive_mutex> pool_guard(vk_pool_lock);
        command_buffer.copyBufferToImage(
            from_buffer->get_raw_buffer(),
            to_image->get_image(),
            this->get_image_actual_image_layout(to_image),
            1,
            &buffer_image_copy
        );
    }
}

void ResourceFrame::record_image_transport(
    vk::CommandBuffer command_buffer,
    RenderEngine::Image* src_image,
    RenderEngine::Image* dst_image,
    std::pair<uint32_t, uint32_t> src_offset,
    std::pair<uint32_t, uint32_t> src_size,
    std::pair<uint32_t, uint32_t> dst_offset,
    std::pair<uint32_t, uint32_t> dst_size
)
{
    std::array<std::pair<RenderEngine::Image*, USAGE_TYPE>, 2> used_images = {
        std::pair{src_image, T_READ},
        std::pair{dst_image, T_WRITE}
    };

    process_images_sync(
        command_buffer,
        used_images
    );

    vk::ImageAspectFlags aspect = (src_image->get_native_format() == vk::Format::eD32Sfloat)
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
            src_image->get_image(), vk::ImageLayout::eTransferSrcOptimal,
            dst_image->get_image(), vk::ImageLayout::eTransferDstOptimal,
            {blit_info}, vk::Filter::eLinear
        );
    }
}
