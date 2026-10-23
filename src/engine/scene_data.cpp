//
// Created by Роман  Тимофеев on 25.08.2026.
//
module;

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/matrix.hpp>

#include "glm/ext/matrix_transform.hpp"

#ifdef __APPLE__
#include <ranges>
#include <memory>
#endif

module SceneData;
import render_engine;
import resource_lib;

using namespace RenderEngine;

void SceneData::update_light_camera() {
    if (!_light_affecting_camera_update) {
        return;
    }

    _light_affecting_camera_update = false;

    auto* light_info = reinterpret_cast<LightInfo*>(
        _scene_light_info->map()
    );

    light_info->camera_position = this->_camera_position;

    _scene_light_info->unmap();
}

void SceneData::update_light_source_ptr() {

    if (!_is_light_sources_changed) {
        return;
    }

    if (_light_source_ptr_buffer->size() / sizeof(std::uint64_t) < _light_sources.size()) {
        _light_source_ptr_buffer =
            RawBuffer::create(
                sizeof(std::uint64_t) * (_light_sources.size() + 16),
                true
            );

        auto* light_info = reinterpret_cast<LightInfo*>(
            _scene_light_info->map()
        );
        light_info->buffer_address = _light_source_ptr_buffer->get_device_address();
        _scene_light_info->unmap();
    }

    auto* b_ptr = reinterpret_cast<std::uint64_t*>(_light_source_ptr_buffer->map());
    std::uint64_t i = 0;

    for (auto& [_, light_source_b] : _light_sources) {
        b_ptr[i++] = light_source_b->get_device_address();
    }

    _light_source_ptr_buffer->unmap();

    auto* light_info = reinterpret_cast<LightInfo*>(
        _scene_light_info->map()
    );
    light_info->light_source_count = _light_sources.size();
    _scene_light_info->unmap();
}

void SceneData::update_camera_state(
    std::size_t render_target_width,
    std::size_t render_target_height
) {

    if (_resolution_source == SceneResolutionSource::RENDER_TARGET) {
        set_aspect(render_target_width, render_target_height);
    }

    if (!_camera_updated) {
        return;
    }

    _camera_updated = false;

    glm::mat4x4 camera_projection = glm::perspective(
        _camera_fov,
        _camera_aspect,
        _camera_range.first,
        _camera_range.second
    );

    glm::vec3 act_up = _camera_up;
    act_up.y = -act_up.y;

    glm::mat4x4 camera_view = glm::lookAt(
        _camera_position,
        _camera_direction,
       act_up
    );

    auto *p_camera_buffer =
            reinterpret_cast<CameraTransform *>(this->_camera_buffer->map());

    p_camera_buffer->projection_matrix = camera_projection;
    p_camera_buffer->view_matrix = camera_view;

    this->_camera_buffer->unmap();
}

void SceneData::remove_render_object(
    const std::string &name
) {
    if (!_render_objects.contains(name)) {
        return;
    }

    wait_device_free();

    _render_objects.erase(name);
}

void SceneData::set_render_objects_transform(const std::string &name, const glm::mat4x4 &transform) {
    if (!_render_objects.contains(name)) {
        return;
    }

    auto &r_object = _render_objects.at(name);

    auto *transform_mat = reinterpret_cast<WorldTransform*>(r_object._position_buffers->map());

    transform_mat->position_matrix = transform;
    transform_mat->normal_matrix = glm::transpose(glm::inverse(transform));

    r_object._position_buffers->unmap();
}

void SceneData::set_render_objects_color(const std::string &name, const glm::vec3 &color) {
    if (!_render_objects.contains(name)) {
        return;
    }

    auto &r_object = _render_objects.at(name);
    auto* mtd = reinterpret_cast<ObjectMetadata*>(r_object._meta_data_buffer->map());
    mtd->color = glm::vec4(color, 1.0);
    r_object._meta_data_buffer->unmap();
}

void SceneData::set_render_objects_coloring_type(const std::string &name, const ColoringType &coloring_type) {
    if (!_render_objects.contains(name)) {
        return;
    }

    auto &r_object = _render_objects.at(name);
    auto* mtd = reinterpret_cast<ObjectMetadata*>(r_object._meta_data_buffer->map());
    mtd->c_type = coloring_type;
    r_object._meta_data_buffer->unmap();
}

void SceneData::add_light_source(
    const std::string &name,
    LightSourceType type
) {
    if (_light_sources.contains(name)) {
        return;
    }

    auto new_buffer = RawBuffer::create(
        sizeof(LightSource),
        true
    );

    auto* ls = reinterpret_cast<LightSource*>(new_buffer->map());
    *ls = {};
    ls->type = type;
    new_buffer->unmap();
    _is_light_sources_changed = true;

    _light_sources.insert({name, new_buffer});
}

void SceneData::remove_light_source(const std::string &name) {
    if (!_light_sources.contains(name)) {
        return;
    }

    _is_light_sources_changed = true;

    _light_sources.erase(name);
}

void SceneData::set_light_source_transform(
    const std::string &name,
    const glm::vec3 &position,
    const glm::vec3 &direction) {

    if (!_light_sources.contains(name)) {
        return;
    }

    auto* l_source = reinterpret_cast<LightSource*>(
        _light_sources.at(name)->map()
    );

    l_source->position = glm::vec4(position, 0.0f);
    l_source->direction = glm::vec4(direction, 0.0);

    _light_sources.at(name)->unmap();
}

void SceneData::set_light_source_color(
    const std::string &name,
    const glm::vec3 &color) {

    if (!_light_sources.contains(name)) {
        return;
    }

    auto* l_source = reinterpret_cast<LightSource*>(
        _light_sources.at(name)->map()
    );

    l_source->light_color = glm::vec4(color, 0.0);

    _light_sources.at(name)->unmap();
}

void SceneData::add_render_object(
    const std::string &name,
    const std::string &model_name,
    std::shared_ptr<GrowingDescriptorPool::ManagedDescriptorSet> &my_descriptor_set
) {
    if (!ResourceLib::is_mesh_exists(model_name) || _render_objects.contains(name)) {
        return;
    }

    auto position_buffer = RawBuffer::create(
        sizeof(WorldTransform),
        true
    );

    auto meta_data_buffer = RawBuffer::create(
        sizeof(ObjectMetadata),
        true,
        true
    );

    auto model_buffer = ResourceLib::get_loaded_mesh(
        model_name
    );

    auto *p_model_matrix = reinterpret_cast<WorldTransform *>(position_buffer->map());

    *p_model_matrix = {};

    position_buffer->unmap();

    auto* p_meta_data = reinterpret_cast<ObjectMetadata*>(meta_data_buffer->map());
    *p_meta_data = {};
    meta_data_buffer->unmap();

    _render_objects.insert({name, {}});
    auto &new_object = _render_objects.at(name);

    new_object._object.vertex_buffer = model_buffer;
    new_object._position_buffers = position_buffer;
    new_object._position_descriptor_set = my_descriptor_set;
    new_object._meta_data_buffer = meta_data_buffer;
    new_object._sets = {
        _camera_descriptor_set->get_set(),
        new_object._position_descriptor_set->get_set()
    };
    new_object._object.sets = new_object._sets;

    auto it = RangedIterator(
        std::array<std::tuple<std::string &, Resource *>, 2>
        {
            std::tuple<std::string &, Resource *>{
                GP_POSITION_MATRIX_NAME,
                new_object._position_buffers.get()
            },
            {
                GP_METADATA_NAME,
                new_object._meta_data_buffer.get()
            }
        }
    );

    new_object._position_descriptor_set->get_set()->write_bindings(
        it
    );
}

void SceneData::set_resolution_source(SceneResolutionSource resolution_source) {
    _resolution_source = resolution_source;
}

void SceneData::set_object_texture(const std::string &object_name, const std::shared_ptr<Image> &texture_name) {
    if (!_render_objects.contains(object_name)) {
        return;
    }

    auto &r_object = _render_objects.at(object_name);

    auto it = RangedIterator(
        std::views::single(
            std::tuple<std::string&, Resource *>{
                GP_TEXTURE_NAME,
                texture_name.get()
            }
        )
    );

    r_object._position_descriptor_set->get_set()->write_bindings(
        it
    );
}
