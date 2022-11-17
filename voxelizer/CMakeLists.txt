
set(SRC
	src/util/camera.cpp
	src/util/camera.hpp
	src/util/gl.cpp
	src/util/gl.hpp
	src/util/radix_sort.hpp
	src/util/render_doc.hpp
	src/util/render_doc.cpp
	src/util/screen_quad.cpp
	src/util/screen_quad.hpp
	src/util/std_integration.hpp
	src/ai_scene_loader.cpp
	src/ai_scene_loader.hpp
	src/main.cpp
	src/mesh.cpp
	src/mesh.hpp
	src/octree.cpp
	src/octree.hpp
	src/scene.hpp
	src/voxelize.cpp
	src/voxelize.hpp
	src/voxel_list.cpp
	src/voxel_list.hpp
)

add_executable(voxelizer ${SRC})

# ------------------------------------------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------------------------------------------

# assimp
find_package(assimp CONFIG REQUIRED)
target_link_libraries(voxelizer PUBLIC assimp::assimp)

# GLM
find_package(glm CONFIG REQUIRED)
target_link_libraries(voxelizer PUBLIC glm::glm)

# libzip
find_package(libzip CONFIG REQUIRED)
target_link_libraries(voxelizer PUBLIC libzip::zip)

# rapidjson
find_package(rapidjson CONFIG REQUIRED)
target_link_libraries(voxelizer PUBLIC rapidjson)

# stb
find_package(Stb REQUIRED)
#target_link_libraries(voxelizer INTERFACE Stb)

# GLFW
find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(voxelizer PUBLIC glfw)

# glad
target_sources(voxelizer
    PUBLIC
	${CMAKE_SOURCE_DIR}/third_party/glad/src/glad.c
	${CMAKE_SOURCE_DIR}/third_party/glad/include/glad/glad.h
	${CMAKE_SOURCE_DIR}/third_party/glad/include/KHR/khrplatform.h
)
target_include_directories(voxelizer PUBLIC ${CMAKE_SOURCE_DIR}/third_party/glad/include)

# renderdoc
target_include_directories(voxelizer PUBLIC ${CMAKE_SOURCE_DIR}/third_party/renderdoc)

# ------------------------------------------------------------------------------------------------
# Embed resources
# ------------------------------------------------------------------------------------------------

# shinji
include(FetchContent)
message(STATUS "Fetching shinji...")
FetchContent_Declare(
    shinji
    GIT_REPOSITORY "https://github.com/loryruta/shinji"
)
FetchContent_MakeAvailable(shinji)
include("${shinji_SOURCE_DIR}/shinji.cmake")

shinji_embed(voxelizer
	resources/shaders/phong.frag
	resources/shaders/scene.vert
	resources/shaders/screen_quad.vert
	resources/shaders/svo_node_alloc.comp
	resources/shaders/svo_node_flag.comp
	resources/shaders/svo_node_init.comp
	resources/shaders/svo_store_leaf.comp
	resources/shaders/svo_tracer.frag
	resources/shaders/voxelize.frag
	resources/shaders/voxelize.geom
	resources/shaders/voxelize.vert
)
shinji_finalize(voxelizer)
