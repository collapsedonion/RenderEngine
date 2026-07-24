//
// Created by Onion on 22.07.2026.
//

export module setters;
export import shader_structures;

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
}
