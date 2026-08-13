//
// Created by Onion on 10.08.2026.
//
#include <render_engine.h>

#include "export_macro.h"
import render_engine;

EXPORT_RE void init_render_engine(GLFWwindow* window)
{
    RenderEngine::init(window);
}

EXPORT_RE void re_free_render_engine()
{
    RenderEngine::release();
}