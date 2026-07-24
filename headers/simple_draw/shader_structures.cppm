//
// Created by Onion on 22.07.2026.
//
module;
#include <cstdint>

export module shader_structures;

namespace RenderEngine
{
    export struct Float2
    {
        float x;
        float y;

        constexpr Float2(float v) : x(v), y(v){}

        constexpr Float2(float x, float y) : x(x), y(y){}
    };

    export struct Float3
    {
        float x;
        float y;
        float z;

        constexpr Float3(float v) : x(v), y(v), z(v){}

        constexpr Float3(float x, float y, float z) : x(x), y(y), z(z){}
    };

    export using Color = Float3;

    export struct VertexData2D
    {
        Float2 position;
        Float2 scale;
        float angle;
    };

    export struct ImageData
    {
        uint32_t width;
        uint32_t height;
    };

}
