//
// Created by Роман  Тимофеев on 06.06.2026.
//
module;
#include <re_typedefs.h>
#include <nlohmann/json.hpp>

#ifdef __APPLE__
#include <fstream>
#endif

module resource_lib;

#ifdef __linux__
import std;
#endif
import Transporters;
import dynamic_dispatchable_iterator;
import render_engine;

static std::unordered_map<std::string, std::vector<std::shared_ptr<RenderEngine::RawBuffer>>> r_loaded_models;
static std::unordered_map<std::string, std::shared_ptr<RenderEngine::Image>> r_loaded_images;

using namespace RenderEngine;
void ResourceLib::init_resource_manager() {
    r_loaded_models = {};
    r_loaded_images = {};
}


std::uint32_t ResourceLib::load_models(
    const std::filesystem::path& model_path,
    const std::string& model_name,
    bool flip_triangle_order,
    bool include_uvs,
    bool include_normals
)
{
    auto loader = ModelLoader(
        model_path,
        flip_triangle_order
    );

    std::size_t mesh_count = loader.get_mesh_count();

    auto* cpu_buffer = new std::vector<std::shared_ptr<RawBuffer>>();
    std::vector<std::shared_ptr<RawBuffer>> device_meshes;
    std::vector<BufferToBufferInfo> buffer_transfers;
    device_meshes.reserve(mesh_count);
    cpu_buffer->reserve(mesh_count);
    buffer_transfers.reserve(mesh_count);

    for (std::size_t i = 0; i < mesh_count; i++) {
        std::shared_ptr<RawBuffer> loaded_mesh = loader.load_mesh(
            i,
            include_uvs,
            include_normals
        );

        cpu_buffer->push_back(loaded_mesh);

        std::shared_ptr<RawBuffer> new_mesh = RawBuffer::create(
            loaded_mesh->size(),
            false,
            false
        );

        device_meshes.push_back(new_mesh);

        BufferToBufferInfo buffer_to_transfer {};
        buffer_to_transfer.from = loaded_mesh;
        buffer_to_transfer.from_index = 0;
        buffer_to_transfer.to = new_mesh;
        buffer_to_transfer.to_index = 0;
        buffer_to_transfer.size = new_mesh->size();
        buffer_transfers.push_back(buffer_to_transfer);
    }

    auto it = RangedIterator(std::views::all(buffer_transfers));

    transfer_buffer_to_buffer(
       it,
        [=]()
        {
            cpu_buffer->clear();
            delete cpu_buffer;
        }
    );

    r_loaded_models.insert(
        {
            model_name,
            std::move(device_meshes)
        }
    );

    return mesh_count;
}

void ResourceLib::load_texture(
    const std::filesystem::path& texture_path,
    const std::string& texture_name,
    bool linear_filtering,
    bool repeat_u,
    bool repeat_v
) {
    auto [text, width, height] = RenderEngine::load_texture_to_buffer(
        texture_path
    );

    std::shared_ptr<Image> image = Image::create(
        width,
        height,
        RE_IMAGE_FORMAT_RGBA8,
        linear_filtering,
        repeat_u,
        repeat_v
    );

    transfer_buffer_to_image(
        text,
        image,
        [=]()
        {
            auto ptr = text;
            ptr.reset();
        }
    );

    r_loaded_images.insert(
        {texture_name, image}
    );
}

std::shared_ptr<RawBuffer> ResourceLib::get_loaded_mesh(
    const std::string& c_mesh_name
) {
    const std::string mesh_name(c_mesh_name);

    size_t last_undescore_index = mesh_name.rfind('_');

    if (last_undescore_index == std::string::npos)
    {
        return nullptr;
    }

    auto model_name = mesh_name.substr(0, last_undescore_index);
    auto mesh_index = std::stoull(mesh_name.substr(last_undescore_index+1));

    if (!r_loaded_models.contains(model_name)) {
        return nullptr;
    }

    auto& mesh_vector = r_loaded_models.at(model_name);

    if (mesh_index >= mesh_vector.size())
    {
        return nullptr;
    }

    return mesh_vector[mesh_index];
}


std::uint32_t ResourceLib::get_loaded_mesh_count(
    const std::string& model_name
)
{
    if (r_loaded_models.contains(model_name))
    {
       return r_loaded_models[model_name].size();
    }

    return 0;
}

std::shared_ptr<Image> ResourceLib::get_loaded_texture(
    const std::string& model_name
) {
    if (!r_loaded_images.contains(model_name)) {
        return nullptr;
    }

    return r_loaded_images[model_name];
}

void ResourceLib::free_models()
{
    RenderEngine::wait_device_free();

    r_loaded_models.clear();
}

void ResourceLib::free_images() {
    wait_device_free();
    r_loaded_images.clear();
}

void ResourceLib::free_resource_manager() {
    free_models();
    free_images();
}


const std::string MESH_FLIP_TRIANGLE_ORDER_NAME = "flip_triangle_order";
const std::string MESH_INCLUDE_UVS_NAME = "include_uvs";
const std::string MESH_INCLUDE_NORMALS_NAME = "include_normals";

const std::string TEXTURE_LINEAR_FILTERING_NAME = "linear_filtering";
const std::string TEXTURE_REPEAT_U_NAME = "repeat_u";
const std::string TEXTURE_REPEAT_V_NAME = "repeat_v";

inline void process_mesh(
    const std::filesystem::path& reference_path,
    const std::string& resource_name,
    nlohmann::json& mesh_json
)
{
    bool flip_triangle_order = false;
    bool include_uvs = true;
    bool include_normals = true;

    std::string rel_path = mesh_json["path"].get<std::string>();

    std::filesystem::path abs_path = reference_path / std::filesystem::path(std::move(rel_path));

    if (mesh_json.contains(MESH_FLIP_TRIANGLE_ORDER_NAME))
    {
        flip_triangle_order = mesh_json[MESH_FLIP_TRIANGLE_ORDER_NAME].get<bool>();
    }

    if (mesh_json.contains(MESH_INCLUDE_NORMALS_NAME))
    {
        include_normals = mesh_json[MESH_INCLUDE_NORMALS_NAME].get<bool>();
    }

    if (mesh_json.contains(MESH_INCLUDE_UVS_NAME))
    {
        include_uvs = mesh_json[MESH_INCLUDE_UVS_NAME].get<bool>();
    }

    ResourceLib::load_models(
        abs_path,
        resource_name,
        flip_triangle_order,
        include_uvs,
        include_normals
    );
}

inline void process_texture(
    const std::filesystem::path& reference_path,
    const std::string& resource_name,
    nlohmann::json& texture_json
)
{
    bool linear_filtering = false;
    bool repeat_u = true;
    bool repeat_v = true;

    std::string rel_path = texture_json["path"].get<std::string>();

    std::filesystem::path abs_path = reference_path / std::filesystem::path(std::move(rel_path));

    if (texture_json.contains(TEXTURE_LINEAR_FILTERING_NAME))
    {
        linear_filtering = texture_json[TEXTURE_LINEAR_FILTERING_NAME].get<bool>();
    }

    if (texture_json.contains(TEXTURE_REPEAT_U_NAME))
    {
        repeat_u = texture_json[TEXTURE_REPEAT_U_NAME].get<bool>();
    }

    if (texture_json.contains(TEXTURE_LINEAR_FILTERING_NAME))
    {
        repeat_v = texture_json[TEXTURE_REPEAT_V_NAME].get<bool>();
    }

    ResourceLib::load_texture(
        abs_path,
        resource_name,
        linear_filtering,
        repeat_u,
        repeat_v
    );
}

void ResourceLib::load_resources_from_json(
    const std::filesystem::path& path_to_json
)
{
    std::filesystem::path json_path(path_to_json);
    std::ifstream file(json_path);
    auto reference_path = std::filesystem::absolute(std::move(json_path)).parent_path();

    auto parsed = nlohmann::json::parse(file);
    file.close();

    for (auto& [resource_name, resource_entry] : parsed.items())
    {
        if (!resource_entry.contains("type"))
        {
            throw std::runtime_error("Missing type field in resource entry");
        }

        if (!resource_entry.contains("path"))
        {
            throw std::runtime_error("Missing path field in resource entry");
        }

        std::string resource_type = resource_entry["type"].get<std::string>();

        if (resource_type == "mesh")
        {
            process_mesh(reference_path, resource_name, resource_entry);
        }
        else if (resource_type == "texture")
        {
            process_texture(reference_path, resource_name, resource_entry);
        }
        else
        {
            throw std::runtime_error(std::format(
                "Invalid type {} of field {}",
                resource_type,
                resource_name
            ));
        }
    }
}
