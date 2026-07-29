//
// Created by Onion on 22.07.2026.
//
module;
#include <sys/stat.h>
#if defined(__APPLE__)
#include <string>
#endif

export module setters;
export import shader_structures;
#if defined(__linux__)
    export import std;
#endif

namespace RenderEngine
{
    export template <class T>
    class ColorSetter
    {
    public:
        T& color(const Color& color)
            requires requires(T& t, const Color& c) { t.set_color(c); }
        {
            static_cast<T&>(*this).set_color(color);
            return static_cast<T&>(*this);
        }
    };

    export template <class T>
    class PositionSetter
    {
    public:
        T& position(const Float2& position)
            requires requires(T& t, const Float2& v) { t.set_position(v); }
        {
            static_cast<T&>(*this).set_position(position);
            return static_cast<T&>(*this);
        }

        T& angle(float angle)
            requires requires(T& t, float a) { t.set_angle(a); }
        {
            static_cast<T&>(*this).set_angle(angle);
            return static_cast<T&>(*this);
        }

        T& size(const Float2& size)
            requires requires(T& t, const Float2& v) { t.set_size(v); }
        {
            static_cast<T&>(*this).set_size(size);
            return static_cast<T&>(*this);
        }
    };

    export template <class T>
    class NameSetter
    {
    public:
        T& name(std::string_view name)
            requires requires(T& t, std::string_view s) {t.set_name(s);}
        {
            static_cast<T&>(*this).set_name(name);
            return static_cast<T&>(*this);
        }
    };

    export template <class T>
    class TransformSetter
    {
    public:
        T& position_matrix(const Mat4x4& m)
            requires requires(T& t, const Mat4x4& v) { t.set_position_matrix(v); }
        {
            static_cast<T&>(*this).set_position_matrix(m);
            return static_cast<T&>(*this);
        }

        T& rotation_matrix(const Mat4x4& m)
            requires requires(T& t, const Mat4x4& v) { t.set_rotation_matrix(v); }
        {
            static_cast<T&>(*this).set_rotation_matrix(m);
            return static_cast<T&>(*this);
        }

        T& scale_matrix(const Mat4x4& m)
            requires requires(T& t, const Mat4x4& v) { t.set_scale_matrix(v); }
        {
            static_cast<T&>(*this).set_scale_matrix(m);
            return static_cast<T&>(*this);
        }

        T& rotation(const Quaternion& q)
            requires requires(T& t, const Mat4x4& v) { t.set_rotation_matrix(v); }
        {
            static_cast<T&>(*this).set_rotation_matrix(Mat4x4(q));
            return static_cast<T&>(*this);
        }

        T& rotation(const Float3& axis, float angle_radians)
            requires requires(T& t, const Mat4x4& v) { t.set_rotation_matrix(v); }
        {
            static_cast<T&>(*this).set_rotation_matrix(Mat4x4(Quaternion::from_axis_angle(axis, angle_radians)));
            return static_cast<T&>(*this);
        }
    };
}
