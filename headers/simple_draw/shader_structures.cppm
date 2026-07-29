//
// Created by Onion on 22.07.2026.
//
module;
#include <cstdint>

#if defined(__APPLE__)
#include <cmath>
#include <numbers>
#endif

export module shader_structures;

#if defined(__linux__)
import std;
#endif

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

    export struct Quaternion
    {
        float x;
        float y;
        float z;
        float w;

        constexpr Quaternion() : x(0.0f), y(0.0f), z(0.0f), w(1.0f) {}

        constexpr Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

        static Quaternion from_axis_angle(const Float3& axis, float angle_radians)
        {
            float x = axis.x;
            float y = axis.y;
            float z = axis.z;

            const float length = std::sqrt(x * x + y * y + z * z);
            if (length > 0.0f)
            {
                const float inv_length = 1.0f / length;
                x *= inv_length;
                y *= inv_length;
                z *= inv_length;
            }

            const float half_angle = angle_radians * 0.5f;
            const float s = std::sin(half_angle);

            return Quaternion(x * s, y * s, z * s, std::cos(half_angle));
        }
    };

    // Row-major storage (m[row][col]) — matches slang's default float4x4
    // layout (same as HLSL), so instances can be copied straight into a
    // constant buffer such as CameraData/VertexDataTransformed.
    export struct Mat4x4
    {
        float m[4][4] = {};

        constexpr Mat4x4() : m{
            {1.0f, 0.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 1.0f, 0.0f},
            {0.0f, 0.0f, 0.0f, 1.0f}
        } {}

        constexpr Mat4x4(
            float m00, float m01, float m02, float m03,
            float m10, float m11, float m12, float m13,
            float m20, float m21, float m22, float m23,
            float m30, float m31, float m32, float m33
        ) : m{
            {m00, m01, m02, m03},
            {m10, m11, m12, m13},
            {m20, m21, m22, m23},
            {m30, m31, m32, m33}
        } {}

        explicit Mat4x4(const Quaternion& q)
        {
            float x = q.x;
            float y = q.y;
            float z = q.z;
            float w = q.w;

            const float length = std::sqrt(x * x + y * y + z * z + w * w);
            if (length > 0.0f)
            {
                const float inv_length = 1.0f / length;
                x *= inv_length;
                y *= inv_length;
                z *= inv_length;
                w *= inv_length;
            }

            const float xx = x * x, yy = y * y, zz = z * z;
            const float xy = x * y, xz = x * z, yz = y * z;
            const float wx = w * x, wy = w * y, wz = w * z;

            m[0][0] = 1.0f - 2.0f * (yy + zz); m[0][1] = 2.0f * (xy - wz);         m[0][2] = 2.0f * (xz + wy);         m[0][3] = 0.0f;
            m[1][0] = 2.0f * (xy + wz);        m[1][1] = 1.0f - 2.0f * (xx + zz); m[1][2] = 2.0f * (yz - wx);         m[1][3] = 0.0f;
            m[2][0] = 2.0f * (xz - wy);        m[2][1] = 2.0f * (yz + wx);         m[2][2] = 1.0f - 2.0f * (xx + yy); m[2][3] = 0.0f;
            m[3][0] = 0.0f;                    m[3][1] = 0.0f;                     m[3][2] = 0.0f;                     m[3][3] = 1.0f;
        }

        Mat4x4 operator*(const Mat4x4& other) const
        {
            Mat4x4 result{};

            for (int row = 0; row < 4; ++row)
            {
                for (int col = 0; col < 4; ++col)
                {
                    float sum = 0.0f;
                    for (int k = 0; k < 4; ++k)
                    {
                        sum += m[row][k] * other.m[k][col];
                    }
                    result.m[row][col] = sum;
                }
            }

            return result;
        }

        // Right-handed projection targeting Vulkan's clip space: depth is
        // mapped to [0, 1] (Vulkan depth range) instead of OpenGL's [-1, 1].
        // No Y flip here: this engine's local/view space already has +Y
        // pointing down (matching Vulkan's native NDC), same as the
        // unflipped 2D pipeline — flipping Y here would reverse the vertex
        // winding that cullMode/frontFace (clockwise-front) expect.
        static Mat4x4 perspective(float fov_degrees, uint32_t width, uint32_t height, float near_plane, float far_plane)
        {
            const float fov_radians = fov_degrees * (std::numbers::pi_v<float> / 180.0f);
            const float f = 1.0f / std::tan(fov_radians * 0.5f);
            const float aspect = static_cast<float>(width) / static_cast<float>(height);

            return Mat4x4(
                f / aspect, 0.0f, 0.0f, 0.0f,
                0.0f, f, 0.0f, 0.0f,
                0.0f, 0.0f, far_plane / (near_plane - far_plane), (far_plane * near_plane) / (near_plane - far_plane),
                0.0f, 0.0f, -1.0f, 0.0f
            );
        }

        static constexpr Mat4x4 scale(const Float3& s)
        {
            return Mat4x4(
                s.x, 0.0f, 0.0f, 0.0f,
                0.0f, s.y, 0.0f, 0.0f,
                0.0f, 0.0f, s.z, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f
            );
        }

        static constexpr Mat4x4 position(const Float3& p)
        {
            return Mat4x4(
                1.0f, 0.0f, 0.0f, p.x,
                0.0f, 1.0f, 0.0f, p.y,
                0.0f, 0.0f, 1.0f, p.z,
                0.0f, 0.0f, 0.0f, 1.0f
            );
        }
    };

    export struct VertexData2D
    {
        Float2 position;
        Float2 scale;
        float angle;
    };

    export struct CameraData
    {
        Mat4x4 projection;
        Mat4x4 location;
    };

    export struct TransformData
    {
        Mat4x4 transform_matrix;
    };

    export struct ImageData
    {
        uint32_t width;
        uint32_t height;
    };

}
