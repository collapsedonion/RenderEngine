//
// Created by Onion on 10.08.2026.
//
module;

#include "re_typedefs.h"

#ifdef __APPLE__
#include <memory>
#include <vulkan/vulkan.hpp>
#include <unordered_map>
#include <ranges>
#endif

export module DescriptorPool;

#ifdef __linux__
export import std;
export import vulkan;
#endif

export import dynamic_dispatchable_iterator;
export import ShaderModule;
export import Resource;

export namespace RenderEngine
{

    class DescriptorPool
    {
    public:
        class DescriptorSet
        {
            std::weak_ptr<DescriptorPool> _my_pool;
            std::size_t _index = 0;
            vk::DescriptorSet _set = {};
            std::unordered_map<
                std::uint32_t,
                std::pair<Resource*, vk::DescriptorType>
            > _bound_resources;

        public:
            ~DescriptorSet();

            void write_bindings(
                Iterator<std::tuple<std::string&, Resource*>>& resources
            );

            [[nodiscard]]
            vk::DescriptorSet get_set() const
            {
                return _set;
            }

            [[nodiscard]]
            std::size_t get_index() const
            {
                return _index;
            }

            auto get_bound_resources()
            {
                return std::views::all(_bound_resources);
            }

            friend DescriptorPool;
        };

    private:
        std::shared_ptr<ShaderModule> _shader_module;
        vk::DescriptorPool _pool = {};
        std::uint32_t _max_set = 0;
        std::uint32_t _allocated_layouts = 0;
        std::weak_ptr<DescriptorPool> _self;

    private:
        DescriptorPool(
            std::shared_ptr<ShaderModule> shader_module,
            std::uint32_t descriptor_count
        );

    public:
        ~DescriptorPool();

        static std::shared_ptr<DescriptorPool> create(
            std::shared_ptr<ShaderModule> shader_module,
            std::uint32_t descriptor_count
        )
        {
            auto ptr = std::shared_ptr<DescriptorPool>(new DescriptorPool(std::move(shader_module), descriptor_count));
            ptr->_self = ptr;
            return ptr;
        }

        std::vector<std::shared_ptr<DescriptorSet>> create_sets(
            Iterator<std::uint32_t>& set_indices
        ) const;
    };
}
