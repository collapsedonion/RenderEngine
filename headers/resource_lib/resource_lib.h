//
// Created by Роман  Тимофеев on 06.06.2026.
//

#ifndef RENDERENGINE_RESOURCE_LIB_H
#define RENDERENGINE_RESOURCE_LIB_H

#define EXPORT_R extern "C"
#include <cstdint>

#include "re_typedefs.h"

/**
 * @file resource_lib.h
 *
 * High level resource manager built on top of @ref render_engine.h.
 * Loads meshes/textures from disk, uploads them to device buffers/images and
 * keeps them accessible by name for the lifetime of the manager.
 *
 * @section resource_lib_json JSON resource description format
 *
 * @ref rm_load_resources_from_json accepts a path to a JSON file describing
 * a set of resources to load. The file is a single JSON object where each
 * key is the resource's name (used later to look it up, e.g. with
 * @ref rm_get_loaded_texture, or as the model_name prefix for
 * @ref rm_get_loaded_mesh) and each value is an object describing that
 * resource. All relative "path" values are resolved relative to the
 * directory containing the JSON file itself, not the process' working
 * directory.
 *
 * Every resource entry must contain:
 *  - "type": either "mesh" or "texture"
 *  - "path": path to the source file (relative to the JSON file's directory,
 *    or absolute)
 *
 * A "mesh" entry is loaded via @ref rm_load_models and additionally accepts:
 *  - "flip_triangle_order" (bool, default false)
 *  - "include_uvs" (bool, default true)
 *  - "include_normals" (bool, default true)
 *
 * A "texture" entry is loaded via @ref rm_load_texture and additionally accepts:
 *  - "linear_filtering" (bool, default false)
 *  - "repeat_u" (bool, default true)
 *  - "repeat_v" (bool, default true)
 *
 * Example:
 * @code{.json}
 * {
 *   "bricks_texture":
 *   {
 *     "type": "texture",
 *     "path": "./bricks.jpg",
 *     "linear_filtering": false,
 *     "repeat_u": false,
 *     "repeat_v": false
 *   },
 *   "teapot_model":
 *   {
 *     "type": "mesh",
 *     "path": "utah_teapot.obj",
 *     "flip_triangle_order": true,
 *     "include_uvs": true,
 *     "include_normals": false
 *   }
 * }
 * @endcode
 */

/**
 * Initializes resource manager's internal storage.
 * Must be called before using any other function of this library.
 */
EXPORT_R void rm_init_resource_manager();

/**
 * Loads a batch of resources described by a JSON file.
 *
 * @see resource_lib_json for the accepted JSON format.
 *
 * @param [in] path_to_json path to JSON file describing resources to load
 *
 * @note Throws std::runtime_error if a resource entry is missing its
 * "type"/"path" field or if "type" is neither "mesh" nor "texture".
 */
EXPORT_R void rm_load_resources_from_json(
    const char* path_to_json
);

/**
 * Loads all meshes from a model file and uploads them to device buffers.
 * Loaded meshes are named as {model_name}_{mesh_index}, where mesh_index is
 * in range [0, returned mesh count), and can later be retrieved with
 * @ref rm_get_loaded_mesh.
 *
 * @param [in] model_path path to model file to load
 * @param [in] model_name name to register loaded meshes under
 * @param [in] flip_triangle_order tells to flip vertex order
 * @param [in] include_uvs tells to load/generate uv's for each mesh
 * @param [in] include_normals tells to load/generate normals for each mesh
 *
 * @return Total count of meshes loaded from model file
 */
EXPORT_R uint32_t rm_load_models(
    const char* model_path,
    const char* model_name,
    bool flip_triangle_order,
    bool include_uvs,
    bool include_normals
);

/**
 * Returns previously loaded mesh buffer by its name.
 *
 * @param [in] mesh_name name formatted as {model_name}_{mesh_index}, as
 * produced by @ref rm_load_models
 *
 * @return Handle to mesh's vertex buffer, or nullptr if mesh_name is
 * malformed or no such mesh was loaded
 */
EXPORT_R RE_pBuffer rm_get_loaded_mesh(
    const char* mesh_name
);

/**
 * Returns amount of meshes loaded for given model name.
 *
 * @param [in] model_name name meshes were registered under with @ref rm_load_models
 *
 * @return Amount of loaded meshes, or 0 if model_name is unknown
 */
EXPORT_R uint32_t rm_get_loaded_mesh_count(
    const char* model_name
);

/**
 * Loads texture from file and uploads it to a device image.
 *
 * @param [in] texture_path path to texture file to load
 * @param [in] texture_name name to register loaded texture under
 * @param [in] linear_filtering flag for enabling linear filtering, if false then nearest sampling will be used
 * @param [in] repeat_u enables repeating across u when sampling
 * @param [in] repeat_v enables repeating across v when sampling
 */
EXPORT_R void rm_load_texture(
    const char* texture_path,
    const char* texture_name,
    bool linear_filtering,
    bool repeat_u,
    bool repeat_v
);

/**
 * Returns previously loaded texture image by its name.
 *
 * @param [in] texture_name name texture was registered under with @ref rm_load_texture
 *
 * @return Handle to loaded image, or nullptr if no such texture was loaded
 */
EXPORT_R RE_pImage rm_get_loaded_texture(
    const char* texture_name
);

/**
 * Frees all loaded mesh buffers and clears mesh registry.
 * @note Waits for device to finish all work before freeing.
 */
EXPORT_R void rm_free_models();

/**
 * Frees all loaded texture images and clears texture registry.
 * @note Waits for device to finish all work before freeing.
 */
EXPORT_R void rm_free_images();

/**
 * Frees all resources owned by the resource manager.
 * Equivalent to calling @ref rm_free_models followed by @ref rm_free_images.
 */
EXPORT_R void rm_free_resource_manager();

#undef EXPORT_R

#endif //RENDERENGINE_RESOURCE_LIB_H
