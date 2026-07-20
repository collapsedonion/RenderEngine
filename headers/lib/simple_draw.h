//
// Created by Onion on 17.07.2026.
//

#ifndef RENDERENGINE_SIMPLE_DRAW_H
#define RENDERENGINE_SIMPLE_DRAW_H
#include <algorithm>
#include <render_engine.h>
#include <string>
#include <functional>
#include <array>
#include <ranges>

#include "simple_draw.h"
#include "spirv_tools.h"

namespace RenderEngine
{
    inline const std::string DIRECT_2D_NAME = "direct_vertex_shader_2D";
    inline const std::string COLORED_NAME = "colored_fragment_shader";

    inline const char* POSITION_DESCRIPTOR_SET = "data2DV";
    inline const char* COLOR_DESCRIPTOR_SET = "dataColor";
    inline const char* IMAGE_DESCRIPTOR_SET = "imageData";

    inline const std::string SIMPLE_COLOR_NAME = "2D_colored";

    class Scene2D;
    class SimpleDrawer;

    struct Float2
    {
        float x;
        float y;

        constexpr Float2(float v) : x(v), y(v)
        {
        }

        constexpr Float2(float x, float y) : x(x), y(y)
        {
        }
    };

    struct Float3
    {
        float x;
        float y;
        float z;

        constexpr Float3(float v) : x(v), y(v), z(v)
        {
        }

        constexpr Float3(float x, float y, float z) : x(x), y(y), z(z)
        {
        }
    };

    using Color = Float3;

    struct VertexData2D
    {
        Float2 position;
        Float2 scale;
        float angle;
    };

    struct ImageData
    {
        uint32_t width;
        uint32_t height;
    };

    struct VertexBuffers
    {
        RE_pBuffer rectangle;
        RE_pBuffer triangle;
    };

    class SceneItem
    {
        virtual RE_RenderObject get_render_object(const VertexBuffers& buffers, RE_pDescriptorPool pool) = 0;

    public:
        virtual ~SceneItem() = default;
        friend Scene2D;
    };

    template <class RT>
    class ColoredItem
    {
    protected:
        virtual Color get_color() = 0;
        virtual void set_color(const Color& color) = 0;

        friend SimpleDrawer;

    public:
        virtual ~ColoredItem() = default;
        virtual Color color() final { return get_color(); };

        virtual RT& color(const Color& color) final
        {
            set_color(color);
            return dynamic_cast<RT&>(*this);
        }
    };

    template <class RT>
    class Positioned2DItem
    {
        virtual Float2 get_position() = 0;
        virtual void set_position(const Float2& position) = 0;

        virtual float get_angle() = 0;
        virtual void set_angle(float angle) = 0;

        virtual Float2 get_size() = 0;
        virtual void set_size(const Float2& size) = 0;

        friend SimpleDrawer;

    public:
        virtual ~Positioned2DItem() = default;
        virtual Float2 position() final { return get_position(); }

        virtual RT& position(const Float2& position) final
        {
            set_position(position);
            return dynamic_cast<RT&>(*this);
        }

        virtual float angle() final { return get_angle(); }

        virtual RT& angle(float angle) final
        {
            set_angle(angle);
            return dynamic_cast<RT&>(*this);
        }

        virtual Float2 size() final { return get_size(); }

        virtual RT& size(const Float2& size) final
        {
            set_size(size);
            return dynamic_cast<RT&>(*this);
        }
    };

    template <class T>
    class SimpleColored2DItem :
        public SceneItem
    {
        static constexpr auto _POOL_INDICIES = std::array<uint32_t, 2>{
            0,
            1
        };

        RE_pBuffer _pos_buffer = {};
        RE_pBuffer _color_buffer = {};

        std::array<RE_pDescriptorSet, 2> _descriptor_sets = {};

    protected:
        bool render_ready = false;

        virtual void free(bool free_descriptor) final
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

        virtual RE_pBuffer get_vertex_buffer(const VertexBuffers& vb) = 0;

        virtual void update_position_buffer() final
        {
            static_assert(requires(T t)
            {
                { t.color() } -> std::same_as<Color>;
            });
            auto* v_data = reinterpret_cast<VertexData2D*>(re_map_buffer(_pos_buffer));
            v_data->position = dynamic_cast<T*>(this)->position();
            v_data->scale = dynamic_cast<T*>(this)->size();
            v_data->angle = dynamic_cast<T*>(this)->angle();
            re_unmap_buffer(_pos_buffer);
        }

        virtual void update_color_buffer() final
        {
            static_assert(requires(T t, VertexData2D vd)
            {
                { t.position() } -> std::same_as<Float2>;
                { t.size() } -> std::same_as<Float2>;
                { t.angle() } -> std::same_as<float>;
            });
            auto* color_data = reinterpret_cast<Color*>(re_map_buffer(_color_buffer));
            *color_data = dynamic_cast<T*>(this)->color();
            re_unmap_buffer(_color_buffer);
        }

        RE_RenderObject get_render_object(const VertexBuffers& vb, RE_pDescriptorPool pool) override
        {
            _pos_buffer = re_create_buffer(sizeof(VertexData2D), true, false);
            _color_buffer = re_create_buffer(sizeof(Color), true, false);

            re_create_descriptor_sets(
                pool,
                2,
                const_cast<uint32_t*>(_POOL_INDICIES.data()),
                _descriptor_sets.data()
            );

            update_color_buffer();
            update_position_buffer();

            re_write_set_buffers(
                _descriptor_sets[0],
                1,
                &POSITION_DESCRIPTOR_SET,
                &_pos_buffer,
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
    };

    class Rectangle2D :
        public ColoredItem<Rectangle2D>,
        public Positioned2DItem<Rectangle2D>,
        public SimpleColored2DItem<Rectangle2D>
    {
    private:
        Color _c = {1, 1, 1};
        Float2 _p = {0, 0};
        Float2 _size = {1, 1};
        float _angle = 0.0f;

    private:
        Color get_color() override { return _c; };
        void set_color(const Color& color) override { _c = color; };
        Float2 get_position() override { return _p; };
        void set_position(const Float2& position) override { _p = position; };
        float get_angle() override { return _angle; };
        void set_angle(float angle) override { _angle = angle; };
        Float2 get_size() override { return _size; };
        void set_size(const Float2& size) override { _size = size; };

    private:
        Rectangle2D() = default;

    protected:
        RE_pBuffer get_vertex_buffer(const VertexBuffers& vb) override
        {
            return vb.rectangle;
        }

    private:
        friend Scene2D;
    };


    class Triangle2D :
        public ColoredItem<Triangle2D>,
        public Positioned2DItem<Triangle2D>,
        public SimpleColored2DItem<Triangle2D>
    {
    private:
        Color _c = {1, 1, 1};
        Float2 _p = {0, 0};
        Float2 _size = {1, 1};
        float _angle = 0.0f;

    private:
        Color get_color() override { return _c; };
        void set_color(const Color& color) override { _c = color; };
        Float2 get_position() override { return _p; };
        void set_position(const Float2& position) override { _p = position; };
        float get_angle() override { return _angle; };
        void set_angle(float angle) override { _angle = angle; };
        Float2 get_size() override { return _size; };
        void set_size(const Float2& size) override { _size = size; };

    private:
        Triangle2D() = default;

    protected:
        RE_pBuffer get_vertex_buffer(const VertexBuffers& vb) override
        {
            return vb.triangle;
        }

    private:
        friend Scene2D;
    };

    class Scene
    {
    protected:
        virtual void finalise(RE_pShaderModule module, const VertexBuffers& buffers) = 0;
        virtual void render(RE_pShaderModule module, RE_pImage target_image) = 0;

    public:
        virtual ~Scene() = default;
        friend SimpleDrawer;
    };

    class Scene2D : public Scene
    {
        std::vector<Rectangle2D> rectangles = {};
        std::vector<Triangle2D> triangles = {};

        std::vector<RE_RenderObject> render_objects = {};
        RE_pDescriptorPool descriptor_pool = {};
        RE_pBuffer image_data = {};

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

            return std::views::concat(rects, triang);
        }

    public:
        ~Scene2D() override
        {
            re_free_descriptor_pool(descriptor_pool);

            for (auto& rec : rectangles)
            {
                rec.free(false);
            }

            for (auto& triangle : triangles)
            {
                triangle.free(0);
            }

            re_free_buffer(image_data);
        }

        Rectangle2D& rect()
        {
            rectangles.push_back({});
            return rectangles.back();
        }

        Triangle2D& triangle()
        {
            triangles.push_back({});
            return triangles.back();
        }

    protected:
        void finalise(RE_pShaderModule module, const VertexBuffers& vertex_buffers) override
        {
            descriptor_pool = re_create_descriptor_pool(
                module, (rectangles.size() + triangles.size()) * 2
            );
            render_objects.reserve(rectangles.size() + triangles.size());

            this->image_data = re_create_buffer(
                sizeof(ImageData),
                true,
                false
            );

            for (auto* item : get_all_items())
            {
                render_objects.push_back(item->get_render_object(vertex_buffers, descriptor_pool));
            }
        }

        void render(RE_pShaderModule module, RE_pImage target_image) override
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

        friend SimpleDrawer;
    };

    template <class T>
        requires std::derived_from<T, Scene>
    using RenderCallback = std::function<void(T&)>;

    class SimpleDrawer
    {
        static constexpr auto TRIANGLE_VERTICES = std::array<Float2, 3>{
            {
                {0.0f, -0.5f}, // Top
                {0.5f, 0.5f}, // BR
                {-0.5f, 0.5f}, // BL
            }
        };

        static constexpr auto QUAD_VERTICES = std::array<Float2, 6>{
            {
                {-0.5f, -0.5f}, // TL
                {0.5f, -0.5f}, // TR
                {0.5f, 0.5f}, // BR
                {-0.5f, -0.5f}, // TL
                {0.5f, 0.5f}, // BR
                {-0.5f, 0.5f}, // BL
            }
        };

        RE_pShaderModule _simple_shader;
        VertexBuffers _vertex_buffers{};

        std::unordered_map<std::string, Scene*> scenes = {};

        void populate_vertex_buffers()
        {
            struct StageCopyContext
            {
                RE_pBuffer stage_buffer;
            };
            _vertex_buffers.rectangle = re_create_buffer(QUAD_VERTICES.size() * sizeof(Float2), false, false);
            _vertex_buffers.triangle = re_create_buffer(TRIANGLE_VERTICES.size() * sizeof(Float2), false, false);
            RE_pBuffer staging_buffer = re_create_buffer(
                (QUAD_VERTICES.size() + TRIANGLE_VERTICES.size()) * sizeof(Float2), true, false);

            {
                auto staging_buffer_context = std::span(reinterpret_cast<Float2*>(re_map_buffer(staging_buffer)),
                                                        QUAD_VERTICES.size() + TRIANGLE_VERTICES.size());
                std::ranges::copy(QUAD_VERTICES, staging_buffer_context.begin());
                std::ranges::copy(TRIANGLE_VERTICES, staging_buffer_context.begin() + QUAD_VERTICES.size());
                re_unmap_buffer(staging_buffer);
            }
            auto* context = new StageCopyContext{
                .stage_buffer = staging_buffer
            };

            std::array<RE_BufferToBufferTransfer, 2> buffer_copies = {
                RE_BufferToBufferTransfer{
                    .from_buffer = staging_buffer,
                    .to_buffer = _vertex_buffers.rectangle,
                    .from_index = 0,
                    .to_index = 0,
                    .size = 0
                },
                RE_BufferToBufferTransfer{
                    .from_buffer = staging_buffer,
                    .to_buffer = _vertex_buffers.triangle,
                    .from_index = QUAD_VERTICES.size() * sizeof(Float2),
                    .to_index = 0,
                    .size = 0
                }
            };

            re_transfer_buffers(
                buffer_copies.data(),
                buffer_copies.size(),
                [](RE_CallbackContext context)
                {
                    auto* _context = reinterpret_cast<StageCopyContext*>(context);

                    re_free_buffer(_context->stage_buffer);
                    delete _context;
                },
                context
            );
        }

    public:
        SimpleDrawer(const std::string& shader_path)
        {
            RE_pSpirVCode simple_shader_code = re_load_spirv_code(shader_path.c_str());
            _simple_shader = re_create_shader_module(simple_shader_code);
            re_free_spirv_code(simple_shader_code);
            re_register_render_pipeline(
                _simple_shader,
                SIMPLE_COLOR_NAME.c_str(),
                DIRECT_2D_NAME.c_str(),
                COLORED_NAME.c_str(),
                false
            );

            populate_vertex_buffers();
        }

        ~SimpleDrawer()
        {
            for (auto [_, scene] : scenes)
            {
                delete scene;
            }
            re_free_buffer(_vertex_buffers.rectangle);
            re_free_buffer(_vertex_buffers.triangle);
            re_free_shader_module(_simple_shader);
        }

        void render_scene_to_texture(
            const std::string& name,
            RE_pImage target_image
        )
        {
            if (!scenes.contains(name))
            {
                return;
            }

            scenes.at(name)->render(_simple_shader, target_image);
        }

        void add_scene_2d(
            const std::string& scene_name,
            const RenderCallback<Scene2D>& callback
        )
        {
            auto* new_context = new Scene2D();
            callback(*new_context);
            new_context->finalise(this->_simple_shader, _vertex_buffers);
            scenes.emplace(scene_name, dynamic_cast<Scene*>(new_context));
        }
    };
}

#endif //RENDERENGINE_SIMPLE_DRAW_H
