//
// Created by Onion on 22.07.2026.
//

module;
#include <render_engine.h>
#include <spirv_tools.h>
module simple_draw;

namespace RenderEngine
{
    // SimpleDrawer

    void SimpleDrawer::populate_vertex_buffers()
    {
        struct StageCopyContext
        {
            RE_pBuffer stage_buffer;
        };

        _vertex_buffers.rectangle = re_create_buffer(QUAD_VERTICES.size() * sizeof(Float2), false, false);
        _vertex_buffers.triangle = re_create_buffer(TRIANGLE_VERTICES.size() * sizeof(Float2), false, false);
        auto circle_range = circle_vertices(.5f, 20);

        _vertex_buffers.circle = re_create_buffer(circle_range.size() * sizeof(Float2), false, false);
        auto full_size = QUAD_VERTICES.size() + TRIANGLE_VERTICES.size() + circle_range.size();

        RE_pBuffer staging_buffer = re_create_buffer(
            (full_size) * sizeof(Float2), true, false);

        {
            auto staging_buffer_context = std::span(reinterpret_cast<Float2*>(re_map_buffer(staging_buffer)),
                                                    full_size);
            std::ranges::copy(QUAD_VERTICES, staging_buffer_context.begin());
            std::ranges::copy(TRIANGLE_VERTICES, staging_buffer_context.begin() + QUAD_VERTICES.size());
            std::ranges::copy(circle_range,
                              staging_buffer_context.begin() + QUAD_VERTICES.size() + TRIANGLE_VERTICES.size());

            re_unmap_buffer(staging_buffer);
        }
        auto* context = new StageCopyContext{
            .stage_buffer = staging_buffer
        };

        std::array<RE_BufferToBufferTransfer, 3> buffer_copies = {
            RE_BufferToBufferTransfer{
                .from_buffer = staging_buffer,
                .to_buffer = _vertex_buffers.rectangle,
                .from_index = 0,
                .to_index = 0,
                .size = 0
            },
            RE_BufferToBufferTransfer{
                .from_buffer = staging_buffer,
                .to_buffer = _vertex_buffers.triangle,
                .from_index = QUAD_VERTICES.size() * sizeof(Float2),
                .to_index = 0,
                .size = 0
            },
            RE_BufferToBufferTransfer{
                .from_buffer = staging_buffer,
                .to_buffer = _vertex_buffers.circle,
                .from_index = (QUAD_VERTICES.size() + TRIANGLE_VERTICES.size()) * sizeof(Float2),
                .to_index = 0,
                .size = 0
            }
        };

        re_transfer_buffers(
            buffer_copies.data(),
            buffer_copies.size(),
            [](RE_CallbackContext context)
            {
                auto* _context = reinterpret_cast<StageCopyContext*>(context);

                re_free_buffer(_context->stage_buffer);
                delete _context;
            },
            context
        );
    }

    SimpleDrawer::SimpleDrawer(const std::string& shader_path)
    {
        RE_pSpirVCode simple_shader_code = re_load_spirv_code(shader_path.c_str());
        _simple_shader = re_create_shader_module(simple_shader_code);
        re_free_spirv_code(simple_shader_code);
        re_register_render_pipeline(
            _simple_shader,
            SIMPLE_COLOR_NAME.c_str(),
            DIRECT_2D_NAME.c_str(),
            COLORED_NAME.c_str(),
            false
        );

        populate_vertex_buffers();
    }

    SimpleDrawer::~SimpleDrawer()
    {
        for (auto [_, scene] : scenes)
        {
            delete scene;
        }
        re_free_buffer(_vertex_buffers.rectangle);
        re_free_buffer(_vertex_buffers.triangle);
        re_free_shader_module(_simple_shader);
    }

    void SimpleDrawer::render_scene_to_texture(
        const std::string& name,
        RE_pImage target_image
    )
    {
        if (!scenes.contains(name))
        {
            return;
        }

        scenes.at(name)->render(_simple_shader, target_image);
    }
}
