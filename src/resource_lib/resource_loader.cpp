//
// Created by Роман  Тимофеев on 06.06.2026.
//
#include <resource_lib.h>
#include <unordered_map>
#include <vector>
#include <string>
#include <format>

#include <render_engine.h>

#define EXPORT_R extern "C"

std::unordered_map<std::string, RE_pBuffer> r_loaded_models;
std::unordered_map<std::string, RE_pImage> r_loaded_images;

EXPORT_R void rm_init_resource_manager() {
    r_loaded_models = {};
    r_loaded_images = {};
}

struct ModelContext {
    std::vector<RE_pBuffer> loaded_models;
};

struct ImageContext {
    RE_pBuffer stage_buffer;
};

void load_model_callback(RE_CallbackContext context) {
    ModelContext* _context = static_cast<ModelContext*>(context);

    for (auto buffer : _context->loaded_models) {
        re_free_buffer(buffer);
    }

    delete _context;
}

void load_image_callback(RE_CallbackContext context) {
    auto* _context = static_cast<ImageContext*>(context);

    re_free_buffer(_context->stage_buffer);

    delete _context;
}

EXPORT_R uint32_t rm_load_models(
    const char *model_path,
    const char *model_name,
    bool flip_triangle_order,
    bool include_uvs,
    bool include_normals
) {
    void *cache = nullptr;

    re_load_model_to_buffer(
        model_path,
        include_uvs,
        include_normals,
        flip_triangle_order,
        -1,
        &cache
    );

    uint32_t mesh_count = re_get_model_mesh_count(cache);

    auto* context = new ModelContext();

    std::vector<RE_pBuffer> device_meshes;
    std::vector<RE_BufferToBufferTransfer> buffer_transfers;
    context->loaded_models.reserve(mesh_count);
    device_meshes.reserve(mesh_count);
    buffer_transfers.reserve(mesh_count);

    for (uint32_t i = 0; i < mesh_count; i++) {
        RE_pBuffer loaded_mesh = re_load_model_to_buffer(
            model_path,
            include_uvs,
            include_normals,
            flip_triangle_order,
            i,
            &cache
        );

        context->loaded_models.push_back(loaded_mesh);

        RE_pBuffer new_mesh = re_create_buffer(
            re_get_buffer_size(loaded_mesh),
            false,
            false
        );

        device_meshes.push_back(new_mesh);
        r_loaded_models.insert(
            {
                std::format("{}_{}", model_name, i),
                new_mesh
            }
        );

        RE_BufferToBufferTransfer buffer_to_transfer {};
        buffer_to_transfer.from_buffer = loaded_mesh;
        buffer_to_transfer.from_index = 0;
        buffer_to_transfer.to_buffer = new_mesh;
        buffer_to_transfer.to_index = 0;
        buffer_to_transfer.size = re_get_buffer_size(new_mesh);
        buffer_transfers.push_back(buffer_to_transfer);
    }

    re_transfer_buffers(
        buffer_transfers.data(),
        buffer_transfers.size(),
        load_model_callback,
        context
    );

    re_free_load_cache(cache);

    return mesh_count;
}

EXPORT_R void rm_load_texture(
    const char* texture_path,
    const char* texture_name,
    bool linear_filtering,
    bool repeat_u,
    bool repeat_v
) {
    uint32_t width, height;

    RE_pBuffer loaded_image =
        re_load_image_to_buffer(
          texture_path,
            &width,
            &height
        );

    RE_pImage image = re_create_image(
        width,
        height,
        RE_IMAGE_FORMAT_RGBA8,
        linear_filtering,
        repeat_u,
        repeat_v
    );

    ImageContext* context = new ImageContext();
    context->stage_buffer = loaded_image;

    re_transfer_buffer_to_image(
        loaded_image,
        image,
        load_image_callback,
        context
    );

    r_loaded_images.insert(
        {texture_name, image}
    );
}

EXPORT_R RE_pBuffer rm_get_loaded_model(
    const char* model_name
) {
    if (!r_loaded_models.contains(model_name)) {
        return nullptr;
    }

    return r_loaded_models[model_name];
}

EXPORT_R RE_pBuffer rm_get_loaded_texture(
    const char* model_name
) {
    if (!r_loaded_images.contains(model_name)) {
        return nullptr;
    }

    return r_loaded_images[model_name];
}

EXPORT_R void rm_free_models() {
    re_wait_device_free();

    for (auto buffer: r_loaded_models) {
        re_free_buffer(buffer.second);
    }

    r_loaded_models.clear();
}

EXPORT_R void rm_free_images() {
    re_wait_device_free();

    for (auto image: r_loaded_images) {
        re_free_image(image.second);
    }

    r_loaded_models.clear();
}

EXPORT_R void rm_free_resource_manager() {
    rm_free_models();
    rm_free_images();
}
