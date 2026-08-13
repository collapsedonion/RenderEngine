//
// Created by Onion on 12.08.2026.
//

export module RenderObject;
export import Buffer;
export import DescriptorPool;

export namespace RenderEngine
{
    struct RenderObject
    {
        std::shared_ptr<RawBuffer> vertex_buffer;
        std::span<std::weak_ptr<DescriptorPool::DescriptorSet>> sets;
    };
}
