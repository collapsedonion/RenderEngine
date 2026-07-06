//
// Created by Роман  Тимофеев on 29.04.2026.
//

#ifndef RENDERENGINE_RE_TYPEDEFS_H
#define RENDERENGINE_RE_TYPEDEFS_H

enum RE_IMAGE_FORMATS {
    RE_IMAGE_FORMAT_R8,
    RE_IMAGE_FORMAT_RGB8,
    RE_IMAGE_FORMAT_BGR8,
    RE_IMAGE_FORMAT_RGBA8,
    RE_IMAGE_FORMAT_DEPTH
};

typedef void* RE_pBuffer;
typedef void* RE_pSpirVCode;
typedef void* RE_pShaderModule;
typedef void* RE_pDescriptorPool;
typedef void* RE_pDescriptorSet;
typedef void* RE_pImage;

#endif //RENDERENGINE_RE_TYPEDEFS_H
