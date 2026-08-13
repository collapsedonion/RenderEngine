//
// Created by Onion on 10.08.2026.
//

export module Resource;

#ifdef __linux__
export import vulkan;
export import std;
#endif

export namespace RenderEngine
{
    class Resource
    {
    public:
        virtual ~Resource() = default;
        virtual vk::Semaphore** get_semaphore_ref() = 0;
        virtual vk::WriteDescriptorSet get_descriptor_set_write(
            vk::DescriptorType type
        ) = 0;
        virtual std::uint64_t get_uid() = 0;
    };
}
