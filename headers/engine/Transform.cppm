//
// Created by Роман  Тимофеев on 18.08.2026.
//
module;

#include <glm_includes.h>

#ifdef __APPLE__
#include <memory>
#include <unordered_set>
#endif

export module Transform;

namespace RenderEngine {

    export class Transform {

        glm::f32vec3 _position = {0,0,0};
        glm::f32vec3 _scale = {1,1,1};
        glm::f32quat _rotation = {1, 0, 0, 0};

        glm::mat4x4 _matrix = glm::mat4x4(1);

        bool _auto_recalculate_matrix = false;
        bool _recalculate_matrix = false;

        std::weak_ptr<Transform> _parent_transform = {};
        std::unordered_set<Transform*> _children = {};

    private:
        Transform() = default;

    private:

        void update_matrix(bool force_recalculate);
        void update_parent(std::shared_ptr<Transform>& new_parent);

    public:
        ~Transform();

        static std::shared_ptr<Transform> create() {
            return std::shared_ptr<Transform>(new Transform());
        }

        static std::shared_ptr<Transform> create(std::shared_ptr<Transform>& parent) {
            auto new_transform = std::shared_ptr<Transform>(new Transform());
            new_transform->set_parent(parent);
            return std::move(new_transform);
        }

        glm::mat4x4 get_matrix() {
            update_matrix(true);
            return _matrix;
        }

        void set_parent(std::shared_ptr<Transform>& new_parent) {
            update_parent(new_parent);
        }

        void set_auto_recalculate(bool auto_recalculate) {
            _auto_recalculate_matrix = auto_recalculate;
        }

        void set_position(const glm::f32vec3& position) {
            _position = position;
            _recalculate_matrix = true;
            update_matrix(false);
        }

        void set_scale(const glm::f32vec3& scale) {
            _scale = scale;
            _recalculate_matrix = true;
            update_matrix(false);
        }

        void set_rotation(const glm::f32quat& rotation) {
            _rotation = rotation;
            _recalculate_matrix = true;
            update_matrix(false);
        }

        [[nodiscard]] glm::f32vec3 get_position() const {
            return _position;
        }

        [[nodiscard]] glm::f32quat get_rotation() const {
            return _rotation;
        }

        [[nodiscard]] glm::f32vec3 get_scale() const {
            return _scale;
        }
    };
}