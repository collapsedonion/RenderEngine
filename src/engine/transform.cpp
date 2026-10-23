//
// Created by Роман  Тимофеев on 18.08.2026.
//
module;

#include <glm_includes.h>

#ifdef __APPLE__
#include <cinttypes>
#include <memory>
#endif

module Transform;

using namespace RenderEngine;

Transform::~Transform() {
    if (auto parent_transform = _parent_transform.lock()) {
        parent_transform->_children.erase(this);
    }
}

void Transform::update_matrix(bool force_recalculate) {
    if (!_recalculate_matrix) {
        return;
    }

    if (!(force_recalculate || _auto_recalculate_matrix)) {
        return;
    }

    for (auto child: _children) {
        child->_recalculate_matrix = true;
    }

    _matrix =
        glm::translate(glm::mat4x4(1), _position) *
        glm::toMat4(_rotation) *
        glm::scale(glm::mat4x4(1), _scale);

    if (auto parent_transform = _parent_transform.lock()) {
        _matrix = parent_transform->get_matrix() * _matrix;
    }

    _recalculate_matrix = false;
}

void Transform::update_parent(std::shared_ptr<Transform> &new_parent) {

    if (auto old_parent = _parent_transform.lock()) {
        old_parent->_children.erase(this);
    }

    this->_parent_transform = new_parent;

    if (auto parent_transform = _parent_transform.lock()) {
        parent_transform->_children.insert(this);
    }
}

