//
// Created by Роман  Тимофеев on 04.05.2026.
//
module;

#include <cstdint>

export module synchronization;

export import vulkan;
import std;

export void init_semaphores();

export std::pair<vk::Semaphore, bool> get_next_semaphore(
    uint32_t resource_count,
    vk::Semaphore ***resource_field
);

export bool is_waited(vk::Semaphore* semaphore);

export void free_semaphore(vk::Semaphore* semaphore);

export void free_semaphores();
