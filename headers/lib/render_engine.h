//
// Created by Роман  Тимофеев on 27.04.2026.
//

#ifndef RENDERENGINE_RENDER_ENGINE_H
#define RENDERENGINE_RENDER_ENGINE_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <re_typedefs.h>

#define EXPORT_RE extern "C"

/**
 * Before using any of libraries functionality initialization function must be called.
 *
 * @param [in] window pointer to valid glfw's window handler, must be created and managed manually
 */
EXPORT_RE void init_render_engine(
    GLFWwindow *window
);

/**
 * After using libraries functionality all resources must be freed.
 *
 * @note All created buffers/images/descriptor pools/etc must be freed manual before freeing engine
 */
EXPORT_RE void re_free_render_engine();

/**
 * Creates buffer.
 *
 * @param [in] byte_size size in bytes of memory to allocate
 * @param [in] host_access set this if you want to use re_map_buffer for direct access to buffer contents
 * @param [in] random_access set this if access to buffer may be not sequential for API and driver specific optimisation
 *
 * @return Handle to created buffer
 */
EXPORT_RE RE_pBuffer re_create_buffer(
    size_t byte_size,
    bool host_access,
    bool random_access
);

/**
 * Returns buffers internal size
 *
 * @param [in] buffer handle to buffer]
 *
 * @return Size of buffer in bytes
 */
EXPORT_RE uint64_t re_get_buffer_size(RE_pBuffer buffer);

/**
 * Load texture from file to buffer
 *
 * @param [in] path must be string with relative/full path to loaded textures
 * @param [out] width pointer for storing texture's width may be NULL
 * @param [out] height pointer for storing texture's height may be NULL
 *
 * @return Handle to new buffer holding decoded textures pixel data 8bits per chanel RGBA, flattened row major.
 */
EXPORT_RE RE_pBuffer re_load_image_to_buffer(
    const char *path,
    uint32_t *width,
    uint32_t *height
);

/**
 * Returns total mesh count in mesh loading cache
 *
 * @param [in] load_cache cache acquired from @ref re_load_model_to_buffer
 *
 * @return Total count of all meshes in cache
 */
EXPORT_RE uint32_t re_get_model_mesh_count(
    void* load_cache
);

/**
 * If cache is nullptr then load mesh from file.
 * If cache is valid cache pointer then returns mesh by its index.
 * If model_index is -1 then only creates cache skipping model loading
 * If cache is nullptr then it will be not created
 * @note Cache must be freed manually
 *
 * Resulting data will be arranged as follows:
 * vertex (float * 3); uv's (float * 2); normals (float * 3);
 * if uv's and normals not presented then format will shrink
 *
 * @param [in] path path to file to load model from
 * @param [in] include_uv tells to load/generate uv's to buffer
 * @param [in] include_normal tells to load/generate normals to buffer
 * @param [in] flip_order tells to flip vertex order
 * @param [in] model_index index of model in file to load
 * @param [out] load_cache pointer for cache storing, may be null
 *
 * @return If model_index != -1 then returns handler for newly created buffer
 */
EXPORT_RE RE_pBuffer re_load_model_to_buffer(
    const char *path,
    bool include_uv,
    bool include_normal,
    bool flip_order,
    int64_t model_index,
    void** load_cache
);

/**
 * Frees model cache
 *
 * @param [in] cache cache acquired from @ref re_load_model_to_buffer
 */
EXPORT_RE void re_free_load_cache(void* cache);

/**
 * Return texture dimension skipping loading it to memory
 *
 *
 * @param [in] path must be string with relative/full path to loaded textures
 * @param [out] width pointer for storing texture's width
 * @param [out] height pointer for storing texture's height
 */
EXPORT_RE void re_fetch_image_file_extent(
    const char *path,
    uint32_t *width,
    uint32_t *height
);

/**
 * Maps buffer to host memory
 * @note Must be unmapped manually
 *
 * @param buffer buffer to map
 *
 * @return Pointer to buffer bytes
 */
EXPORT_RE uint8_t *re_map_buffer(RE_pBuffer buffer);

/**
 * Unmaps buffer
 *
 * @param [in] buffer buffer to unmap, previously mapped with @ref re_map_buffer
 */
EXPORT_RE void re_unmap_buffer(RE_pBuffer buffer);

/**
 * Performs buffer transfer on gpu
 * Buffer transfering is performed in parallel, so if you need to do somethung after transfer is completed use end_callback
 *
 * @param [in] buffers_copies pointer to ranges of buffers to transfer
 * @param [in] num_buffers_copies number of buffers copies to perform
 * @param [in] end_callback pointer to transfer end callback, can be null
 * @param [in] context pointer to data that should be passed to end_callback
 *
 * @note context is copied by pointer to end_callback and must be freed by hand inside of callback
 */
EXPORT_RE void re_transfer_buffers(
    RE_BufferToBufferTransfer *buffers_copies,
    size_t num_buffers_copies,
    RE_OperationEndCallback end_callback,
    RE_CallbackContext context
);

/**
 * Performs by byte copy from buffer to image
 * All data from buffer is copied, except if image is too small, then it will be cut.
 * Transferring is performed in parallel, so if you need to do something after transfer is completed use end_callback
 *
 * @param [in] source_buffer handle for source buffer
 * @param [in] target_image hand for target image
 * @param [in] end_callback pointer to transfer end callback, can be null
 * @param [in] context pointer to data that should be passed to end_callback
 *
 * @note context is copied by pointer to end_callback and must be freed by hand inside of callback
 */
EXPORT_RE void re_transfer_buffer_to_image(
     RE_pBuffer source_buffer,
     RE_pImage target_image,
     RE_OperationEndCallback end_callback,
     RE_CallbackContext context
 );

/**
 * Free allocated buffer
 *
 * @param [in] buffer handle to buffer to free
 */
EXPORT_RE void re_free_buffer(
    RE_pBuffer buffer
);

/**
 * Created shader module from previously loaded spir-v code
 * All registered pipeline will be free automatically upon module deletion
 *
 * @note Make sure that yours shader compiler includes metadata, because lib automatically figures vertex layout and descriptor set sizes.
 * @note For every compute shader automatically pipeline will be created, but graphics pipeline must be allocated by hand.
 *
 * @param shader_code handler for shader's code
 *
 * @return Handler to freshly created module
 */
EXPORT_RE RE_pShaderModule re_create_shader_module(
    RE_pSpirVCode shader_code
);

/**
 * Registries render pipeline in loaded module.
 *
 * @param shader_module existing shader module
 * @param pipeline_name name for pipeline being registered with
 * @param vertex_name name of vertex shader function
 * @param fragment_name name of fragment shader function
 * @param depth flag for enabling depth test
 */
EXPORT_RE void re_register_render_pipeline(
    RE_pShaderModule shader_module,
    const char *pipeline_name,
    const char *vertex_name,
    const char *fragment_name,
    bool depth
);

/**
 * Creates buffer compatible with pipelines layout that suitable for storing specific count of vertices.
 *
 * @note Resulting buffer is not host accessible.
 *
 * @param shader_module module where pipeline is stored
 * @param pipeline_name name of render pipeline which must be compatible with this vertex buffer
 * @param vertex_count number of vertices to stores
 *
 * @return Handler for new buffer
 */
EXPORT_RE RE_pBuffer re_allocate_vertex_buffer(
    RE_pShaderModule shader_module,
    const char *pipeline_name,
    uint32_t vertex_count
);

/**
 * Free shader module
 *
 * @param [in] shader_module handle to shader module to free
 */
EXPORT_RE void re_free_shader_module(
    RE_pShaderModule shader_module
);


/**
 * Creates image stored on a device
 *
 * @param [in] width width of image in pixels
 * @param [in] height height of image in pixels
 * @param [in] format pixel format of image
 * @param [in] linear_filtering flag for enabling linear filtering, if false then nearest sampling will be used.
 * @param [in] repeat_u enables repeating across u when sampling
 * @param [in] repeat_v enables repeating across v when sampling
 */
EXPORT_RE RE_pImage re_create_image(
    uint32_t width,
    uint32_t height,
    RE_IMAGE_FORMATS format,
    bool linear_filtering, // true - linear; false - nearest,
    bool repeat_u, //true - repeat; false - clamp to edge,
    bool repeat_v
);

/**
 * Returns swap chains next present image
 *
 * @note It must not be freed by hand
 *
 * @return Handler for current present image
 */
EXPORT_RE RE_pImage re_get_present_image();

/**
 * Returns image's dimensions
 *
 * @param [in] image handle to image to query
 * @param [out] width pointer for storing image's width
 * @param [out] height pointer for storing image's height
 */
EXPORT_RE void re_get_image_dimensions(
    RE_pImage image,
    uint32_t *width,
    uint32_t *height
);

/**
 * Frees allocated image
 *
 * @param [in] image handle to image to free
 */
EXPORT_RE void re_free_image(
    RE_pImage image
);

/**
 * Performs scaling transferring from image to image
 *
 * @param [in] transfers pointer to array of image to image transfer descriptions
 * @param [in] transfer_count number of transfers to perform
 */
EXPORT_RE void re_transfer_image_to_image(
    RE_ImageToImageTransfer* transfers,
    uint32_t transfer_count
);

/**
 * Creates descriptor pool capable for storing specific amount of descriptor sets compatible with all pipelines of module.
 *
 * @param [in] shader_module module whose pipelines the pool's descriptor sets will be compatible with
 * @param [in] descriptor_count maximum amount of sets that pool can store
 *
 * @return Handler for new pool
 */
EXPORT_RE RE_pDescriptorPool re_create_descriptor_pool(
    RE_pShaderModule shader_module,
    uint32_t descriptor_count
);

/**
 * Frees descriptor pool
 * @note Allocated sets will be freed automatically
 *
 * @param [in] descriptor_pool handle to descriptor pool to free
 */
EXPORT_RE void re_free_descriptor_pool(
    RE_pDescriptorPool descriptor_pool
);

/**
 * Allocates descriptor set
 *
 * @param [in] descriptor_pool pool to allocate sets from
 * @param [in] set_count amount of allocated sets
 * @param [in] set_index array of sets indices
 * @param [out] sets pointer to preallocated memory for storing sets handles
 */
EXPORT_RE void re_create_descriptor_sets(
    RE_pDescriptorPool descriptor_pool,
    uint32_t set_count,
    uint32_t *set_index,
    RE_pDescriptorSet *sets
);

/**
 * Updates buffer bindings
 *
 * @param [in] set descriptor set to update
 * @param [in] write_count amount of buffers to update
 * @param [in] names array of shader's binding names where buffers must be bound
 * @param [in] buffers array of buffers to bind
 * @param [in] offsets offsets of buffers, can be null
 * @param [in] sizes size of bound buffers, can be null
 */
EXPORT_RE void re_write_set_buffers(
    RE_pDescriptorSet set,
    size_t write_count,
    const char **names,
    RE_pBuffer *buffers,

    uint32_t* offsets,
    uint32_t* sizes
);

/**
 * Updated image bindings
 *
 * @param [in] set descriptor set to update
 * @param [in] write_count amount of images to update
 * @param [in] names array of shader's binding names where images must be bound
 * @param [in] images array of images to bind
 */
EXPORT_RE void re_write_set_images(
    RE_pDescriptorSet set,
    size_t write_count,
    const char **names,
    RE_pImage *images
);

/**
 * Free allocated descriptor set
 *
 * @param [in] set_count amount of descriptor sets to free
 * @param [in] descriptor_set array of descriptor sets to free
 */
EXPORT_RE void re_free_descriptor_sets(
    uint32_t set_count,
    RE_pDescriptorSet *descriptor_set
);

/**
 * Dispatches compute shader
 *
 * @param [in] shader_name name of compute shader function to dispatch
 * @param [in] shader_module module where shader is stored
 * @param [in] set_count amount of descriptor sets to bind
 * @param [in] sets array of descriptor sets to bind
 * @param [in] group_x number of local workgroups to dispatch along x
 * @param [in] group_y number of local workgroups to dispatch along y
 * @param [in] group_z number of local workgroups to dispatch along z
 */
EXPORT_RE void re_dispatch_compute_shader(
    const char *shader_name,
    RE_pShaderModule shader_module,
    uint32_t set_count,
    RE_pDescriptorSet *sets,
    uint32_t group_x,
    uint32_t group_y,
    uint32_t group_z
);

/**
 * Dispatches render pipeline
 *
 * @param [in] pipeline_name name of registered render pipeline to use
 * @param [in] shader_module module where pipeline is stored
 * @param [in] image_count amount of target images
 * @param [in] target_images images that render will be performed to
 * @param [in] render_object_count amount of objects to render
 * @param [in] render_objects array of objects to render
 * @param [in] depth_image pointer to depth image if pipelin uses depth buffering
 * @param [in] load_image flag that will disable image clearing before rendering
 */
EXPORT_RE void re_render(
    const char *pipeline_name,
    RE_pShaderModule shader_module,
    uint32_t image_count,
    RE_pImage *target_images,

    uint32_t render_object_count,
    RE_RenderObject *render_objects,


    RE_pImage *depth_image,
    bool load_image
);

/**
 *  Presents swap chain image
 *  @note Use only acquired present imagee
 *
 * @param [in] image present image acquired from @ref re_get_present_image
 */
EXPORT_RE void re_present_image(
    RE_pImage image
);

/**
 * Waits till device finishes all work
 */
EXPORT_RE void re_wait_device_free();

#undef EXPORT_RE

#endif //RENDERENGINE_RENDER_ENGINE_H
