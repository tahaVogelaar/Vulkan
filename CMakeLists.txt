cmake_minimum_required(VERSION 3.20)
project(untitled4)
set(CMAKE_CXX_STANDARD 20)

# Dependencies
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_library(SHADERC_LIB shaderc REQUIRED)

# ImGui sources
set(IMGUI_SOURCES
        external/imgui/imgui.cpp
        external/imgui/imgui_demo.cpp
        external/imgui/imgui_draw.cpp
        external/imgui/imgui_tables.cpp
        external/imgui/imgui_widgets.cpp
        external/imgui/backends/imgui_impl_glfw.cpp
        external/imgui/backends/imgui_impl_vulkan.cpp
)

# Engine library
file(GLOB_RECURSE ENGINE_SOURCES src/*.cpp)
list(REMOVE_ITEM ENGINE_SOURCES ${CMAKE_SOURCE_DIR}/src/main.cpp)

add_library(engine STATIC ${ENGINE_SOURCES}
        /home/taha/fastgltf/build/libfastgltf.a
)
target_include_directories(engine PUBLIC
        src
        external
        external/imgui
        external/imgui/backends
        external/assimp/include
        external/fastgltf/include
)
set(ASSIMP_LIB "${CMAKE_SOURCE_DIR}/external/assimp/lib/libassimp.a")
set(FASTGLTF_LIB "${CMAKE_SOURCE_DIR}/external/fastgltf/libfastgltf.a")

target_link_libraries(engine PRIVATE Vulkan::Vulkan glfw ${SHADERC_LIB} z dl pthread ${FASTGLTF_LIB})

# Executable
add_executable(untitled4 src/main.cpp ${IMGUI_SOURCES}
        external/json.hpp
        external/stb_image_write.h
)
target_link_libraries(untitled4 PRIVATE engine)
