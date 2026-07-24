//
// Created by Onion on 22.07.2026.
//
module;
#include <render_engine.h>

module scene2d;

using namespace RenderEngine;

void SimpleColored2DItem::free(bool free_descriptor)
{
    if (!render_ready)
    {
        return;
    }

    if (free_descriptor)
    {
        re_free_descriptor_sets(_descriptor_sets.size(), _descriptor_sets.data());
    }

    re_free_buffer(_pos_buffer);
    re_free_buffer(_color_buffer);
}

void SimpleColored2DItem::set_image_data_buffer(RE_pBuffer buffer)
{
    _image_data_buffer = buffer;
}

void SimpleColored2DItem::update_position_buffer()
{
    auto* v_data = reinterpret_cast<VertexData2D*>(re_map_buffer(_pos_buffer));
    v_data->position = position();
    v_data->scale = size();
    v_data->angle = angle();
    re_unmap_buffer(_pos_buffer);
}

void SimpleColored2DItem::update_color_buffer()
{
    auto* color_data = reinterpret_cast<Color*>(re_map_buffer(_color_buffer));
    *color_data = color();
    re_unmap_buffer(_color_buffer);
}

// Rectangle2D

Color Rectangle2D::get_color() { return _c; }
void Rectangle2D::set_color(const Color& color) { _c = color; }
Float2 Rectangle2D::get_position() { return _p; }
void Rectangle2D::set_position(const Float2& position) { _p = position; }
float Rectangle2D::get_angle() { return _angle; }
void Rectangle2D::set_angle(float angle) { _angle = angle; }
Float2 Rectangle2D::get_size() { return _size; }
void Rectangle2D::set_size(const Float2& size) { _size = size; }

RE_pBuffer Rectangle2D::get_vertex_buffer(const VertexBuffers& vb)
{
    return vb.rectangle;
}

// Triangle2D

Color Triangle2D::get_color() { return _c; }
void Triangle2D::set_color(const Color& color) { _c = color; }
Float2 Triangle2D::get_position() { return _p; }
void Triangle2D::set_position(const Float2& position) { _p = position; }
float Triangle2D::get_angle() { return _angle; }
void Triangle2D::set_angle(float angle) { _angle = angle; }
Float2 Triangle2D::get_size() { return _size; }
void Triangle2D::set_size(const Float2& size) { _size = size; }

RE_pBuffer Triangle2D::get_vertex_buffer(const VertexBuffers& vb)
{
    return vb.triangle;
}

// Circle2D

Color Circle2D::get_color() { return _c; }
void Circle2D::set_color(const Color& color) { _c = color; }
Float2 Circle2D::get_position() { return _p; }
void Circle2D::set_position(const Float2& position) { _p = position; }
float Circle2D::get_angle() { return _angle; }
void Circle2D::set_angle(float angle) { _angle = angle; }
Float2 Circle2D::get_size() { return _size; }
void Circle2D::set_size(const Float2& size) { _size = size; }

RE_pBuffer Circle2D::get_vertex_buffer(const VertexBuffers& vb)
{
    return vb.circle;
}

Scene2D::Scene2D()
{
    this->image_data = re_create_buffer(
        sizeof(ImageData),
        true,
        false
    );
}

Scene2D::~Scene2D()
{
    re_free_descriptor_pool(descriptor_pool);

    for (auto& rec : rectangles)
    {
        rec.free(false);
    }

    for (auto& triangle : triangles)
    {
        triangle.free(false);
    }

    for (auto& circle : circles)
    {
        circle.free(false);
    }

    re_free_buffer(image_data);
}

Rectangle2D& Scene2D::rect()
{
    rectangles.push_back({});
    rectangles.back().set_image_data_buffer(this->image_data);
    return rectangles.back();
}

Triangle2D& Scene2D::triangle()
{
    triangles.push_back({});
    triangles.back().set_image_data_buffer(this->image_data);
    return triangles.back();
}

Circle2D& Scene2D::circle()
{
    circles.push_back({});
    circles.back().set_image_data_buffer(this->image_data);
    return circles.back();
}

void Scene2D::finalise(RE_pShaderModule module, const VertexBuffers& vertex_buffers)
{
    descriptor_pool = re_create_descriptor_pool(
        module, (rectangles.size() + triangles.size() + circles.size()) * 2
    );
    render_objects.reserve(rectangles.size() + triangles.size() + circles.size());

    for (auto* item : get_all_items())
    {
        render_objects.push_back(item->get_render_object(vertex_buffers, descriptor_pool));
    }
}

void Scene2D::render(RE_pShaderModule module, RE_pImage target_image)
{
    auto* image_data = reinterpret_cast<ImageData*>(re_map_buffer(this->image_data));
    re_get_image_dimensions(target_image, &image_data->width, &image_data->height);
    re_unmap_buffer(this->image_data);

    re_render(
        SIMPLE_COLOR_NAME.c_str(),
        module,
        1,
        &target_image,
        render_objects.size(),
        render_objects.data(),
        nullptr,
        false
    );
}
