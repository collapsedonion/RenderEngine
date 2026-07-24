//
// Created by Роман  Тимофеев on 21.05.2026.
//

module;

#include "export_macro.h"
#include <re_typedefs.h>

export module dispatchers;

export EXPORT_RE void re_dispatch_compute_shader(
    const char *shader_name,
    RE_pShaderModule shader_module,
    uint32_t set_count,
    RE_pDescriptorSet *sets,
    uint32_t group_x,
    uint32_t group_y,
    uint32_t group_z
);

export EXPORT_RE void re_render(
    const char *pipeline_name,
    RE_pShaderModule shader_module,
    uint32_t image_count,
    RE_pImage *target_images,

    uint32_t render_object_count,
    RE_RenderObject *render_objects,

    RE_pImage *depth_image,

    bool load_image
);

export EXPORT_RE void re_transfer_buffer_to_image(
    RE_pBuffer source_buffer,
    RE_pImage target_image,
    RE_OperationEndCallback end_callback,
    RE_CallbackContext context
);

export EXPORT_RE void re_transfer_image_to_image(
    RE_ImageToImageTransfer* transfers,
    uint32_t transfer_count
);

export EXPORT_RE void re_transfer_buffers(
    RE_BufferToBufferTransfer *buffers_copies,
    size_t num_buffers_copies,
    RE_OperationEndCallback end_callback,
    RE_CallbackContext context
);
