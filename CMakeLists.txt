cmake_minimum_required(VERSION 4.0)
project(untitled4)

set(CMAKE_CXX_STANDARD 20)

# ImGui sources
set(IMGUI_SOURCES
        /home/taha/external//imgui/imgui.cpp
        /home/taha/external/imgui/imgui_demo.cpp
       /home/taha/external//imgui/imgui_draw.cpp
       /home/taha/external//imgui/imgui_tables.cpp
       /home/taha/external//imgui/imgui_widgets.cpp
       /home/taha/external//imgui/backends/imgui_impl_glfw.cpp
       /home/taha/external//imgui/backends/imgui_impl_vulkan.cpp
        external/entt.hpp
        src/lve_light.cpp
        src/lve_light.h
        src/lve_shadow_renderer.cpp
        src/lve_shadow_renderer.h
)

# Collect all sources for the library (excluding main.cpp)
file(GLOB_RECURSE MYLIB_SOURCES src/*.cpp)

# Include directories
include_directories(
        /home/taha/external/imgui
        /home/taha/external/imgui/backends
)

# Create a static library from all other cpp files
add_library(mylib STATIC ${MYLIB_SOURCES}
        src/Mesh.cpp
        src/Mesh.h
        src/lve_camera.cpp
        src/lve_camera.hpp
        src/Texture.cpp
        src/Texture.h
        external/stb_image.h
        src/lve_frame_buffer.cpp
        src/lve_frame_buffer.h
        src/testFrameBuffers.h
        src/lve_shadowMap.cpp
        src/lve_shadowMap.h
        external/entt.hpp
        src/lve_light.cpp
        src/lve_light.h)


# Include headers
target_include_directories(mylib PUBLIC src)
target_include_directories(mylib PUBLIC external/tinyobjloader)
target_include_directories(mylib PUBLIC external)
target_include_directories(mylib PUBLIC ${SHADERC_INCLUDE_DIRS})


find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)


add_executable(untitled4 src/main.cpp
        src/Texture.cpp
        src/Texture.h
        src/testFrameBuffers.h
        ${IMGUI_SOURCES}
        external/entt.hpp
        src/lve_light.cpp
        src/lve_light.h
)

find_library(SHADERC_LIB shaderc REQUIRED)

target_link_libraries(untitled4 PRIVATE
        mylib
        Vulkan::Vulkan
        glfw
        ${SHADERC_LIB}
)


# Link the library
target_link_libraries(untitled4 PRIVATE mylib)
