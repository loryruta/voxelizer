
set(SRC
	viewer/main.cpp
	viewer/octree_tracer.cpp
	viewer/octree_tracer.hpp
	viewer/scene_renderer.hpp
	viewer/scene_renderer.cpp
	viewer/camera.cpp
	viewer/camera.hpp
)

add_executable(viewer_exec ${SRC})

# ------------------------------------------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------------------------------------------

# Self
target_include_directories(viewer_exec PUBLIC "${CMAKE_SOURCE_DIR}/viewer")

# voxelizer
target_link_libraries(viewer_exec PUBLIC voxelizer)

# GLFW
find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(viewer_exec PUBLIC glfw)

# ------------------------------------------------------------------------------------------------
# Embed resources
# ------------------------------------------------------------------------------------------------

shinji_embed(viewer_exec "voxelizer::viewer"
	resources/shaders/scene.frag
	resources/shaders/scene.vert
	resources/shaders/screen_quad.vert
	resources/shaders/svo_tracer.frag
)
shinji_finalize(viewer_exec "voxelizer::viewer")
