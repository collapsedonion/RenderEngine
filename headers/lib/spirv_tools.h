//
// Created by Роман  Тимофеев on 28.04.2026.
//

#ifndef RENDERENGINE_SPIRV_TOOLS_H
#define RENDERENGINE_SPIRV_TOOLS_H

#include <re_typedefs.h>

#define EXPORT_RE extern "C"

EXPORT_RE RE_pSpirVCode re_load_spirv_code(const char* path);
EXPORT_RE void re_free_spirv_code(RE_pSpirVCode code);

#undef EXPORT_RE

#endif //RENDERENGINE_SPIRV_TOOLS_H
