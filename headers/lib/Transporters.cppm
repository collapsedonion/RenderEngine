//
// Created by Onion on 12.08.2026.
//

module;

#ifdef __APPLE__
#include <memory>
#include <functional>
#endif

export module Transporters;
#ifdef __linux__
export import std;
#endif
import Buffer;
import Image;
import dynamic_dispatchable_iterator;

export namespace RenderEngine
{
    struct BufferToBufferInfo
    {
        std::weak_ptr<RawBuffer> from;
        std::weak_ptr<RawBuffer> to;
        std::uint32_t from_index = 0;
        std::uint32_t to_index = 0;
        std::uint32_t size = 0;
    };

    struct ImageToImageInfo
    {
        std::weak_ptr<Image> from_image;
        std::weak_ptr<Image> to_image;
        std::uint32_t from_offset_x = 0;
        std::uint32_t from_offset_y = 0;
        std::uint32_t from_width = 0;
        std::uint32_t from_height = 0;
        std::uint32_t to_offset_x = 0;
        std::uint32_t to_offset_y = 0;
        std::uint32_t to_width = 0;
        std::uint32_t to_height = 0;
    };

    void transfer_buffer_to_image(
        std::shared_ptr<RawBuffer>& src,
        std::shared_ptr<Image>& dst,
        std::function<void()> transfer_callback
    );

    void transfer_image_to_image(
        Iterator<ImageToImageInfo>& image_transfers
    );

    void transfer_buffer_to_buffer(
        Iterator<BufferToBufferInfo>& buffer_transfers,
        std::function<void()> transfer_callback
    );
}
