//
// Created by Роман  Тимофеев on 25.08.2026.
//
module;

#include <glm/matrix.hpp>
#include <glm/vec3.hpp>

#include "glm/fwd.hpp"

#ifdef __APPLE__
#include <memory>
#include <unordered_map>
#include <ranges>
#endif

export module SceneData;
import GrowingDescriptorPool;
import RenderObject;
import RenderEngineEnumeration;
import Image;

namespace RenderEngine {

    inline std::string GP_CAMERA_MATRIX_NAME = "CAMERA_PROJECTION_MATRIX";
    inline std::string GP_POSITION_MATRIX_NAME = "WORLD_MATRIX";
    inline std::string GP_TEXTURE_NAME = "OBJECT_TEXTURE";
    inline std::string GP_METADATA_NAME = "MTD";

    inline std::string LP_LIGHT_INFO_NAME = "LIGHT_INFO";

    struct LightInfo {
        alignas(8) glm::uint64_t buffer_address = 0;
        alignas(16) glm::vec3 ambient_color = glm::vec3(0.1, 0.1, 0.1);
        alignas(4) std::uint32_t light_source_count = 0;
        alignas(16) glm::vec3 camera_position = glm::vec3(0.0f, 0.0f, 0.0f);
    };

    struct WorldTransform {
        alignas(16) glm::mat4x4 position_matrix = glm::mat4x4(1.0f);
        alignas(16) glm::mat4x4 normal_matrix = glm::mat3x3(1.0f);
    };

    struct CameraTransform {
        alignas(16) glm::mat4x4 projection_matrix = glm::mat4x4(1.0f);
        alignas(16) glm::mat4x4 view_matrix = glm::mat4x4(1.0f);
    };

    struct LightSource {
        alignas(16) glm::vec4 light_color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        alignas(16) glm::vec4 position = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
        alignas(16) glm::vec4 direction = glm::vec4(
            1.0f / std::sqrt(2.0), 1.0f / std::sqrt(2.0), 0.0, 0.0f
        );
        alignas(4) LightSourceType type = LightSourceType::DIRECTIONAL;
    };

    export class SceneData {

        std::shared_ptr<GrowingDescriptorPool::ManagedDescriptorSet>
        _camera_descriptor_set;

        std::shared_ptr<GrowingDescriptorPool::ManagedDescriptorSet>
        _light_source_descriptor_set;

        std::shared_ptr<RawBuffer> _scene_light_info;
        std::shared_ptr<RawBuffer> _light_source_ptr_buffer;

        bool _is_light_sources_changed = true;
        std::unordered_map<
            std::string,
            std::shared_ptr<RawBuffer>
        > _light_sources;

        struct ObjectMetadata {
            alignas(4) ColoringType c_type = ColoringType::UV_AS_COLOR;
            alignas(16) glm::vec4 color;
            alignas(4) float specular_reflection = 0.8;
            alignas(4) float difuse_reflection = 0.8;
            alignas(4) float ambient_reflection = 1.0;
            alignas(4) float shininess = 2.0;
        };

        struct RObject {
            RenderObject _object;
            std::vector<std::weak_ptr<DescriptorPool::DescriptorSet>> _sets;
            std::shared_ptr<RawBuffer> _position_buffers;
            std::shared_ptr<RawBuffer> _meta_data_buffer;
            std::shared_ptr<GrowingDescriptorPool::ManagedDescriptorSet> _position_descriptor_set;
        };

        std::unordered_map<
            std::string,
            RObject
        > _render_objects;

        std::shared_ptr<RawBuffer> _camera_buffer;

        SceneResolutionSource _resolution_source = SceneResolutionSource::MANUAL;

        bool _camera_updated = true;
        bool _light_affecting_camera_update = true;
        float _camera_fov = 45.0f;
        float _camera_aspect = 1.0f;
        glm::vec3 _camera_position = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 _camera_direction = glm::vec3(0.0f, 0.0f, -1.0f);
        glm::vec3 _camera_up = glm::vec3(0.0f, -1.0f, 0.0f);
        std::pair<float, float> _camera_range = {0.1, 100};

    private:
        void update_light_camera();

        void update_light_source_ptr();

        void update_camera_state(
            std::size_t render_target_width = 0,
            std::size_t render_target_height = 0
        );

    public:
        explicit SceneData(
            std::shared_ptr<GrowingDescriptorPool::ManagedDescriptorSet>&
            camera_set,
            std::shared_ptr<GrowingDescriptorPool::ManagedDescriptorSet>&
            light_set,
            float camera_aspect = 1.0
        ): _camera_descriptor_set(camera_set), _camera_aspect(camera_aspect), _light_source_descriptor_set(light_set) {
            _camera_buffer = RawBuffer::create(
                sizeof(CameraTransform),
                true
            );

            _scene_light_info = RawBuffer::create(
                sizeof(LightInfo),
                true
            );

            _light_source_ptr_buffer = RawBuffer::create(
                sizeof(std::uint64_t) * 16,
                true
            );

            auto l_it = RangedIterator(
                std::views::single(
                    std::tuple<std::string&, Resource*>{
                        LP_LIGHT_INFO_NAME,
                        _scene_light_info.get()
                    }
                )
            );

            _light_source_descriptor_set->get_set()->write_bindings(
                l_it
            );

            auto* p_sli = reinterpret_cast<LightInfo*>(_scene_light_info->map());
            *p_sli = {};
            p_sli->buffer_address = _light_source_ptr_buffer->get_device_address();
            _scene_light_info->unmap();

            auto it = RangedIterator(
                std::views::single(
                    std::tuple<
                        std::string&,
                        Resource*
                    >{
                        GP_CAMERA_MATRIX_NAME,
                        _camera_buffer.get()
                    }
                )
            );

            _camera_descriptor_set->get_set()->write_bindings(
                it
            );

            update_camera_state();
        }

        auto get_render_objects(std::size_t render_target_width, std::size_t render_target_height) {
            update_camera_state(render_target_width, render_target_height);

            auto range_of_render_objects = _render_objects |
                std::views::transform([](auto& rObject) -> RenderObject& {
                    return rObject.second._object;
                });

            return RangedIterator(range_of_render_objects);
        }

        std::shared_ptr<GrowingDescriptorPool::ManagedDescriptorSet>
        get_light_descriptor_set() {
            update_light_camera();
            update_light_source_ptr();
            return _light_source_descriptor_set;
        }

        void set_scenes_ambient_color(
            const glm::vec3& ambient_color
        ) {
            auto* p_sli = reinterpret_cast<LightInfo*>(_scene_light_info->map());
            p_sli->ambient_color = ambient_color;
            _scene_light_info->unmap();
        }

        void add_light_source(
            const std::string& name,
            LightSourceType type
        );

        void remove_light_source(
            const std::string& name
        );

        void set_light_source_transform(
            const std::string& name,
            const glm::vec3& position,
            const glm::vec3& direction
        );

        void set_light_source_color(
            const std::string& name,
            const glm::vec3& color
        );

        void add_render_object(
            const std::string& name,
            const std::string& model_name,
            std::shared_ptr<GrowingDescriptorPool::ManagedDescriptorSet>& my_descriptor_set
        );

        void remove_render_object(
            const std::string& name
        );

        void set_render_objects_transform(
            const std::string& name,
            const glm::mat4x4& transform
        );

        void set_render_objects_color(
            const std::string& name,
            const glm::vec3& color
        );

        void set_render_objects_coloring_type(
            const std::string& name,
            const ColoringType& coloring_type
        );

        void set_resolution_source(
            SceneResolutionSource resolution_source
        );

        void set_fov(float fov) {
            _camera_fov = fov;
            _camera_updated = true;
        }

        void set_aspect(float aspect) {
            _camera_aspect = aspect;
            _camera_updated = true;
        }

        void set_object_texture(
            const std::string& object_name,
            const std::shared_ptr<Image>& texture
        );

        void set_camera_eye(
            glm::vec3 camera_eye
        ) {
            _camera_direction = camera_eye;
            _camera_updated = true;
        }

        void set_aspect(std::uint32_t width, std::uint32_t height, bool forced = false) {
            if (!(this->_resolution_source == SceneResolutionSource::MANUAL || forced)) {
                return;
            }

            _camera_aspect = static_cast<float>(width) / height;
            _camera_updated = true;
        }
    };
}