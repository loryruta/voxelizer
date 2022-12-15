cmake_minimum_required (VERSION 3.20)
project (3d-rasterizer)

set(CMAKE_CXX_STANDARD 17)

# shinji
message(STATUS "Loading shinji...")
include("${CMAKE_CURRENT_SOURCE_DIR}/shinji.cmake")

add_subdirectory(voxelizer)
add_subdirectory(viewer)
