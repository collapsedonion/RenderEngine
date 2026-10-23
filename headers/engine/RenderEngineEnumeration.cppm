//
// Created by Роман  Тимофеев on 18.08.2026.
//
module;
#ifdef __APPLE__
#include <cinttypes>
#endif

export module RenderEngineEnumeration;

export namespace RenderEngine {
    enum class RenderTargetChannel: std::uint8_t {
        COLOR = 0x0,
        DEPTH = 0x1,
        WORLD_POSITION = 0x2,
        WORLD_NORMAL = 0x3,
        LIGHT_MAP = 0x4,
        MATERIAL = 0x5,
    };

    enum class ColoringType : std::uint32_t {
        UV_AS_COLOR = 0b0001,
        COLORED = 0b0010,
        TEXTURED = 0b0100
    };

    enum class SceneResolutionSource : std::uint8_t {
        MANUAL = 0x0,
        WINDOW = 0x1,
        RENDER_TARGET = 0x2
    };

    enum class LightSourceType : std::uint32_t {
        DIRECTIONAL = 0b0001,
        SPOT = 0b0010,
        POINT = 0b0100
    };
}
