cmake_minimum_required(VERSION 4.0)
project(untitled4)

set(CMAKE_CXX_STANDARD 20)

# Collect all sources for the library (excluding main.cpp)
file(GLOB_RECURSE MYLIB_SOURCES src/*.cpp)

# Create a static library from all other cpp files
add_library(mylib STATIC ${MYLIB_SOURCES}
        src/Mesh.cpp
        src/Mesh.h
        src/lve_camera.cpp
        src/lve_camera.hpp)

# Include headers
target_include_directories(mylib PUBLIC src)
target_include_directories(mylib PUBLIC external/tinyobjloader)
target_include_directories(mylib PUBLIC ${SHADERC_INCLUDE_DIRS})

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)


add_executable(untitled4 src/main.cpp)

find_library(SHADERC_LIB shaderc REQUIRED)

target_link_libraries(untitled4 PRIVATE
        mylib
        Vulkan::Vulkan
        glfw
        ${SHADERC_LIB}
)


# Link the library
target_link_libraries(untitled4 PRIVATE mylib)

