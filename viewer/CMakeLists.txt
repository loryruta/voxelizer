
set(SRC
	viewer/main.cpp
	viewer/octree_tracer.cpp
	viewer/octree_tracer.hpp
)

add_executable(viewer ${SRC})

# ------------------------------------------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------------------------------------------

# common
target_include_directories(viewer PUBLIC "${CMAKE_SOURCE_DIR}/common")
target_link_libraries(viewer PUBLIC common)

# GLFW
find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(viewer PUBLIC glfw)

# ------------------------------------------------------------------------------------------------
# Embed resources
# ------------------------------------------------------------------------------------------------

shinji_embed(viewer
	resources/shaders/screen_quad.vert
	resources/shaders/svo_tracer.frag
)
shinji_finalize(viewer)
