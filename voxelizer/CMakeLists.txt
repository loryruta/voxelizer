
set(SRC
	voxelizer/ai_scene_loader.cpp
	voxelizer/ai_scene_loader.hpp
	voxelizer/main.cpp
	voxelizer/mesh.cpp
	voxelizer/mesh.hpp
	voxelizer/octree_builder.cpp
	voxelizer/octree_builder.hpp
	voxelizer/scene.hpp
	voxelizer/voxelize.cpp
	voxelizer/voxelize.hpp
	voxelizer/voxel_list.cpp
	voxelizer/voxel_list.hpp
)

add_executable(voxelizer ${SRC})

# ------------------------------------------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------------------------------------------

# common
target_include_directories(voxelizer PUBLIC "${CMAKE_SOURCE_DIR}/common")
target_link_libraries(voxelizer PUBLIC common)

# assimp
find_package(assimp CONFIG REQUIRED)
target_link_libraries(voxelizer PUBLIC assimp::assimp)

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

# ------------------------------------------------------------------------------------------------
# Embed resources
# ------------------------------------------------------------------------------------------------

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
