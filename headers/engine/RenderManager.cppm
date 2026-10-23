//
// Created by Роман  Тимофеев on 18.08.2026.
//
module;

#include <glfw/glfw3.h>
#include <glm/matrix.hpp>

#include "glm/gtx/io.hpp"

#ifdef __APPLE__
#include <filesystem>
#include <memory>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#endif

export module RenderManager;
import ShaderModule;
import Image;
import Buffer;
import RenderEngineEnumeration;
import GrowingDescriptorPool;
import RenderObject;
import SceneData;
import dynamic_dispatchable_iterator;
import Resource;

namespace RenderEngine {
    export class RenderManager;

    inline std::string GEOMETRY_PASS_NAME = "ENGINE_GEOMETRY";
    inline std::string MATERIAL_OUTPUT_NAME = "material_output";

    inline std::string LP_RESULT_NAME = "result_texture";
    inline std::string LP_NORMAL_MAP_NAME = "normal_map";
    inline std::string LP_POSITION_MAP_NAME = "position_map";
    inline std::string LP_MATERIAL_MAP_NAME = "material_map";

    class RenderTarget {
        std::shared_ptr<Image> _color;
        std::shared_ptr<Image> _depth;
        std::shared_ptr<Image> _world_position;
        std::shared_ptr<Image> _world_normals;
        std::shared_ptr<Image> _light_map;
        std::shared_ptr<Image> _material_data;

        std::pair<std::uint32_t, std::uint32_t> _dimensions;

    public:
        RenderTarget(
            std::uint32_t width,
            std::uint32_t height,
            std::optional<std::pair<std::uint32_t, uint32_t>> separate_light_resolution = {}
        );

        auto get_images() {
            auto collection = std::array<Image *, 4>{
                _color.get(),
                _world_position.get(),
                _world_normals.get(),
                _material_data.get()
            };

            return RangedIterator(collection);
        }

        std::shared_ptr<Image> get_light_map() {
            return _light_map;
        }

        std::shared_ptr<Image> get_world_position() {
            return _world_position;
        }

        std::shared_ptr<Image> get_world_normals() {
            return _world_normals;
        }

        auto get_dimensions() {
            return _dimensions;
        }

        std::shared_ptr<Image> get_depth_image() {
            return _depth;
        }

        std::shared_ptr<Image> get_material_data() {
            return _material_data;
        }

        RenderTarget(RenderTarget &s) = default;

        RenderTarget(RenderTarget &&s) = default;

        std::shared_ptr<Image> get_image_by_channel(
            RenderTargetChannel channel
        ) {
            switch (channel) {
                case RenderTargetChannel::COLOR:
                    return _color;
                case RenderTargetChannel::DEPTH:
                    return _depth;
                case RenderTargetChannel::WORLD_POSITION:
                    return _world_position;
                case RenderTargetChannel::WORLD_NORMAL:
                    return _world_normals;
                case RenderTargetChannel::LIGHT_MAP:
                    return _light_map;
                case RenderTargetChannel::MATERIAL:
                    return _material_data;

            }
            return {};
        }
    };

    class Material {
        std::shared_ptr<ShaderModule> _shader;
        std::unique_ptr<GrowingDescriptorPool> _my_pool;

    public:
        Material(
            std::shared_ptr<ShaderModule> &shader
        ) : _shader(shader) {
            _my_pool =
                    std::unique_ptr<GrowingDescriptorPool>(
                        new GrowingDescriptorPool(shader, 16)
                    );
        }

        std::shared_ptr<Image> compute_to_image(
            std::uint32_t width,
            std::uint32_t height,
            const std::string &shader_name,
            std::optional<Iterator<
                std::tuple<std::string &, Resource *>
            > &> resource
        );
    };

    class RenderManager {
        static bool _initialized;
        static std::shared_ptr<ShaderModule> _geometry_shader_module;
        static std::shared_ptr<ShaderModule> _light_pass_module;

        static std::unordered_set<
            std::string
        > _window_sized_scenes;
        static std::unordered_map<
            std::string,
            RenderTarget
        > _render_targets;
        static std::unordered_map<
            std::string,
            SceneData
        > _scenes;
        static std::unordered_map<
            std::string,
            Material
        > _materials;


        static GLFWwindow *_window;
        static GrowingDescriptorPool *_gdp;
        static GrowingDescriptorPool *_light_pass_descriptor_pool;

    public:
        static void update_window_size(
            std::uint32_t width,
            std::uint32_t height
        );

        static void init(
            const std::filesystem::path &gp_code_path,
            const std::filesystem::path &light_code_path
        );

        static void add_scene(
            const std::string &name
        );

        static void remove_scene(
            const std::string &name
        );

        static void add_light_source(
            const std::string& scene_name,
            const std::string& light_name,
            LightSourceType type
        ) {
            if (!_scenes.contains(scene_name)) {
                return;
            }

            _scenes.at(scene_name).add_light_source(
                light_name,
                type
            );
        }

        static void remove_light_source(
            const std::string& scene_name,
            const std::string& light_name
        ) {
            if (!_scenes.contains(scene_name)) {
                return;
            }

            _scenes.at(scene_name).remove_light_source(
                light_name
            );
        }

        static void set_light_source_color(
            const std::string &scene_name,
            const std::string &light_name,
            const glm::vec3 &color
        ) {
            if (!_scenes.contains(scene_name)) {
                return;
            }

            _scenes.at(scene_name).set_light_source_color(
                light_name,
                color
            );
        }

        static void set_light_source_transform(
            const std::string &scene_name,
            const std::string &light_name,
            const glm::vec3& position,
            const glm::vec3& direction
        ) {
            if (!_scenes.contains(scene_name)) {
                return;
            }

            _scenes.at(scene_name).set_light_source_transform(
                light_name,
                position,
                direction
            );
        }

        static void add_render_object_to_scene(
            const std::string &scene_name,
            const std::string &object_name,
            const std::string &model_name
        ) {
            if (!_scenes.contains(scene_name)) {
                return;
            }

            auto new_set = _gdp->allocate_new_set(1);

            _scenes.at(scene_name).add_render_object(
                object_name,
                model_name,
                new_set
            );
        }

        static void set_render_objects_transform(
            const std::string &scene_name,
            const std::string &object_name,
            const glm::mat4x4 &transform
        ) {
            if (!_scenes.contains(scene_name)) {
                return;
            }

            _scenes.at(scene_name).set_render_objects_transform(
                object_name,
                transform
            );
        }

        static void set_render_objects_color(
            const std::string &scene_name,
            const std::string &object_name,
            const glm::vec3 &color
        ) {
            if (!_scenes.contains(scene_name)) {
                return;
            }

            _scenes.at(scene_name).set_render_objects_color(
                object_name,
                color
            );
        }

        static void set_render_objects_coloring_type(
            const std::string &scene_name,
            const std::string &object_name,
            ColoringType coloring_type
        ) {
            if (!_scenes.contains(scene_name)) {
                return;
            }

            _scenes.at(scene_name).set_render_objects_coloring_type(
                object_name,
                coloring_type
            );
        }

        static void remove_render_object_from_scene(
            const std::string &scene_name,
            const std::string &object_name
        ) {
            if (!_scenes.contains(scene_name)) {
                return;
            }

            _scenes.at(scene_name).remove_render_object(object_name);
        }

        static void add_render_target(
            const std::string &name,
            std::uint32_t width,
            std::uint32_t height,
            std::optional<std::pair<std::uint32_t, uint32_t>> separate_light_resolution = {}
        );

        static void render_scene_to_target(
            const std::string &target_name,
            const std::string &scene_name
        );

        static void bake_light_to_target(
            const std::string &target_name,
            const std::string &scene_name
        );

        static void set_scene_fov(const std::string &name, float fov);

        static void set_scene_aspect(
            const std::string &name,
            std::uint32_t width,
            std::uint32_t height
        ) {
            if (!_scenes.contains(name)) {
                return;
            }

            _scenes.at(name).set_aspect(width, height);
        }

        static void set_scene_camera_eye(
            const std::string &name,
            glm::vec3 camera_eye
        ) {
            if (!_scenes.contains(name)) {
                return;
            }

            _scenes.at(name).set_camera_eye(camera_eye);
        }

        static void set_scene_aspect_source(
            const std::string &name,
            SceneResolutionSource source
        );

        static void display_render_target(
            const std::string &name,
            RenderTargetChannel channel_to_present
        );

        /**
         * Material must have only 1 descriptor set with index 0
         *
         * @param path_to_material
         */
        static void add_material(
            const std::string &path_to_material,
            const std::string &material_name
        );

        static std::shared_ptr<Image> compute_material(
            const std::string &material_name,
            const std::string &shader_name,
            std::uint32_t width,
            std::uint32_t height,
            std::optional<Iterator<
                std::tuple<std::string &, Resource *>
            > &>resource = {}
        ) {
            if (!_materials.contains(material_name)) {
                return {};
            }

            return _materials.at(material_name).compute_to_image(
                width,
                height,
                shader_name,
                resource
            );
        }

        static void remove_material(
            const std::string &material_name
        ) {
            if (!_scenes.contains(material_name)) {
                return;
            }

            _materials.erase(material_name);
        }

        static void set_render_objects_texture(
            const std::string &scene_name,
            const std::string &object_name,
            const std::shared_ptr<Image> &img
        ) {
            if (!_scenes.contains(scene_name)) {
                return;
            }

            _scenes.at(scene_name).set_object_texture(
                object_name,
                img
            );
        }

        //loop_body must return true if execution finished
        static void start_loop(
            std::function<bool(double dt)> loop_body
        );

        static void stop();
    };
}
