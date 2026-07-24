//
// Created by Onion on 22.07.2026.
//
module;
#include <cstdint>
#include <render_engine.h>

export module scene2d;
export import simple_draw;
export import setters;
import std;

namespace RenderEngine
{
    export class Scene2D;

    export class SimpleColored2DItem :
        public SceneItem,
        public ColoredItem,
        public Positioned2DItem
    {
        static constexpr auto POOL_INDICIES = std::array<uint32_t, 2>{
            0,
            1
        };

        RE_pBuffer _pos_buffer = {};
        RE_pBuffer _color_buffer = {};
        RE_pBuffer _image_data_buffer = {};

        std::array<RE_pDescriptorSet, 2> _descriptor_sets = {};

    protected:
        bool render_ready = false;

        void free(bool free_descriptor);

        void set_image_data_buffer(RE_pBuffer buffer);

        virtual RE_pBuffer get_vertex_buffer(const VertexBuffers& vb) = 0;

        void update_position_buffer();

        void update_color_buffer();

        RE_RenderObject get_render_object(const VertexBuffers& vb, RE_pDescriptorPool pool) override
        {
            _pos_buffer = re_create_buffer(sizeof(VertexData2D), true, false);
            _color_buffer = re_create_buffer(sizeof(Color), true, false);

            re_create_descriptor_sets(
                pool,
                2,
                const_cast<uint32_t*>(POOL_INDICIES.data()),
                _descriptor_sets.data()
            );

            update_color_buffer();
            update_position_buffer();

            auto buffers = std::array<RE_pBuffer, 2>{
                _pos_buffer,
                _image_data_buffer
            };

            auto names = std::array<const char*, 2>{
                POSITION_DESCRIPTOR_SET,
                IMAGE_DESCRIPTOR_SET
            };

            re_write_set_buffers(
                _descriptor_sets[0],
                2,
                names.data(),
                buffers.data(),
                nullptr,
                nullptr
            );

            re_write_set_buffers(
                _descriptor_sets[1],
                1,
                &COLOR_DESCRIPTOR_SET,
                &_color_buffer,
                nullptr,
                nullptr
            );

            auto newObject = RE_RenderObject{
                .vertex_buffer = get_vertex_buffer(vb),
                .descriptor_set_count = 2,
                .descriptor_sets = _descriptor_sets.data(),
            };

            render_ready = true;
            return newObject;
        }

        friend Scene2D;
    };

    export class Rectangle2D :
        public SimpleColored2DItem,
        public ColorSetter<Rectangle2D>,
        public PositionSetter<Rectangle2D>
    {
    public:
        using ColorSetter::color;
        using ColoredItem::color;
        using Positioned2DItem::position;
        using Positioned2DItem::size;
        using Positioned2DItem::angle;
        using PositionSetter::position;
        using PositionSetter::size;
        using PositionSetter::angle;

    private:
        Color _c = {1, 1, 1};
        Float2 _p = {0, 0};
        Float2 _size = {1, 1};
        float _angle = 0.0f;

    public:
        Color get_color() override;
        void set_color(const Color& color) override;
        Float2 get_position() override;
        void set_position(const Float2& position) override;
        float get_angle() override;
        void set_angle(float angle) override;
        Float2 get_size() override;
        void set_size(const Float2& size) override;

    private:
        Rectangle2D() = default;

    protected:
        RE_pBuffer get_vertex_buffer(const VertexBuffers& vb) override;

        friend Scene2D;
    };

    export class Triangle2D :
        public SimpleColored2DItem,
        public ColorSetter<Triangle2D>,
        public PositionSetter<Triangle2D>
    {
    public:
        using ColorSetter::color;
        using ColoredItem::color;
        using Positioned2DItem::position;
        using Positioned2DItem::size;
        using Positioned2DItem::angle;
        using PositionSetter::position;
        using PositionSetter::size;
        using PositionSetter::angle;

    private:
        Color _c = {1, 1, 1};
        Float2 _p = {0, 0};
        Float2 _size = {1, 1};
        float _angle = 0.0f;

    public:
        Color get_color() override;
        void set_color(const Color& color) override;
        Float2 get_position() override;
        void set_position(const Float2& position) override;
        float get_angle() override;
        void set_angle(float angle) override;
        Float2 get_size() override;
        void set_size(const Float2& size) override;

    private:
        Triangle2D() = default;

    protected:
        RE_pBuffer get_vertex_buffer(const VertexBuffers& vb) override;
        friend Scene2D;
    };

    export class Circle2D :
        public SimpleColored2DItem,
        public ColorSetter<Circle2D>,
        public PositionSetter<Circle2D>
    {
    public:
        using ColorSetter::color;
        using ColoredItem::color;
        using Positioned2DItem::position;
        using Positioned2DItem::size;
        using Positioned2DItem::angle;
        using PositionSetter::position;
        using PositionSetter::size;
        using PositionSetter::angle;

    private:
        Color _c = {1, 1, 1};
        Float2 _p = {0, 0};
        Float2 _size = {1, 1};
        float _angle = 0.0f;

    public:
        Color get_color() override;
        void set_color(const Color& color) override;
        Float2 get_position() override;
        void set_position(const Float2& position) override;
        float get_angle() override;
        void set_angle(float angle) override;
        Float2 get_size() override;
        void set_size(const Float2& size) override;

    private:
        Circle2D() = default;

    protected:
        RE_pBuffer get_vertex_buffer(const VertexBuffers& vb) override;
        friend Scene2D;
    };

    class Scene2D : public Scene
    {
        std::vector<Rectangle2D> rectangles = {};
        std::vector<Triangle2D> triangles = {};
        std::vector<Circle2D> circles = {};

        std::vector<RE_RenderObject> render_objects = {};
        RE_pDescriptorPool descriptor_pool = {};
        RE_pBuffer image_data = {};

        // Returns auto — must stay in header
        auto get_all_items()
        {
            auto rects = rectangles | std::views::transform([](Rectangle2D& rect)
            {
                return dynamic_cast<SceneItem*>(&rect);
            });

            auto triang = triangles | std::views::transform([](auto& triangle)
            {
                return dynamic_cast<SceneItem*>(&triangle);
            });

            auto circ = circles | std::views::transform([](auto& circ)
            {
                return dynamic_cast<SceneItem*>(&circ);
            });

            return std::views::concat(rects, triang, circ);
        }

    public:
        Scene2D();
        ~Scene2D() override;

        Rectangle2D& rect();
        Triangle2D& triangle();
        Circle2D& circle();

    protected:
        void finalise(RE_pShaderModule module, const VertexBuffers& vertex_buffers) override;
        void render(RE_pShaderModule module, RE_pImage target_image) override;

        friend SimpleDrawer;
    };
}
