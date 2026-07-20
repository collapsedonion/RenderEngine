//
// Created by Роман  Тимофеев on 29.04.2026.
//

#ifndef RENDERENGINE_RE_TYPEDEFS_H
#define RENDERENGINE_RE_TYPEDEFS_H
#include <cstdint>

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

struct RE_BufferToBufferTransfer {
    RE_pBuffer from_buffer;
    RE_pBuffer to_buffer;
    uint32_t from_index = 0;
    uint32_t to_index = 0;
    uint32_t size = 0;
};

struct RE_RenderObject {
    RE_pBuffer vertex_buffer;
    uint32_t descriptor_set_count;
    RE_pDescriptorSet *descriptor_sets;
};

struct RE_ImageToImageTransfer
{
    RE_pImage from_image;
    RE_pImage to_image;
    uint32_t from_offset_x = 0;
    uint32_t from_offset_y = 0;
    uint32_t from_width = 0;
    uint32_t from_height = 0;
    uint32_t to_offset_x = 0;
    uint32_t to_offset_y = 0;
    uint32_t to_width = 0;
    uint32_t to_height = 0;
};

#endif //RENDERENGINE_RE_TYPEDEFS_H
