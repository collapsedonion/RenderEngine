//
// Created by Onion on 17.07.2026.
//
module;

#include "spirv_tools.h"
export module simple_draw;
export import shader_structures;
import std;

namespace RenderEngine
{
    export inline const std::string DIRECT_2D_NAME = "direct_vertex_shader_2D";
    export inline const std::string COLORED_NAME = "colored_fragment_shader";

    export inline const char* POSITION_DESCRIPTOR_SET = "data2DV";
    export inline const char* COLOR_DESCRIPTOR_SET = "dataColor";
    export inline const char* IMAGE_DESCRIPTOR_SET = "imageData";

    export inline const std::string SIMPLE_COLOR_NAME = "2D_colored";

    export struct VertexBuffers
    {
        RE_pBuffer rectangle;
        RE_pBuffer triangle;
        RE_pBuffer circle;
    };

    export class SceneItem
    {
    public:
        virtual RE_RenderObject get_render_object(const VertexBuffers& buffers, RE_pDescriptorPool pool) = 0;

        virtual ~SceneItem() = default;
    };

    export class ColoredItem
    {
    public:
        virtual Color get_color() = 0;
        virtual void set_color(const Color& color) = 0;
        virtual ~ColoredItem() = default;
        Color color() { return get_color(); };
    };

    export class Positioned2DItem
    {
    public:
        virtual Float2 get_position() = 0;
        virtual void set_position(const Float2& position) = 0;

        virtual float get_angle() = 0;
        virtual void set_angle(float angle) = 0;

        virtual Float2 get_size() = 0;
        virtual void set_size(const Float2& size) = 0;

        virtual ~Positioned2DItem() = default;

        Float2 position() { return get_position(); }
        float angle() { return get_angle(); }
        Float2 size() { return get_size(); }
    };

    export class Scene
    {
    public:
        virtual void finalise(RE_pShaderModule module, const VertexBuffers& buffers) = 0;
        virtual void render(RE_pShaderModule module, RE_pImage target_image) = 0;

        virtual ~Scene() = default;
    };

    export template <class T>
        requires std::derived_from<T, Scene>
    using RenderCallback = std::function<void(T&)>;

    export class SimpleDrawer
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

        // Returns auto — must stay in header
        static auto circle_vertices(float radius, uint32_t point_count)
        {
            const float step = 2.0f * std::numbers::pi_v<float> / static_cast<float>(point_count);

            return std::views::iota(0u, point_count * 3)
                | std::views::transform([radius, step, point_count](uint32_t idx) -> Float2
                {
                    const uint32_t tri = idx / 3;
                    const uint32_t vert = idx % 3;
                    if (vert == 0) return {0.0f, 0.0f};
                    const uint32_t i = (vert == 1) ? tri : (tri + 1) % point_count;
                    const float angle = static_cast<float>(i) * step;
                    return {radius * std::sin(angle), -radius * std::cos(angle)};
                });
        }

        void populate_vertex_buffers();

    public:
        SimpleDrawer(const std::string& shader_path);
        ~SimpleDrawer();

        void render_scene_to_texture(
            const std::string& name,
            RE_pImage target_image
        );

        template<class T>
        void add_scene(
            const std::string& scene_name,
            const RenderCallback<T>& callback
        )
        {
            auto* new_context = new T();
            callback(*new_context);
            new_context->finalise(this->_simple_shader, _vertex_buffers);
            scenes.emplace(scene_name, dynamic_cast<Scene*>(new_context));
        }
    };
}
