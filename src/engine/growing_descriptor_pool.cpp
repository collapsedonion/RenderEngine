//
// Created by Роман  Тимофеев on 24.08.2026.
//
module;

#ifdef __APPLE__
#include <cinttypes>
#include <ranges>
#include <memory>
#include <vector>
#include <shared_mutex>
#endif

module GrowingDescriptorPool;

using namespace RenderEngine;

std::optional<
    std::pair<std::uint64_t, std::uint64_t>
>   GrowingDescriptorPool::get_free_set() {

    std::vector<std::uint64_t>* v = nullptr;
    std::uint64_t pool_id;

    {
        std::shared_lock lock{_free_descriptor_set_mutex};

        if (this->_free_descriptor_set.empty()) {
            return {};
        }

        auto& [_pool_id, _v]
        = *this->_free_descriptor_set.begin();

        if (_v.size() == 0) {
            return {};
        }
        v = &_v;
        pool_id = _pool_id;
    }

    std::unique_lock lock{_free_descriptor_set_mutex};

    auto set_id = v->back();
    v->pop_back();

    if (v->size() == 1) {
        this->_free_descriptor_set.erase(pool_id);
    }

    return {{pool_id, set_id}};
}

std::shared_ptr<GrowingDescriptorPool::ManagedDescriptorSet>
GrowingDescriptorPool::allocate_new_set(
    std::uint32_t _set_id
) {
    auto free_set = get_free_set();

    std::shared_lock lock{_alloc_pool_mutex};

    std::uint64_t id;
    std::uint32_t new_set_id;
    ManagedPool* last_pool;

    if (!free_set.has_value()) {
        if (!is_last_pool_has_free_space()) {
            allocate_new_pool();
        }

        auto& [_id, _last_pool] = *(--_allocated_pools.end());
        id = _id;
        last_pool = &_last_pool;
        new_set_id = last_pool->_current_set_id++;
    }else {
        id = free_set.value().first;
        new_set_id = free_set.value().second;
        last_pool = &_allocated_pools.at(id);
    }

    auto range_it = RangedIterator(std::views::single(_set_id));

    auto new_set =
        last_pool->_pool->create_sets(range_it)[0];

    auto managed_descriptor_set = new ManagedDescriptorSet();

    managed_descriptor_set->_my_growing_pool = this;
    managed_descriptor_set->_pool_id = id;
    managed_descriptor_set->_set_index = new_set_id;
    managed_descriptor_set->_set = new_set;

    auto shared_ds = std::shared_ptr<ManagedDescriptorSet>(managed_descriptor_set);

    return shared_ds;
}

void GrowingDescriptorPool::allocate_new_pool() {
    auto new_pool = DescriptorPool::create(
        this->_shader_module,
        this->_sets_per_pool
    );

    auto new_managed_pool = ManagedPool();
    new_managed_pool._pool = new_pool;

    {
        std::unique_lock lock{_alloc_pool_mutex};
        _allocated_pools.insert({_current_pool++, new_managed_pool});
    }
}

bool GrowingDescriptorPool::is_last_pool_has_free_space() {
    std::shared_lock lock{_alloc_pool_mutex};
    auto& last_pool = (--_allocated_pools.end())->second;
    return last_pool._pool->get_pool_size() > last_pool._current_set_id;
}

GrowingDescriptorPool::ManagedDescriptorSet::~ManagedDescriptorSet() {
    std::shared_lock lock{_my_growing_pool->_free_descriptor_set_mutex};

    if (!_my_growing_pool->_free_descriptor_set.contains(this->_pool_id)) {
        _my_growing_pool->_free_descriptor_set.insert({});
    }

    _my_growing_pool->_free_descriptor_set[this->_pool_id].push_back(
        this->_set_index
    );
}