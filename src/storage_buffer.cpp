//
// Created by Onion on 22.07.2026.
//

module;

#include <re_typedefs.h>
#include "stb_image.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <VkBootstrap.h>

#include <vk_mem_alloc.h>
#include "uid.h"

#include "export_macro.h"

#if defined(__APPLE__)
#include <array>
#include <vector>
#include <filesystem>
#include <vulkan/vulkan.hpp>
#endif

module Buffer;

#if defined(__linux__)
import vulkan;
import std;
#endif
import render_engine_shares;

using namespace RenderEngine;

struct Cache {
    Assimp::Importer importer{};
    const aiScene *scene = nullptr;
};

std::uint8_t* RawBuffer::map() {
    if (!_valid)
    {
        return nullptr;
    }

    if (!_host_access)
    {
        return nullptr;
    }

    void *result;

    vmaMapMemory(vma_allocator, _allocation, &result);

    return static_cast<std::uint8_t *>(result);
}

void RawBuffer::unmap() {
    if (!_valid)
    {
        return;
    }

    vmaUnmapMemory(vma_allocator, _allocation);
}

RawBuffer::RawBuffer(size_t byte_size, bool host, bool random_access)
{
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

    _buffer = vk_buffer;
    _allocation = vma_allocation;
    _size = byte_size;
    _random_access = random_access;
    _host_access = host;
    _valid = true;
    _set_info.buffer = _buffer;
    _set_info.offset = 0;
    _set_info.range = vk::WholeSize;
    _uid = gen_uid();
}

ModelLoader::ModelLoader(const std::filesystem::path& path, bool flip_order)
{
    auto start_flags = flip_order ? aiProcess_FlipWindingOrder : 0;
    _scene = _importer.ReadFile(
        path,
        start_flags | aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_GenUVCoords |
        aiProcess_JoinIdenticalVertices);
}

ModelLoader::~ModelLoader()
{
    _importer.FreeScene();
}

std::shared_ptr<RawBuffer> ModelLoader::load_mesh(
    std::uint64_t model_index,
    bool include_uvs,
    bool include_normals
) const
{
    auto mesh = _scene->mMeshes[model_index];
    std::vector<float> vertex_data {};
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

            if (include_uvs) {
                uv[0] = mesh->mTextureCoords[0][face.mIndices[vert_i]].x;
                uv[1] = mesh->mTextureCoords[0][face.mIndices[vert_i]].y;
                vertex_data.insert(vertex_data.end(), uv.begin(), uv.end());
            }

            if (include_normals) {
                normal[0] = mesh->mNormals[face.mIndices[vert_i]].x;
                normal[1] = mesh->mNormals[face.mIndices[vert_i]].y;
                normal[2] = mesh->mNormals[face.mIndices[vert_i]].z;
                vertex_data.insert(vertex_data.end(), normal.begin(), normal.end());
            }
        }
    }

    auto new_buffer = RawBuffer::create(
        sizeof(float) * vertex_data.size(),
        true,
        false
    );

    float *maped = reinterpret_cast<float *>(new_buffer->map());
    memcpy(maped, vertex_data.data(), vertex_data.size() * sizeof(float));
    new_buffer->unmap();

    return new_buffer;
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

std::tuple<std::shared_ptr<RawBuffer>, uint32_t, uint32_t> RenderEngine::load_texture_to_buffer(
    const std::filesystem::path& path
)
{
    int width, height, channels;

    stbi_uc *image_data = stbi_load(
        path.c_str(),
        &width,
        &height,
        &channels,
        4
    );

    auto buffer = RawBuffer::create(
        width * height * 4 * sizeof(uint8_t),
        true,
        false
    );

    auto *buffer_data = buffer->map();

    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            const uint32_t raw_index = x + y * width;
            const uint32_t r = raw_index * 4;
            const uint32_t g = raw_index * 4 + 1;
            const uint32_t b = raw_index * 4 + 2;
            const uint32_t a = raw_index * 4 + 3;

            buffer_data[r] = image_data[r];
            buffer_data[g] = image_data[g];
            buffer_data[b] = image_data[b];
            buffer_data[a] = image_data[a];
        }
    }

    buffer->unmap();

    stbi_image_free(image_data);

    return {buffer, width, height};
}

vk::WriteDescriptorSet RawBuffer::get_descriptor_set_write(
    vk::DescriptorType type
)
{
    vk::WriteDescriptorSet write_descriptor_set = {};
    write_descriptor_set.descriptorCount = 1;
    write_descriptor_set.pBufferInfo = &this->_set_info;
    write_descriptor_set.descriptorType = type;
    return write_descriptor_set;
}

RawBuffer::~RawBuffer() {
    if (!_valid)
    {
        return;
    }

    vkb_device_lock.lock();
    vmaDestroyBuffer(vma_allocator, _buffer, _allocation);
    vkb_device_lock.unlock();
}
