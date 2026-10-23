//
// Created by Роман  Тимофеев on 18.08.2026.
//
module;
#include <glfw/glfw3.h>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/matrix.hpp>

#ifdef __APPLE__
#include <functional>
#include <filesystem>
#include <unordered_set>
#include <ranges>
#endif

#include <spirv_tools.h>

module RenderManager;
import render_engine;
import resource_lib;
import Transporters;
import GrowingDescriptorPool;
import dynamic_dispatchable_iterator;
import SceneData;
import Dispatchers;

using namespace RenderEngine;

bool RenderManager::_initialized = false;
std::shared_ptr<ShaderModule> RenderManager::_geometry_shader_module = {};
std::shared_ptr<ShaderModule> RenderManager::_light_pass_module = {};

std::unordered_map<
    std::string,
    RenderTarget
> RenderManager::_render_targets = {};

std::unordered_set<
    std::string
> RenderManager::_window_sized_scenes = {};

std::unordered_map<
    std::string,
    SceneData
> RenderManager::_scenes = {};

std::unordered_map<
    std::string,
    Material
> RenderManager::_materials = {};


GrowingDescriptorPool *RenderManager::_gdp = nullptr;
GrowingDescriptorPool *RenderManager::_light_pass_descriptor_pool = nullptr;

void RenderManager::update_window_size(std::uint32_t width, std::uint32_t height) {
    for (auto &_scene: _window_sized_scenes) {
        _scenes.at(_scene).set_aspect(width, height, true);
    }
}

GLFWwindow *RenderManager::_window = nullptr;

void window_resize_callback(GLFWwindow *window, int width, int height);

void RenderManager::init(
    const std::filesystem::path &shader_code_path,
    const std::filesystem::path &lp_code_path
    ) {
    if (_initialized) {
        return;
    }

    _initialized = true;

    glfwInit();
    glfwInitVulkanLoader(vkGetInstanceProcAddr);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    _window = glfwCreateWindow(
        800,
        600,
        "Engine",
        nullptr,
        nullptr
    );

    RenderEngine::init(_window);

    auto spvCode = re_load_spirv_code(shader_code_path.c_str());

    _geometry_shader_module = ShaderModule::create(spvCode);
    _geometry_shader_module->register_render_pipeline(
        GEOMETRY_PASS_NAME,
        "vertex_shader",
        "fragment_shader",
        true,
        {
            RE_IMAGE_FORMAT_RGBA8,
            RE_IMAGE_FORMAT_FLOAT4,
            RE_IMAGE_FORMAT_FLOAT4,
            RE_IMAGE_FORMAT_FLOAT4
        }
    );
    re_free_spirv_code(spvCode);

    auto lpCode = re_load_spirv_code(
        lp_code_path.c_str()
    );

    _light_pass_module = ShaderModule::create(
        lpCode
    );
    re_free_spirv_code(lpCode);

    ResourceLib::init_resource_manager(
        true,
        true
    );

    _gdp = new GrowingDescriptorPool(
        _geometry_shader_module,
        16
    );

    _light_pass_descriptor_pool = new GrowingDescriptorPool(
        _light_pass_module,
        16
    );

    glfwSetWindowSizeCallback(_window, &window_resize_callback);
}

void RenderManager::start_loop(
    std::function<bool(double dt)> loop_body
) {
    if (!_initialized) {
        return;
    }

    bool app_finished = false;

    auto previous_time_point = std::chrono::high_resolution_clock::now();

    while (
        !glfwWindowShouldClose(_window)
        &&
        !app_finished
    ) {
        glfwPollEvents();
        auto time_point_now = std::chrono::high_resolution_clock::now();
        auto delta = time_point_now - previous_time_point;
        double delta_time =
                std::chrono::duration_cast<std::chrono::duration<double> >(delta).count();

        app_finished = loop_body(delta_time);

        previous_time_point = time_point_now;
    }
}

RenderTarget::RenderTarget(
    std::uint32_t width,
    std::uint32_t height,
    std::optional<std::pair<std::uint32_t, uint32_t>> separate_light_resolution
    ) {
    this->_dimensions = {width, height};

    this->_color = Image::create(
        width,
        height,
        RE_IMAGE_FORMAT_RGBA8,
        false,
        true,
        true
    );

    this->_world_position = Image::create(
        width,
        height,
        RE_IMAGE_FORMAT_FLOAT4,
        false,
        true,
        true
    );

    this->_depth = Image::create(
        width,
        height,
        RE_IMAGE_FORMAT_DEPTH,
        false,
        false,
        false
    );

    this->_world_normals = Image::create(
        width,
        height,
        RE_IMAGE_FORMAT_FLOAT4,
        false,
        true,
        true
    );

    this->_material_data = Image::create(
        width,
        height,
        RE_IMAGE_FORMAT_FLOAT4,
        false,
        true,
        true
    );

    this->_light_map = Image::create(
        separate_light_resolution.has_value() ? separate_light_resolution.value().first : width,
        separate_light_resolution.has_value() ? separate_light_resolution.value().second : height,
        RE_IMAGE_FORMAT_FLOAT4,
        false,
        true,
        true
    );
}

void RenderManager::add_render_target(
    const std::string &name,
    std::uint32_t width,
    std::uint32_t height,
    std::optional<std::pair<std::uint32_t, uint32_t>> separate_light_resolution
) {
    if (!_initialized) {
        return;
    }

    if (_render_targets.contains(name)) {
        return;
    }

    _render_targets.insert(
        {
            name,
            RenderTarget(width, height, separate_light_resolution)
        }
    );
}

void RenderManager::render_scene_to_target(const std::string &name, const std::string &scene_name) {
    if (!_initialized || !_scenes.contains(scene_name) || !_render_targets.contains(name)) {
        return;
    }

    auto &_render_target = _render_targets.at(name);
    auto dimensions = _render_target.get_dimensions();
    auto _render_objects = _scenes.at(scene_name).get_render_objects(
        dimensions.first,
        dimensions.second
    );
    auto _images = _render_target.get_images();
    auto _depth_image = _render_target.get_depth_image();

    dispatch_graphics_pipeline(
        _geometry_shader_module,
        GEOMETRY_PASS_NAME,
        _images,
        _render_objects,
        _depth_image,
        false
    );
}

void RenderManager::bake_light_to_target(const std::string &target_name, const std::string &scene_name) {
    if (!_initialized || !_scenes.contains(scene_name) || !_render_targets.contains(target_name)) {
        return;
    }

    auto& _render_target = _render_targets.at(target_name);
    auto& scene = _scenes.at(scene_name);
    auto dimensions = _render_target.get_dimensions();

    auto light_ds = scene.get_light_descriptor_set();

    wait_device_free();

    auto writes = RangedIterator(
        std::array{
            std::tuple<std::string&, Resource*>{LP_RESULT_NAME, _render_target.get_light_map().get()},
            std::tuple<std::string&, Resource*>{LP_NORMAL_MAP_NAME, _render_target.get_world_normals().get()},
            std::tuple<std::string&, Resource*>{LP_POSITION_MAP_NAME, _render_target.get_world_position().get()},
            std::tuple<std::string&, Resource*>{LP_MATERIAL_MAP_NAME, _render_target.get_material_data().get()}
        }
    );

    light_ds->get_set()->write_bindings(
        writes
    );

    auto it = RangedIterator(
        std::views::single(
            light_ds->get_set().get()
        )
    );

    std::string blin_name = "phong_shading_model";

    dispatch_compute_shader(
        _light_pass_module,
        blin_name,
        it,
        std::ceil((float)dimensions.first / 32),
        std::ceil((float)dimensions.second / 32),
        1
    );
}

void RenderManager::set_scene_fov(const std::string &name, float fov) {
    if (!_initialized || _scenes.contains(name)) {
        return;
    }

    _scenes.at(name).set_fov(fov);
}

void RenderManager::set_scene_aspect_source(const std::string &name, SceneResolutionSource source) {
    if (!_scenes.contains(name)) {
        return;
    }

    if (source != SceneResolutionSource::WINDOW) {
        if (_window_sized_scenes.contains(name)) {
            _window_sized_scenes.erase(name);
        }
    } else {
        _window_sized_scenes.insert(name);
        int width, height;
        glfwGetWindowSize(_window, &width, &height);
        _scenes.at(name).set_aspect(width, height, true);
    }


    _scenes.at(name).set_resolution_source(source);
}

void RenderManager::display_render_target(
    const std::string &name,
    RenderTargetChannel channel_to_present) {
    if (!_initialized) {
        return;
    }

    if (channel_to_present == RenderTargetChannel::DEPTH) {
        return;
    }

    if (!_render_targets.contains(name)) {
        return;
    }

    auto present_image = Image::get_swapchain_image();

    auto &render_target = _render_targets.at(name);
    auto img = render_target.get_image_by_channel(
        channel_to_present
    );

    ImageToImageInfo iti_info = {
        .from_image = img,
        .to_image = present_image
    };

    auto it = RangedIterator(
        std::views::single(iti_info)
    );

    transfer_image_to_image(
        it
    );

    present_image->present();
}

void RenderManager::add_material(const std::string &path_to_material, const std::string &material_name) {
    auto spv_module = re_load_spirv_code(
        path_to_material.c_str()
    );

    auto module = ShaderModule::create(
        spv_module
    );

    re_free_spirv_code(spv_module);

    _materials.insert({
        material_name,
        Material(module)
    });
}

void RenderManager::add_scene(const std::string &name) {
    if (!_initialized || _scenes.contains(name)) {
        return;
    }

    auto camera_set = _gdp->allocate_new_set(0);
    auto lp_set = _light_pass_descriptor_pool->allocate_new_set(0);

    _scenes.insert({
        name,
        SceneData(camera_set, lp_set)
    });
}

void RenderManager::remove_scene(const std::string &name) {
    if (!_initialized || !_scenes.contains(name)) {
        return;
    }

    wait_device_free();
    if (_window_sized_scenes.contains(name)) {
        _window_sized_scenes.erase(name);
    }

    _scenes.erase(name);
}

void RenderManager::stop() {
    if (!_initialized) {
        return;
    }

    RenderEngine::wait_device_free();

    _materials.clear();

    _scenes.clear();

    _render_targets.clear();

    _light_pass_module.reset();
    _geometry_shader_module.reset();

    delete _light_pass_descriptor_pool;
    delete _gdp;
    ResourceLib::free_resource_manager();

    RenderEngine::release();

    glfwDestroyWindow(_window);
    glfwTerminate();
}

void window_resize_callback(GLFWwindow *window, int width, int height) {
    RenderManager::update_window_size(width, height);
}

std::shared_ptr<Image> Material::compute_to_image(
    std::uint32_t width,
    std::uint32_t height,
    const std::string &shader_name,
    std::optional<Iterator<std::tuple<std::string &, Resource *>> &>resource) {
    if (!_shader->contains_compute_pipeline(
        shader_name
    )) {
        return {};
    }

    auto my_set = _my_pool->allocate_new_set(
        0
    );

    if (resource.has_value()) {
        my_set->get_set()->write_bindings(
            resource.value()
        );
    }

    std::shared_ptr<Image> _buffered_image = Image::create(
        width, height,
        RE_IMAGE_FORMAT_FLOAT4,
        false,
        false,
        false
    );

    auto output_image = RangedIterator(
        std::views::single(
            std::tuple<std::string&, Resource *>{
                MATERIAL_OUTPUT_NAME,
                _buffered_image.get()
            }
        )
    );

    my_set->get_set()->write_bindings(
        output_image
    );

    auto it = RangedIterator(
        std::views::single(
            my_set->get_set().get()
        )
    );

    dispatch_compute_shader(
        _shader,
        const_cast<std::string &>(shader_name),
        it,
        std::ceil((float)width / 32),
        std::ceil((float)height / 32),
        1
    );

    std::shared_ptr<Image> _resulting_image = Image::create(
        width, height, RE_IMAGE_FORMAT_RGBA8, false, true, true
    );

    auto img_it = RangedIterator(
        std::views::single(
            ImageToImageInfo{
                _buffered_image,
                _resulting_image
            }
        )
    );

    transfer_image_to_image(
        img_it,
        [=]() {
            auto _bi = _buffered_image;
            _bi.reset();
            auto _set = my_set;
            _set.reset();
        }
    );

    return _resulting_image;
}
