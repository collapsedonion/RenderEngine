//
// Created by Роман  Тимофеев on 11.05.2026.
//
module;

#include <re_typedefs.h>
#include <render_engine.h>
#include "stb_image.h"

#include <thread>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <VkBootstrap.h>
#include <ranges>

#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>
#include "uid.h"

#include "export_macro.h"

export module storage_buffer;

import render_engine_shares;
import image;

export struct RE_Buffer {
    size_t size = 0;
    UID uid = 0;
    bool host_accessible = false;
    bool random_accessible = false;
    vk::Buffer buffer {};
    VmaAllocation allocation {};
    vk::Semaphore* semaphore = nullptr;
};

export EXPORT_RE uint8_t *re_map_buffer(RE_pBuffer buffer) {
    auto *_buffer = static_cast<RE_Buffer *>(buffer);

    void *result;

    vmaMapMemory(vma_allocator, _buffer->allocation, &result);

    return static_cast<uint8_t *>(result);
}

export EXPORT_RE void re_unmap_buffer(RE_pBuffer buffer) {
    auto *_buffer = static_cast<RE_Buffer *>(buffer);
    vmaUnmapMemory(vma_allocator, _buffer->allocation);
}

export EXPORT_RE uint64_t re_get_buffer_size(RE_pBuffer buffer) {
    RE_Buffer *_buffer = static_cast<RE_Buffer *>(buffer);

    return _buffer->size;
}

export EXPORT_RE RE_pBuffer re_create_buffer(
    size_t byte_size,
    bool host,
    bool random_access
) {
    vk::BufferCreateInfo buffer_create_info{};
    buffer_create_info.size = byte_size;

    buffer_create_info.usage =
            vk::BufferUsageFlagBits::eUniformBuffer |
            vk::BufferUsageFlagBits::eStorageBuffer |
            vk::BufferUsageFlagBits::eTransferSrc |
            vk::BufferUsageFlagBits::eTransferDst |
            vk::BufferUsageFlagBits::eVertexBuffer;

    buffer_create_info.sharingMode = vk::SharingMode::eExclusive;

    uint32_t queue_family = vkb_device.get_queue_index(vkb::QueueType::graphics).value();
    buffer_create_info.setQueueFamilyIndices(
        {
            queue_family
        }
    );

    VmaAllocationCreateInfo allocation_create_info = {};
    allocation_create_info.usage =
            host ? VMA_MEMORY_USAGE_AUTO_PREFER_HOST : VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    allocation_create_info.flags = host
                                       ? (
                                           random_access
                                               ? VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT
                                               : VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                                       )
                                       : 0;

    auto *buffer = new RE_Buffer();

    VkBuffer vk_buffer = {};
    VmaAllocation vma_allocation = {};

    auto c_buffer_create_info = static_cast<VkBufferCreateInfo>(buffer_create_info);
    vkb_device_lock.lock();
    vmaCreateBuffer(
        vma_allocator,
        &c_buffer_create_info,
        &allocation_create_info,
        &vk_buffer,
        &vma_allocation,
        nullptr
    );
    vkb_device_lock.unlock();

    buffer->buffer = vk_buffer;
    buffer->allocation = vma_allocation;
    buffer->size = byte_size;
    buffer->random_accessible = random_access;
    buffer->host_accessible = host;
    buffer->uid = gen_uid();

    return reinterpret_cast<RE_pBuffer>(buffer);
}

struct Cache {
    Assimp::Importer importer{};
    const aiScene *scene = nullptr;
};

export EXPORT_RE uint32_t re_get_model_mesh_count(
    void *cache
) {
    return static_cast<Cache *>(cache)->scene->mNumMeshes;
}

export EXPORT_RE RE_pBuffer re_load_model_to_buffer(
    const char *path,
    bool include_uv,
    bool include_normal,
    bool flip_order,
    int64_t model_index,
    void **cache
) {
    const aiScene *scene;

    Assimp::Importer importer;

    auto start_flags = flip_order ? aiProcess_FlipWindingOrder : 0;

    if (cache == nullptr) {
        scene = importer.ReadFile(
            path,
            start_flags | aiProcess_Triangulate |
            aiProcess_GenNormals |
            aiProcess_GenUVCoords |
            aiProcess_JoinIdenticalVertices);
    } else if (*cache == nullptr) {
        *cache = new Cache();
        static_cast<Cache *>(*cache)->scene = static_cast<Cache *>(*cache)->importer.ReadFile(
            path,
            start_flags | aiProcess_Triangulate |
            aiProcess_GenNormals |
            aiProcess_GenUVCoords |
            aiProcess_JoinIdenticalVertices);
        scene = static_cast<Cache *>(*cache)->scene;
    } else {
        scene = static_cast<Cache *>(*cache)->scene;
    }

    if (model_index == -1) {
        return nullptr;
    }

    std::vector<float> vertex_data;

    if (scene == nullptr) {
        return nullptr;
    }

    if (model_index >= scene->mNumMeshes) {
        return nullptr;
    }

    auto mesh = scene->mMeshes[model_index];
    vertex_data.reserve(8 * mesh->mNumFaces);

    std::array<float, 3> vertex{};
    std::array<float, 2> uv{};
    std::array<float, 3> normal{};

    for (uint32_t face_index = 0; face_index < mesh->mNumFaces; face_index++) {
        auto face = mesh->mFaces[face_index];

        for (uint32_t vert_i = 0; vert_i < 3; vert_i++) {
            vertex[0] = mesh->mVertices[face.mIndices[vert_i]].x;
            vertex[1] = mesh->mVertices[face.mIndices[vert_i]].y;
            vertex[2] = mesh->mVertices[face.mIndices[vert_i]].z;

            vertex_data.insert(vertex_data.end(), vertex.begin(), vertex.end());

            if (include_uv) {
                uv[0] = mesh->mTextureCoords[0][face.mIndices[vert_i]].x;
                uv[1] = mesh->mTextureCoords[0][face.mIndices[vert_i]].y;
                vertex_data.insert(vertex_data.end(), uv.begin(), uv.end());
            }

            if (include_normal) {
                normal[0] = mesh->mNormals[face.mIndices[vert_i]].x;
                normal[1] = mesh->mNormals[face.mIndices[vert_i]].y;
                normal[2] = mesh->mNormals[face.mIndices[vert_i]].z;
                vertex_data.insert(vertex_data.end(), normal.begin(), normal.end());
            }
        }
    }

    RE_pBuffer new_buffer = re_create_buffer(
        sizeof(float) * vertex_data.size(),
        true,
        false
    );

    float *maped = reinterpret_cast<float *>(re_map_buffer(new_buffer));
    memcpy(maped, vertex_data.data(), vertex_data.size() * sizeof(float));
    re_unmap_buffer(new_buffer);

    return new_buffer;
}

EXPORT_RE void re_free_load_cache(void *cache) {
    auto *c = static_cast<Cache *>(cache);

    c->importer.FreeScene();
    delete c;
}

EXPORT_RE void re_fetch_image_file_extent(
    const char *path,
    uint32_t *width,
    uint32_t *height
) {
    int chanels;
    int _w, _h;
    stbi_info(path, &_w, &_h, &chanels);
    *width = _w;
    *height = _h;
}

EXPORT_RE RE_pBuffer re_load_image_to_buffer(
    const char *path,
    uint32_t *_width,
    uint32_t *_height
) {
    int width, height, channels;

    stbi_uc *image_data = stbi_load(
        path,
        &width,
        &height,
        &channels,
        4
    );

    RE_pBuffer buffer = re_create_buffer(
        width * height * 4 * sizeof(uint8_t),
        true,
        false
    );

    auto *buffer_data = reinterpret_cast<uint8_t *>(re_map_buffer(buffer));

    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            uint32_t raw_index = x + y * width;
            uint32_t r = raw_index * 4;
            uint32_t g = raw_index * 4 + 1;
            uint32_t b = raw_index * 4 + 2;
            uint32_t a = raw_index * 4 + 3;

            buffer_data[r] = image_data[r];
            buffer_data[g] = image_data[g];
            buffer_data[b] = image_data[b];
            buffer_data[a] = image_data[a];
        }
    }

    re_unmap_buffer(buffer);

    stbi_image_free(image_data);

    *_width = width;
    *_height = height;

    return buffer;
}

EXPORT_RE void re_free_buffer(
    RE_pBuffer buffer
) {
    auto *_buffer = reinterpret_cast<RE_Buffer *>(buffer);

    vkb_device_lock.lock();
    vmaDestroyBuffer(vma_allocator, _buffer->buffer, _buffer->allocation);
    vkb_device_lock.unlock();
    delete _buffer;
}
