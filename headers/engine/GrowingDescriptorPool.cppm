//
// Created by Роман  Тимофеев on 24.08.2026.
//
module;

#ifdef __APPLE__
#include <cinttypes>
#include <memory>
#include <unordered_map>
#include <map>
#include <vector>
#include <optional>
#include <shared_mutex>
#endif

export module GrowingDescriptorPool;
import DescriptorPool;

namespace RenderEngine {

    export class GrowingDescriptorPool {
    public:
        class ManagedDescriptorSet {
            GrowingDescriptorPool* _my_growing_pool = nullptr;

            std::uint32_t _pool_id = 0;
            std::uint32_t _set_index = 0;

            std::shared_ptr<
                DescriptorPool::DescriptorSet
            > _set = {};

        private:
            ManagedDescriptorSet() = default;

        public:
            std::shared_ptr<DescriptorPool::DescriptorSet>
            get_set() {
                return _set;
            }
            ~ManagedDescriptorSet();

            friend GrowingDescriptorPool;
        };

    private:
        class ManagedPool {
            std::uint32_t _current_set_id = 0;
            std::shared_ptr<DescriptorPool> _pool = {};

        private:
            ManagedPool() = default;

            friend GrowingDescriptorPool;
        };

    private:
        std::uint64_t _current_pool = 0;
        std::uint32_t _sets_per_pool = 16;

        std::shared_ptr<ShaderModule>
        _shader_module = {};

        std::map<
            std::uint64_t,
            ManagedPool
        > _allocated_pools = {};
        std::shared_mutex _alloc_pool_mutex = {};

        std::unordered_map<
            std::uint64_t,
            std::vector<std::uint64_t>
        >
        _free_descriptor_set = {};
        std::shared_mutex _free_descriptor_set_mutex = {};

    private:
        std::optional<
            std::pair<std::uint64_t, std::uint64_t>
        > get_free_set();

        bool is_last_pool_has_free_space();
        void allocate_new_pool();

    public:
        GrowingDescriptorPool(
            std::shared_ptr<ShaderModule>& shader_module,
            std::uint32_t sets_per_pool
        ) {
            this->_sets_per_pool = sets_per_pool;
            this->_shader_module = shader_module;
            allocate_new_pool();
        }

        std::shared_ptr<ManagedDescriptorSet> allocate_new_set
        (
            std::uint32_t _set_id
        );

    };
}
