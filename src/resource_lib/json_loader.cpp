//
// Created by Onion on 08.07.2026.
//
#include "resource_lib.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <exception>

#define EXPORT_R extern "C"

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

    rm_load_models(
        abs_path.c_str(),
        resource_name.c_str(),
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

    rm_load_texture(
        abs_path.c_str(),
        resource_name.c_str(),
        linear_filtering,
        repeat_u,
        repeat_v
    );
}

EXPORT_R void rm_load_resources_from_json(
    const char* path_to_json
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
