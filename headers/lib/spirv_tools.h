//
// Created by Роман  Тимофеев on 28.04.2026.
//

#ifndef RENDERENGINE_SPIRV_TOOLS_H
#define RENDERENGINE_SPIRV_TOOLS_H

#include <re_typedefs.h>

#define EXPORT_RE extern "C"

/**
 * Loads compiled SPIR-V code from file
 *
 * @param [in] path must be string with relative/full path to compiled shader's SPIR-V binary
 *
 * @return Handle to loaded SPIR-V code, suitable for @ref re_create_shader_module
 */
EXPORT_RE RE_pSpirVCode re_load_spirv_code(const char* path);

/**
 * Frees loaded SPIR-V code
 *
 * @param [in] code handle to SPIR-V code acquired from @ref re_load_spirv_code
 */
EXPORT_RE void re_free_spirv_code(RE_pSpirVCode code);

#undef EXPORT_RE

#endif //RENDERENGINE_SPIRV_TOOLS_H
