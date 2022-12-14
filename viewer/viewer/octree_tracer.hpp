#pragma once

#include <glm/glm.hpp>

#include <voxelizer/octree.hpp>
#include <voxelizer/gl.hpp>

namespace voxelizer
{
	class octree_tracer
	{
	private:
		program m_program;
		screen_quad m_screen_quad;

	public:
		octree_tracer();

		void render(
			glm::uvec2 const& screen,

			glm::vec3 const& position,
			glm::vec3 const& size,

			glm::mat4 const& camera_projection,
			glm::mat4 const& camera_view,
			glm::vec3 const& camera_position,

			GLuint octree_buffer,
			GLintptr octree_buffer_offset,
			GLsizeiptr octree_buffer_size,

			uint32_t starting_node_address = 0
		);
	};
}
