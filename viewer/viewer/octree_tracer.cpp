#include "octree_tracer.hpp"

#include <iostream>

#include <glm/gtc/type_ptr.hpp>

#include <shinji.hpp>

// ------------------------------------------------------------------------------------------------ octree_tracer

voxelizer::octree_tracer::octree_tracer()
{
	// Program
	Shader screen_quad(GL_VERTEX_SHADER);
	screen_quad.source_from_string(shinji::load_resource_from_bundle("resources/shaders/screen_quad.vert").first);
	screen_quad.compile();

	m_program.attach_shader(screen_quad);

	Shader svo_tracer(GL_FRAGMENT_SHADER);
	svo_tracer.source_from_string(shinji::load_resource_from_bundle("resources/shaders/svo_tracer.frag").first);
	svo_tracer.compile();

	m_program.attach_shader(svo_tracer);

	m_program.link();
}

void voxelizer::octree_tracer::render(
	glm::uvec2 const& screen,

	glm::vec3 const& position,
	glm::vec3 const& size,

	glm::mat4 const& camera_projection,
	glm::mat4 const& camera_view,
	glm::vec3 const& camera_position,

	GLuint octree_buffer,
	GLintptr octree_buffer_offset,
	GLsizeiptr octree_buffer_size,

	uint32_t starting_node_address
)
{
	m_program.use();

	glUniform3fv(m_program.get_uniform_location("u_octree_from"), 1, glm::value_ptr(position));
	glUniform3fv(m_program.get_uniform_location("u_octree_size"), 1, glm::value_ptr(size));

	glUniform2uiv(m_program.get_uniform_location("u_screen"), 1, glm::value_ptr(screen));

	// camera
	glUniformMatrix4fv(m_program.get_uniform_location("u_projection"), 1, GL_FALSE, glm::value_ptr(camera_projection));
	glUniformMatrix4fv(m_program.get_uniform_location("u_view"), 1, GL_FALSE, glm::value_ptr(camera_view));
	glUniform3fv(m_program.get_uniform_location("u_position"), 1, glm::value_ptr(camera_position));

	glUniform1ui(m_program.get_uniform_location("u_start_address"), starting_node_address);

	glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 0, octree_buffer, octree_buffer_offset, octree_buffer_size);

	m_screen_quad.render();

	Program::unuse();
}
