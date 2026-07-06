//
// Created by Роман  Тимофеев on 27.04.2026.
//

#ifndef RENDERENGINE_RENDER_ENGINE_H
#define RENDERENGINE_RENDER_ENGINE_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <re_typedefs.h>

typedef void* RE_CallbackContext;
typedef void(*RE_OperationEndCallback)(RE_CallbackContext);
#define EXPORT_RE extern "C"

struct RE_BufferToBufferTransfer {
    RE_pBuffer from_buffer;
    RE_pBuffer to_buffer;
    size_t from_index;
    size_t to_index;
    size_t size;
};

struct RE_RenderObject {
    RE_pBuffer vertex_buffer;
    uint32_t descriptor_set_count;
    RE_pDescriptorSet *descriptor_sets;
};

//Initialisation
EXPORT_RE void init_render_engine(
    GLFWwindow *window
);

EXPORT_RE void re_free_render_engine();

//Buffer manipulation
EXPORT_RE RE_pBuffer re_create_buffer(
    size_t byte_size,
    bool host_access,
    bool random_access
);

EXPORT_RE uint64_t re_get_buffer_size(RE_pBuffer buffer);

EXPORT_RE RE_pBuffer re_load_image_to_buffer(
    const char *path,
    uint32_t *width,
    uint32_t *height
);

EXPORT_RE uint32_t re_get_model_mesh_count(
    void* load_cache
);

//cache must be preinialised to nullptr, -1 resource index signals to skip loading to buffer, only emit cache
EXPORT_RE RE_pBuffer re_load_model_to_buffer(
    const char *path,
    bool include_uv,
    bool include_normal,
    bool flip_order,
    int64_t model_index,
    void** load_cache
);

EXPORT_RE void re_free_load_cache(void* cache);

EXPORT_RE void re_fetch_image_file_extent(
    const char *path,
    uint32_t *width,
    uint32_t *height
);

EXPORT_RE uint8_t *re_map_buffer(RE_pBuffer buffer);

EXPORT_RE void re_unmap_buffer(RE_pBuffer buffer);

EXPORT_RE void re_transfer_buffers(
    RE_BufferToBufferTransfer *buffers_copies,
    size_t num_buffers_copies,
    RE_OperationEndCallback end_callback,
    RE_CallbackContext context
);

EXPORT_RE void re_transfer_buffer_to_image(
     RE_pBuffer source_buffer,
     RE_pImage target_image,
     RE_OperationEndCallback end_callback,
     RE_CallbackContext context
 );

EXPORT_RE void re_free_buffer(
    RE_pBuffer buffer
);

//Shader manipulation
EXPORT_RE RE_pShaderModule re_create_shader_module(
    RE_pSpirVCode shader_code
);

EXPORT_RE void re_register_render_pipeline(
    RE_pShaderModule shader_module,
    const char *pipeline_name,
    const char *vertex_name,
    const char *fragment_name,
    bool depth
);

EXPORT_RE RE_pBuffer re_allocate_vertex_buffer(
    RE_pShaderModule shader_module,
    const char *pipeline_name,
    uint32_t vertex_count
);

EXPORT_RE void re_free_shader_module(
    RE_pShaderModule shader_module
);

//Image operations
EXPORT_RE RE_pImage re_create_image(
    uint32_t width,
    uint32_t height,
    RE_IMAGE_FORMATS format,
    bool linear_filtering, // true - linear; false - nearest,
    bool repeat_u, //true - repeat; false - clamp to edge,
    bool repeat_v
);

EXPORT_RE RE_pImage re_get_present_image();

EXPORT_RE void re_get_image_dimensions(
    RE_pImage image,
    uint32_t *width,
    uint32_t *height
);

EXPORT_RE void re_free_image(
    RE_pImage image
);

//Descriptor manipulation
EXPORT_RE RE_pDescriptorPool re_create_descriptor_pool(
    RE_pShaderModule shader_module,
    uint32_t descriptor_count
);

EXPORT_RE void re_free_descriptor_pool(
    RE_pDescriptorPool descriptor_pool
);

EXPORT_RE void re_create_descriptor_sets(
    RE_pDescriptorPool descriptor_pool,
    uint32_t set_count,
    uint32_t *set_index,
    RE_pDescriptorSet *sets
);

EXPORT_RE void re_write_set_buffers(
    RE_pDescriptorSet set,
    size_t write_count,
    const char **names,
    RE_pBuffer *buffers
);

EXPORT_RE void re_write_set_images(
    RE_pDescriptorSet set,
    size_t write_count,
    const char **names,
    RE_pImage *images
);

EXPORT_RE void re_display_image();

//Sets must be in same descriptor pool
EXPORT_RE void re_free_descriptor_sets(
    uint32_t set_count,
    RE_pDescriptorSet *descriptor_set
);

//Computing dispatchers
EXPORT_RE void re_dispatch_compute_shader(
    const char *shader_name,
    RE_pShaderModule shader_module,
    uint32_t set_count,
    RE_pDescriptorSet *sets,
    uint32_t group_x,
    uint32_t group_y,
    uint32_t group_z
);

EXPORT_RE void re_render(
    const char *pipeline_name,
    RE_pShaderModule shader_module,
    uint32_t image_count,
    RE_pImage *target_images,

    uint32_t render_object_count,
    RE_RenderObject *render_objects,


    RE_pImage *depth_image
);

EXPORT_RE void re_present_image(
    RE_pImage image
);

//Utility manipulation
EXPORT_RE void re_wait_device_free();

#undef EXPORT_RE

#endif //RENDERENGINE_RENDER_ENGINE_H
