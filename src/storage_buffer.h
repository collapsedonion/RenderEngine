//
// Created by Роман  Тимофеев on 11.05.2026.
//

#ifndef RENDERENGINE_RENDERBUFFER_H
#define RENDERENGINE_RENDERBUFFER_H
#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>

#include "uid.h"

struct RE_Buffer {
    size_t size = 0;
    UID uid = 0;
    bool host_accessible = false;
    bool random_accessible = false;
    vk::Buffer buffer {};
    VmaAllocation allocation {};
    vk::Semaphore* semaphore = nullptr;
};

#endif //RENDERENGINE_RENDERBUFFER_H
