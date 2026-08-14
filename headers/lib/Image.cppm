//
// Created by Onion on 10.08.2026.
//
module;
#include <vk_mem_alloc.h>
#include "re_typedefs.h"

#ifdef __APPLE__
#include <vulkan/vulkan.hpp>
#include <memory>
#endif

export module Image;

#ifdef __linux__
export import vulkan;
export import std;
#endif

export import Resource;

export namespace RenderEngine
{
    class Image: public Resource
    {
        std::uint64_t _uid = 0;

        std::uint32_t _width = 0;
        std::uint32_t _height = 0;

        bool _swap_chain = false;
        std::uint32_t _sc_index = 0;

        vk::Format _format = vk::Format::eR8G8B8A8Srgb;
        vk::Image _img = {};
        vk::ImageView _view = {};
        vk::Sampler _sampler = {};

        VmaAllocation _allocation = {};
        vk::ImageLayout _layout = vk::ImageLayout::eUndefined;
        vk::Semaphore* _semaphore = nullptr;

        vk::DescriptorImageInfo _combined_img_sampler = {};
        vk::DescriptorImageInfo _storage_img = {};

    private:
        Image(
            std::uint32_t width,
            std::uint32_t height,
            RE_IMAGE_FORMATS format,
            bool linear_filtering,
            bool repeat_u,
            bool repeat_v
        );


        Image();
    public:
        ~Image() final;

        static std::shared_ptr<Image> get_swapchain_image();

        static std::shared_ptr<Image> create(
            std::uint32_t width,
            std::uint32_t height,
            RE_IMAGE_FORMATS format,
            bool linear_filtering,
            bool repeat_u,
            bool repeat_v
        )
        {
            return std::shared_ptr<Image>(
                new Image(
                    width,
                    height,
                    format,
                    linear_filtering,
                    repeat_u,
                    repeat_v
                )
            );
        }

        void present();

        [[nodiscard]]
        std::uint32_t width() const
        {
            return _width;
        }

        [[nodiscard]]
        std::uint32_t height() const
        {
            return _height;
        }

        [[nodiscard]]
        std::pair<std::uint32_t, std::uint32_t> get_extent() const
        {
            return {_width, _height};
        }

        [[nodiscard]]
        vk::ImageLayout* get_layout_ptr()
        {
            return &_layout;
        }

        [[nodiscard]]
        vk::Semaphore** get_semaphore_ref() override
        {
            return &_semaphore;
        }

        [[nodiscard]]
        std::uint64_t get_uid() override
        {
            return _uid;
        }

        [[nodiscard]]
        vk::Image get_image() const
        {
            return _img;
        }

        [[nodiscard]]
        vk::ImageView get_view() const
        {
            return _view;
        }

        [[nodiscard]]
        vk::Format get_native_format() const
        {
            return _format;
        }

        vk::WriteDescriptorSet get_descriptor_set_write(vk::DescriptorType type) override;
    };
}