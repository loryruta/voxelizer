set(SRC
	common/util/camera.cpp
	common/util/camera.hpp
	common/util/gl.cpp
	common/util/gl.hpp
	common/util/radix_sort.hpp
	common/util/render_doc.hpp
	common/util/render_doc.cpp
	common/util/screen_quad.cpp
	common/util/screen_quad.hpp
	common/util/std_integration.hpp
	common/octree.cpp
	common/octree.hpp
)

add_library(common ${SRC})

# ------------------------------------------------------------------------------------------------
# Dependencies
# ------------------------------------------------------------------------------------------------

# GLM
find_package(glm CONFIG REQUIRED)
target_link_libraries(common PUBLIC glm::glm)

# glad
target_sources(common
    PUBLIC
	${CMAKE_SOURCE_DIR}/third_party/glad/src/glad.c
	${CMAKE_SOURCE_DIR}/third_party/glad/include/glad/glad.h
	${CMAKE_SOURCE_DIR}/third_party/glad/include/KHR/khrplatform.h
)
target_include_directories(common PUBLIC ${CMAKE_SOURCE_DIR}/third_party/glad/include)

# renderdoc
target_include_directories(common PUBLIC ${CMAKE_SOURCE_DIR}/third_party/renderdoc)
