//
//
// Created by Onion on 28.07.2026.
//
module;
#include <cstdint>
#include <render_engine.h>

#if defined(__APPLE__)
#include <array>
#include <ranges>
#include <vector>
#endif

export module scene3d;
export import simple_draw;
export import setters;
#if defined(__linux__)
import std;
#endif

namespace RenderEngine
{
    export class Scene3D;

    export class SimpleColored3DItem :
        public NamedItem,
        public SceneItem,
        public ColoredItem,
        public TransformedItem
    {
        static constexpr auto POOL_INDICIES = std::array<uint32_t, 2>{
            2,
            1
        };

        RE_pBuffer _transform_buffer;
        RE_pBuffer _color_buffer;
        RE_pBuffer _camera_matrix_data;


        std::weak_ptr<SimpleColored3DItem> _father_object = {};
        std::unordered_set<SimpleColored3DItem*> _child_objects = {};
        std::string _name = "item3d";
        std::size_t _name_hash = 0;

        std::array<RE_pDescriptorSet, 2> _descriptor_sets = {};

    protected:
        bool render_ready = false;

        void free(bool free_descriptor);

        void set_camera_data_buffer(RE_pBuffer buffer);

        virtual RE_pBuffer get_vertex_buffer(const VertexBuffers& vb) = 0;

        virtual Mat4x4 get_matrix() override;

        void update_transform_buffer();

        void update_color_buffer();

        RE_RenderObject get_render_object(const VertexBuffers& vb, RE_pDescriptorPool pool) override
        {
            _transform_buffer = re_create_buffer(sizeof(TransformData), true, false);
            _color_buffer = re_create_buffer(sizeof(Color), true, false);

            re_create_descriptor_sets(
                pool,
                2,
                const_cast<uint32_t*>(POOL_INDICIES.data()),
                _descriptor_sets.data()
            );

            update_color_buffer();
            update_transform_buffer();

            auto buffers = std::array<RE_pBuffer, 2>{
                _transform_buffer,
                _camera_matrix_data
            };

            auto names = std::array<const char*, 2>{
                TRANSFORM_DATA_DESCRIPTOR_SET,
                CAMERA_MATRIX_DESCRIPTOR_SET
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

    public:

        void set_parent(std::shared_ptr<SimpleColored3DItem> father);
        const std::string& get_name() override;
        void set_name(std::string_view name) override;

        friend Scene3D;
    };

    export class Rectangle3D :
        public SimpleColored3DItem,
        public ColorSetter<Rectangle3D>,
        public TransformSetter<Rectangle3D>,
        public NameSetter<Rectangle3D>
    {
    public:
        using ColorSetter::color;
        using ColoredItem::color;
        using NamedItem::name;
        using NameSetter::name;

    private:
        Color _c = {1, 1, 1};

        Mat4x4 _position_matrix = {};
        Mat4x4 _rotation_matrix = {};
        Mat4x4 _scale_matrix = {};

        Rectangle3D() = default;

    protected:
        RE_pBuffer get_vertex_buffer(const VertexBuffers& vb) override;

    public:
        Color get_color() override;
        void set_color(const Color& color) override;
        const Mat4x4& get_position_matrix() override;
        const Mat4x4& get_rotation_matrix() override;
        const Mat4x4& get_scale_matrix() override;
        void set_position_matrix(const Mat4x4& m) override;
        void set_rotation_matrix(const Mat4x4& m) override;
        void set_scale_matrix(const Mat4x4& m) override;

    protected:
        friend Scene3D;
    };

    export class Triangle3D :
        public SimpleColored3DItem,
        public ColorSetter<Triangle3D>,
        public TransformSetter<Triangle3D>,
        public NameSetter<Triangle3D>
    {
    public:
        using ColorSetter::color;
        using ColoredItem::color;
        using NameSetter::name;
        using NamedItem::name;

    private:
        Color _c = {1, 1, 1};

        Mat4x4 _position_matrix = {};
        Mat4x4 _rotation_matrix = {};
        Mat4x4 _scale_matrix = {};

    public:
        Color get_color() override;
        void set_color(const Color& color) override;
        const Mat4x4& get_position_matrix() override;
        const Mat4x4& get_rotation_matrix() override;
        const Mat4x4& get_scale_matrix() override;
        void set_position_matrix(const Mat4x4& m) override;
        void set_rotation_matrix(const Mat4x4& m) override;
        void set_scale_matrix(const Mat4x4& m) override;

    private:
        Triangle3D() = default;

    protected:
        RE_pBuffer get_vertex_buffer(const VertexBuffers& vb) override;
        friend Scene3D;
    };

    export class Circle3D :
        public SimpleColored3DItem,
        public ColorSetter<Circle3D>,
        public TransformSetter<Circle3D>,
        public NameSetter<Circle3D>
    {
    public:
        using ColorSetter::color;
        using ColoredItem::color;
        using NameSetter::name;
        using NamedItem::name;

    private:
        Color _c = {1, 1, 1};
        Mat4x4 _position_matrix = {};
        Mat4x4 _rotation_matrix = {};
        Mat4x4 _scale_matrix = {};

    public:
        Color get_color() override;
        void set_color(const Color& color) override;
        const Mat4x4& get_position_matrix() override;
        const Mat4x4& get_rotation_matrix() override;
        const Mat4x4& get_scale_matrix() override;
        void set_position_matrix(const Mat4x4& m) override;
        void set_rotation_matrix(const Mat4x4& m) override;
        void set_scale_matrix(const Mat4x4& m) override;

    private:
        Circle3D() = default;

    protected:
        RE_pBuffer get_vertex_buffer(const VertexBuffers& vb) override;
        friend Scene3D;
    };

    class Scene3D : public Scene
    {
        std::vector<std::shared_ptr<SimpleColored3DItem>> items = {};

        std::vector<RE_RenderObject> render_objects = {};
        RE_pDescriptorPool descriptor_pool = {};
        RE_pBuffer camera_data = {};
        RE_pImage _render_image = {};
        RE_pImage _depth_image = {};
        bool _resolution_set = false;

    public:
        Scene3D();
        ~Scene3D() override;

        template <typename T>
            requires std::derived_from<T, SimpleColored3DItem>
        T& add_item()
        {
            items.push_back(
               std::shared_ptr<SimpleColored3DItem>(
                   static_cast<SimpleColored3DItem*>(new T{})
               )
            );
            items.back()->set_camera_data_buffer(this->camera_data);
            return dynamic_cast<T&>(*items.back());
        }

        //[width, height]
        void init_resolution(std::pair<uint32_t, uint32_t> resolution);

        [[nodiscard]]
        std::weak_ptr<SimpleColored3DItem> get_item_by_name(std::string_view name) const;

    protected:
        void finalise(RE_pShaderModule module, const VertexBuffers& vertex_buffers) override;
        void render(RE_pShaderModule module, RE_pImage target_image) override;

        friend SimpleDrawer;
    };
}
