//
// Created by Onion on 12.08.2026.
//
module;
#include <re_typedefs.h>

#ifdef __APPLE__
#include <vector>
#include <vulkan/vulkan.hpp>
#include <map>
#include <unordered_map>
#include <memory>
#endif

export module ShaderModule;
#ifdef __linux__
export import std;
export import vulkan;
#endif
export import Buffer;

export namespace RenderEngine
{

    class ShaderModule
    {
        struct DescriptorPoolSizes
        {
            std::uint32_t storage_buffer_count = 0;
            std::uint32_t uniform_buffer_count = 0;
            std::uint32_t storage_image_count = 0;
            std::uint32_t combined_image_count = 0;
        };

        struct GraphicsPipeline
        {
            vk::Pipeline pl;
            std::vector<vk::VertexInputAttributeDescription> vertex_buffer_input;
            std::size_t bytes_per_vertex;
            std::size_t image_binding_count;
            bool depth_enable;
        };

    private:
        vk::ShaderModule _sm;
        vk::PipelineLayout _pl_layout;
        std::map<std::uint32_t, vk::DescriptorSetLayout> _set_layouts = {};

        std::unordered_map<
            std::string,
            std::tuple<std::uint32_t, std::uint32_t, vk::DescriptorType> //set; binding; type;
        > _sets_bindings {};

        std::unordered_map<
            std::string,
            std::vector<vk::VertexInputAttributeDescription>
        > _vertex_shaders {};

        std::unordered_map<
            std::string,
            std::size_t
        >  _fragment_shaders{};

        std::unordered_map<
            std::string,
            vk::Pipeline
        > _compute_pipelines;

        std::unordered_map<
            std::string,
            GraphicsPipeline
        > _graphics_pipelines;

        DescriptorPoolSizes _pool_sizes{};

    private:
        ShaderModule(
            RE_pSpirVCode pCode
        );

    public:
        ~ShaderModule();

        static std::shared_ptr<ShaderModule> create(
            RE_pSpirVCode pCode
        )
        {
            return std::shared_ptr<ShaderModule>(new ShaderModule(pCode));
        }

        std::vector<vk::DescriptorPoolSize> get_pool_sizes(
            std::size_t reserved_sets
        );

        void register_render_pipeline(
            const std::string& pipeline_name,
            const std::string& vertex_name,
            const std::string& fragment_name,
            bool depth_test_enable
        );

        std::shared_ptr<RawBuffer> allocate_compatible_vertex_buffer(
            const std::string& graphics_pipeline_name,
            std::size_t vertex_count
        );

        [[nodiscard]]
        vk::DescriptorSetLayout get_set_layout(
            std::uint32_t index
        ) const
        {
            return _set_layouts.at(index);
        }

        [[nodiscard]]
        std::tuple<std::uint32_t, std::uint32_t, vk::DescriptorType> get_binding_info(
            const std::string& name
        ) const
        {
            return _sets_bindings.at(name);
        }

        [[nodiscard]]
        vk::Pipeline get_graphics_pipeline(
            const std::string& name
        ) const
        {
            return _graphics_pipelines.at(name).pl;
        }

        [[nodiscard]]
        vk::Pipeline get_compute_pipeline(
            const std::string& name
        ) const
        {
            return _compute_pipelines.at(name);
        }

        [[nodiscard]]
        vk::PipelineLayout get_pipeline_layout() const
        {
            return _pl_layout;
        }

        [[nodiscard]]
        std::size_t get_pipeline_vertex_size(const std::string& name) const
        {
            return _graphics_pipelines.at(name).bytes_per_vertex;
        }

        [[nodiscard]]
        std::size_t get_max_binding_count() const
        {
            return _sets_bindings.size();
        }
    };
}