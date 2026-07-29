//
//
// Created by Onion on 28.07.2026.
//
module;
#include <render_engine.h>

module scene3d;

using namespace RenderEngine;

void SimpleColored3DItem::set_parent(std::shared_ptr<SimpleColored3DItem> father)
{
    if (auto old_father = _father_object.lock())
    {
        old_father->_child_objects.erase(this);
    }

    this->_father_object = father;
    father->_child_objects.insert(this);
}

const std::string& SimpleColored3DItem::get_name()
{
    return this->_name;
}

void SimpleColored3DItem::set_name(std::string_view name)
{
    this->_name = name;
    this->_name_hash = std::hash<std::string_view>{}(name);
}

void SimpleColored3DItem::free(bool free_descriptor)
{
    if (!render_ready)
    {
        return;
    }

    if (free_descriptor)
    {
        re_free_descriptor_sets(_descriptor_sets.size(), _descriptor_sets.data());
    }

    re_free_buffer(_transform_buffer);
    re_free_buffer(_color_buffer);
}

void SimpleColored3DItem::set_camera_data_buffer(RE_pBuffer buffer)
{
    _camera_matrix_data = buffer;
}

Mat4x4 SimpleColored3DItem::get_matrix()
{
    Mat4x4 my_matrix = TransformedItem::get_matrix();

    if (auto father = _father_object.lock())
    {
        my_matrix = father->get_matrix() * my_matrix;
    }

    return my_matrix;
}

void SimpleColored3DItem::update_transform_buffer()
{
    auto* t_data = reinterpret_cast<TransformData*>(re_map_buffer(_transform_buffer));

    for (auto& child: this->_child_objects)
    {
        child->update_transform_buffer();
    }

    t_data->transform_matrix = this->get_matrix();
    re_unmap_buffer(_transform_buffer);
}

void SimpleColored3DItem::update_color_buffer()
{
    auto* color_data = reinterpret_cast<Color*>(re_map_buffer(_color_buffer));
    *color_data = color();
    re_unmap_buffer(_color_buffer);
}

// Rectangle3D

Color Rectangle3D::get_color() { return _c; }

void Rectangle3D::set_color(const Color& color)
{
    _c = color;
    if (render_ready)
    {
        update_color_buffer();
    }
}

const Mat4x4& Rectangle3D::get_position_matrix() { return _position_matrix; }
const Mat4x4& Rectangle3D::get_rotation_matrix() { return _rotation_matrix; }
const Mat4x4& Rectangle3D::get_scale_matrix() { return _scale_matrix; }

void Rectangle3D::set_position_matrix(const Mat4x4& m)
{
    _position_matrix = m;
    if (render_ready)
    {
        update_transform_buffer();
    }
}

void Rectangle3D::set_rotation_matrix(const Mat4x4& m)
{
    _rotation_matrix = m;
    if (render_ready)
    {
        update_transform_buffer();
    }
}

void Rectangle3D::set_scale_matrix(const Mat4x4& m)
{
    _scale_matrix = m;
    if (render_ready)
    {
        update_transform_buffer();
    }
}

RE_pBuffer Rectangle3D::get_vertex_buffer(const VertexBuffers& vb)
{
    return vb.rectangle;
}

// Triangle3D

Color Triangle3D::get_color() { return _c; }

void Triangle3D::set_color(const Color& color)
{
    _c = color;
    if (render_ready)
    {
        update_color_buffer();
    }
}

const Mat4x4& Triangle3D::get_position_matrix() { return _position_matrix; }
const Mat4x4& Triangle3D::get_rotation_matrix() { return _rotation_matrix; }
const Mat4x4& Triangle3D::get_scale_matrix() { return _scale_matrix; }

void Triangle3D::set_position_matrix(const Mat4x4& m)
{
    _position_matrix = m;
    if (render_ready)
    {
        update_transform_buffer();
    }
}

void Triangle3D::set_rotation_matrix(const Mat4x4& m)
{
    _rotation_matrix = m;
    if (render_ready)
    {
        update_transform_buffer();
    }
}

void Triangle3D::set_scale_matrix(const Mat4x4& m)
{
    _scale_matrix = m;
    if (render_ready)
    {
        update_transform_buffer();
    }
}

RE_pBuffer Triangle3D::get_vertex_buffer(const VertexBuffers& vb)
{
    return vb.triangle;
}

// Circle3D

Color Circle3D::get_color() { return _c; }

void Circle3D::set_color(const Color& color)
{
    _c = color;
    if (render_ready)
    {
        update_color_buffer();
    }
}

const Mat4x4& Circle3D::get_position_matrix() { return _position_matrix; }
const Mat4x4& Circle3D::get_rotation_matrix() { return _rotation_matrix; }
const Mat4x4& Circle3D::get_scale_matrix() { return _scale_matrix; }

void Circle3D::set_position_matrix(const Mat4x4& m)
{
    _position_matrix = m;
    if (render_ready)
    {
        update_transform_buffer();
    }
}

void Circle3D::set_rotation_matrix(const Mat4x4& m)
{
    _rotation_matrix = m;
    if (render_ready)
    {
        update_transform_buffer();
    }
}

void Circle3D::set_scale_matrix(const Mat4x4& m)
{
    _scale_matrix = m;
    if (render_ready)
    {
        update_transform_buffer();
    }
}

RE_pBuffer Circle3D::get_vertex_buffer(const VertexBuffers& vb)
{
    return vb.circle;
}

Scene3D::Scene3D()
{
    this->camera_data = re_create_buffer(
        sizeof(CameraData),
        true,
       false
    );
}

Scene3D::~Scene3D()
{
    re_free_descriptor_pool(descriptor_pool);

    for (auto&& item : items)
    {
        item->free(false);
        item.reset();
    }

    re_free_buffer(this->camera_data);

    if (this->_resolution_set)
    {
        re_free_image(this->_render_image);
        re_free_image(this->_depth_image);
    }
}

void Scene3D::init_resolution(std::pair<uint32_t, uint32_t> resolution)
{
    if (this->_resolution_set)
    {
        return;
    }

    auto [width, height] = resolution;
    this->_render_image = re_create_image(
        width,
        height,
        RE_IMAGE_FORMAT_RGBA8,
        false,
        false,
        false
    );

    this->_depth_image = re_create_image(
        width,
        height,
        RE_IMAGE_FORMAT_DEPTH,
        false,
        false,
        false
    );
}

std::weak_ptr<SimpleColored3DItem> Scene3D::get_item_by_name(std::string_view name) const
{
    std::size_t hash = std::hash<std::string_view>{}(name);

    auto f_it = std::ranges::find_if(this->items, [=](auto& item)
    {
        return item->_name_hash == hash;
    });

    return *f_it;
}

void Scene3D::finalise(RE_pShaderModule module, const VertexBuffers& vertex_buffers)
{
    descriptor_pool = re_create_descriptor_pool(
        module, items.size() * 2
    );
    render_objects.reserve(items.size());

    for (auto& item : items)
    {
        render_objects.push_back(item->get_render_object(vertex_buffers, descriptor_pool));
    }
}

void Scene3D::render(RE_pShaderModule module, RE_pImage target_image)
{
    if (this->_resolution_set)
    {
        return;
    }
    uint32_t width, height;

    re_get_image_dimensions(target_image, &width, &height);

    auto* camera_data = reinterpret_cast<CameraData*>(re_map_buffer(this->camera_data));
    camera_data->projection = Mat4x4::perspective(
        40.0f,
        width,
        height,
        0.1f,
        100.0f
    );
    re_unmap_buffer(this->camera_data);

    re_render(
        TRANSFORMED_COLOR_NAME.c_str(),
        module,
        1,
        &this->_render_image,
        this->render_objects.size(),
        this->render_objects.data(),
        &this->_depth_image,
        false
    );

    RE_ImageToImageTransfer transfer = {
        .from_image = this->_render_image,
        .to_image = target_image
    };

    re_transfer_image_to_image(
        &transfer,
        1
    );
}
