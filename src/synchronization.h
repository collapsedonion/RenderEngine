//
// Created by Роман  Тимофеев on 04.05.2026.
//

#ifndef RENDERENGINE_SYNCHRONIZATION_H
#define RENDERENGINE_SYNCHRONIZATION_H
#include <vulkan/vulkan.hpp>

void init_semaphores();
void free_semaphores();

//accepts array of pointers to semaphore pointers
std::pair<vk::Semaphore, bool> get_next_semaphore(uint32_t resource_count, vk::Semaphore*** resource_field);
void free_semaphore(vk::Semaphore* semaphore);
bool is_waited(vk::Semaphore* semaphore);

#endif //RENDERENGINE_SYNCHRONIZATION_H
