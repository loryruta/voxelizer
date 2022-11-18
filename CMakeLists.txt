cmake_minimum_required (VERSION 3.20)
project (3d-rasterizer)

set(CMAKE_CXX_STANDARD 17)

# shinji
include(FetchContent)
message(STATUS "Fetching shinji...")
FetchContent_Declare(
    shinji
    GIT_REPOSITORY "https://github.com/loryruta/shinji"
)
FetchContent_MakeAvailable(shinji)
include("${shinji_SOURCE_DIR}/shinji.cmake")

add_subdirectory(common)
add_subdirectory(voxelizer)
add_subdirectory(viewer)
