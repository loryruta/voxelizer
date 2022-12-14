
set(SRC
	voxelizer/gl.cpp
	voxelizer/gl.hpp
	voxelizer/render_doc.cpp
	voxelizer/render_doc.hpp
	voxelizer/ai_scene_loader.cpp
	voxelizer/ai_scene_loader.hpp
	voxelizer/octree.cpp
	voxelizer/octree.hpp
	voxelizer/octree_builder.cpp
	voxelizer/octree_builder.hpp
	voxelizer/scene.cpp
	voxelizer/scene.hpp
	voxelizer/voxel_list.cpp
	voxelizer/voxel_list.hpp
	voxelizer/voxelize.cpp
	voxelizer/voxelize.hpp
)

add_library(voxelizer ${SRC})

# ------------------------------------------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------------------------------------------

# Self
target_include_directories(voxelizer PUBLIC "${CMAKE_SOURCE_DIR}/voxelizer")

# GLM
find_package(glm CONFIG REQUIRED)
target_link_libraries(voxelizer PUBLIC glm::glm)

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

shinji_embed(voxelizer "voxelizer"
	resources/shaders/svo_node_alloc.comp
	resources/shaders/svo_node_flag.comp
	resources/shaders/svo_node_init.comp
	resources/shaders/svo_store_leaf.comp
	resources/shaders/svo_tracer.frag
	resources/shaders/voxelize.frag
	resources/shaders/voxelize.geom
	resources/shaders/voxelize.vert
)
shinji_finalize(voxelizer "voxelizer")

# ------------------------------------------------------------------------------------------------
# Executable
# ------------------------------------------------------------------------------------------------

add_executable(voxelizer_exec voxelizer/main.cpp)

target_link_libraries(voxelizer_exec PRIVATE voxelizer)
