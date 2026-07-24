//
// Created by Роман  Тимофеев on 11.05.2026.
//
module;

#include <re_typedefs.h>
#include <vk_mem_alloc.h>
#include "uid.h"
#include "export_macro.h"

export module storage_buffer;

export import vulkan;

export struct RE_Buffer {
    size_t size = 0;
    UID uid = 0;
    bool host_accessible = false;
    bool random_accessible = false;
    vk::Buffer buffer {};
    VmaAllocation allocation {};
    vk::Semaphore* semaphore = nullptr;
};

export EXPORT_RE uint8_t *re_map_buffer(RE_pBuffer buffer);

export EXPORT_RE void re_unmap_buffer(RE_pBuffer buffer);

export EXPORT_RE uint64_t re_get_buffer_size(RE_pBuffer buffer);

export EXPORT_RE RE_pBuffer re_create_buffer(
    size_t byte_size,
    bool host,
    bool random_access
);

export EXPORT_RE uint32_t re_get_model_mesh_count(
    void *cache
);

export EXPORT_RE RE_pBuffer re_load_model_to_buffer(
    const char *path,
    bool include_uv,
    bool include_normal,
    bool flip_order,
    int64_t model_index,
    void **cache
);

export EXPORT_RE void re_free_load_cache(void *cache);

export EXPORT_RE void re_fetch_image_file_extent(
    const char *path,
    uint32_t *width,
    uint32_t *height
);

export EXPORT_RE RE_pBuffer re_load_image_to_buffer(
    const char *path,
    uint32_t *_width,
    uint32_t *_height
);

export EXPORT_RE void re_free_buffer(
    RE_pBuffer buffer
);
