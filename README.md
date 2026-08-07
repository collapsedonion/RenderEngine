## RenderEngine
This library tries to provide rendering capabilities and experience similar to OpenGL API
but with modern Vulkan API, also it's provides automations of descriptor set creation, image layout transitions and resource access synchronization.

## Structure
* `headers/lib` provides headers for render/compute functions also helper functions for resource and shader loading
* `headers/resource_lib` provides library for more complex resource loading functionality 
* `headers/simple_draw` provides library for simple rendering with flat primitives without shading and lighting

## Examples
* In `main.cpp` provided simple example of rendering several textured objects.
* In `simple_draw_main` provided example of rendering with simple_draw lib.

## Building

* gcc recommended for building, or similar compiler with support of C++20 modules and C++23 "import std" on linux.
* On MacOS glfw must be installed for successful compilation.

* !!!Building on Windows is not currently supported but planed for future!!! 

Also provided CMakeLists.txt only provides configuration for building with Vulkan SDK 1.4.350.
