//
// Created by Onion on 22.07.2026.
//

module;

#include <cstdint>

module synchronization;

import vulkan;
import std;
import render_engine_shares;

constexpr uint32_t SEMAPHORES_COUNT = 32;

struct LastAccessResource {
    bool accessed = false;
    std::vector<vk::Semaphore **> semaphore;
};

vk::Semaphore allocated_semaphores[SEMAPHORES_COUNT] = {};
LastAccessResource semaphores_resources[SEMAPHORES_COUNT] = {};
std::pmr::unordered_map<vk::Semaphore *, uint32_t> semaphore_reverse_map{};

uint32_t last_allocated;

void init_semaphores() {
    last_allocated = SEMAPHORES_COUNT;

    vk::Device device = vkb_device.device;

    for (uint32_t i = 0; i < SEMAPHORES_COUNT; i++) {
        vk::SemaphoreCreateInfo info = {};

        allocated_semaphores[i] = device.createSemaphore(
            info
        );

        semaphores_resources[i] = {};

        semaphore_reverse_map.insert({&allocated_semaphores[i], i});
    }
}

std::pair<vk::Semaphore, bool> get_next_semaphore(
    uint32_t resource_count,
    vk::Semaphore ***resource_field
) {
    if (last_allocated == 0) {
        last_allocated = SEMAPHORES_COUNT;
    }

    vk::Semaphore *new_semaphore = &allocated_semaphores[--last_allocated];
    auto &last_resource = semaphores_resources[last_allocated];
    bool wait = false;

    if (last_resource.accessed) {
        for (auto &res: last_resource.semaphore) {
            if (*res == new_semaphore) {
                *res = nullptr;
            }
        }

        last_resource.semaphore.clear();

        wait = true;
    }

    last_resource.semaphore.resize(resource_count);

    for (uint32_t i = 0; i < resource_count; i++) {
        *resource_field[i] = &allocated_semaphores[last_allocated];
        last_resource.semaphore[i] = resource_field[i];
    }

    last_resource.accessed = true;

    return {*new_semaphore, wait};
}

void test_semaphores() {
    std::vector<vk::Semaphore *> pseudo_resources;
    std::vector<vk::Semaphore **> semaphores;
    pseudo_resources.reserve(20);

    for (uint32_t i = 0; i < 20; i++) {
        pseudo_resources.push_back(nullptr);
        semaphores.push_back(&pseudo_resources[i]);
    }

    get_next_semaphore(5, semaphores.data());
    get_next_semaphore(5, semaphores.data() + 5);
    get_next_semaphore(5, semaphores.data() + 10);
    get_next_semaphore(5, semaphores.data() + 15);
    get_next_semaphore(5, semaphores.data() + 5);
}

bool is_waited(vk::Semaphore* semaphore) {
   return semaphore_reverse_map.contains(semaphore) && !semaphores_resources[semaphore_reverse_map[semaphore]].accessed;
}

void free_semaphore(vk::Semaphore* semaphore) {
    if (semaphore_reverse_map.contains(semaphore)) {
        semaphores_resources[semaphore_reverse_map[semaphore]].accessed = false;
    }
}

void free_semaphores() {
    vk::Device device = vkb_device.device;
    for (auto &allocated_semaphore: allocated_semaphores) {
        device.destroy(allocated_semaphore);
    }
}
