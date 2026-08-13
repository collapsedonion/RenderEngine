//
// Created by Роман  Тимофеев on 29.04.2026.
//

#ifndef RENDERENGINE_RE_TYPEDEFS_H
#define RENDERENGINE_RE_TYPEDEFS_H
#include <cstdint>
#include <cstddef>

enum RE_IMAGE_FORMATS {
    RE_IMAGE_FORMAT_R8,
    RE_IMAGE_FORMAT_RGB8,
    RE_IMAGE_FORMAT_BGR8,
    RE_IMAGE_FORMAT_RGBA8,
    RE_IMAGE_FORMAT_DEPTH
};

/*
 * All objects in RenderEngine are manged by hand, use should manually call free functions when needed
 */
typedef void* RE_pSpirVCode;

#endif //RENDERENGINE_RE_TYPEDEFS_H
