//
// Created by Onion on 10.08.2026.
//
module;
#include <vk_mem_alloc.h>

#include "assimp/scene.h"
#ifdef __APPLE__
#include <cstdint>
#include <cinttypes>
#include <vulkan/vulkan.hpp>
#endif
#include <assimp/Importer.hpp>

export module Buffer;

#ifdef __linux__
export import std;
export import vulkan;
#endif

import Resource;

export namespace RenderEngine
{
    class RawBuffer: public Resource
    {
        std::size_t _size = 0;
        bool _host_access = false;
        bool _random_access = false;
        vk::Buffer _buffer = {};
        VmaAllocation _allocation = {};
        vk::Semaphore* _waiting_semaphore = nullptr;
        vk::DescriptorBufferInfo _set_info = {};
        std::uint64_t _uid = 0;

        bool _valid = false;

    public:
        RawBuffer() = default;
        ~RawBuffer() final;

    private:

        RawBuffer(size_t byte_size, bool host_access, bool random_access);

    public:

        /**
         * Creates buffer.
         *
         * @param [in] byte_size size in bytes of memory to allocate
         * @param [in] host_access set this if you want to use re_map_buffer for direct access to buffer contents
         * @param [in] random_access set this if access to buffer may be not sequential for API and driver specific optimisation
         *
         * @return Shared pointer to RawBuffer
        */
        static std::shared_ptr<RawBuffer> create(
            size_t byte_size,
            bool host_access = false,
            bool random_access = false
            )
        {
            return std::shared_ptr<RawBuffer>(
                new RawBuffer(byte_size,
                host_access,
                random_access)
            );
        }

        /**
         * Returns buffers internal size
         *
         * @return Size of buffer in bytes
         */
        [[nodiscard]] std::size_t size() const
        {
            return _size;
        }

        /**
         * Maps buffer to host memory
         * @note Must be unmapped manually
         *
         * @return Pointer to buffer bytes
         */
        [[nodiscard]] std::uint8_t* map();

        /**
         * Unmaps buffer
         */
        void unmap();


        /**
         *
         * @return Handler to vulkan buffer
         */
        [[nodiscard]]
        vk::Buffer get_raw_buffer() const
        {
            return _buffer;
        }

        [[nodiscard]]
        std::uint64_t get_uid() override
        {
            return _uid;
        }

        vk::WriteDescriptorSet get_descriptor_set_write(
            vk::DescriptorType type
        ) override;
        vk::Semaphore** get_semaphore_ref() override
        {
            return &_waiting_semaphore;
        }
    };

    class ModelLoader
    {
        Assimp::Importer _importer {};
        const aiScene* _scene = nullptr;

    public:
        /**
         * Creates Model Loader
         * @param [in] path path to file to load mesh from
         * @param [in] flip_vertex_winding tells to flip vertex winding or not
         */
        ModelLoader(
            const std::filesystem::path& path,
            bool flip_vertex_winding
        );
        ~ModelLoader();

        /**
         * Loads Mesh to RawBuffer
         *
         * Resulting data will be arranged as follows:
         * vertex (float * 3); uv's (float * 2); normals (float * 3);
         * if uv's and normals not presented then format will shrink
         *
         * @param [in] model_index index of mesh to load
         * @param [in] include_uvs includes uv's
         * @param [in] include_normals includes normals
         * @return Shared pointer to new RawBuffer
         */
        [[nodiscard]]
        std::shared_ptr<RawBuffer> load_mesh(
            std::uint64_t model_index,
            bool include_uvs,
            bool include_normals
        ) const;

        /**
         *
         * @return Count of mesh that can be load
         */
        [[nodiscard]]
        std::size_t get_mesh_count() const
        {
            return _scene->mNumMeshes;
        }
    };

    /**
     * Load texture from file to buffer
     *
     * @param [in] path must be string with relative/full path to loaded textures
     *
     * @return Handle to new buffer holding decoded textures pixel data 8bits per chanel RGBA, flattened row major. Image width. Image height;
     */
     std::tuple<std::shared_ptr<RawBuffer>, uint32_t, uint32_t> load_texture_to_buffer(
         const std::filesystem::path& path
     );
}
