//
// Created by Роман  Тимофеев on 06.06.2026.
//

#ifndef RENDERENGINE_RESOURCE_LIB_H
#define RENDERENGINE_RESOURCE_LIB_H

#define EXPORT_R extern "C"
#include <cstdint>

#include "re_typedefs.h"

EXPORT_R void rm_init_resource_manager();

//loads all models with names formated as {model_name}_{mesh_index}
//where mesh_index in range from 0 to output values excluding
EXPORT_R uint32_t rm_load_models(
    const char* model_path,
    const char* model_name,
    bool flip_triangle_order,
    bool include_uvs,
    bool include_normals
);

EXPORT_R RE_pBuffer rm_get_loaded_model(
    const char* model_name
);

EXPORT_R void rm_load_texture(
    const char* texture_path,
    const char* texture_name,
    bool linear_filtering,
    bool repeat_u,
    bool repeat_v
);

EXPORT_R RE_pImage rm_get_loaded_texture(
    const char* texture_name
);

EXPORT_R void rm_free_models();
EXPORT_R void rm_free_images();

EXPORT_R void rm_free_resource_manager();

#undef EXPORT_R

#endif //RENDERENGINE_RESOURCE_LIB_H
