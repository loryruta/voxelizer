#include "scene_renderer.hpp"

#include <sstream>

#include <glm/gtc/type_ptr.hpp>

#include <shinji.hpp>

voxelizer::scene_renderer::scene_renderer()
{
	shader vert_shader(GL_VERTEX_SHADER);
	vert_shader.source_from_string(viewer::shinji::load_resource_from_bundle("resources/shaders/scene.vert").m_data);
	vert_shader.compile();

	m_program.attach_shader(vert_shader);

	shader frag_shader(GL_FRAGMENT_SHADER);
	frag_shader.source_from_string(viewer::shinji::load_resource_from_bundle("resources/shaders/scene.frag").m_data);
	frag_shader.compile();

	m_program.attach_shader(frag_shader);

	m_program.link();
}

void voxelizer::scene_renderer::scene_renderer::render(
	glm::mat4 const& camera_projection,
	glm::mat4 const& camera_view,
	glm::mat4 const& transform,
	voxelizer::scene const& scene
)
{
	glEnable(GL_DEPTH_TEST);

	m_program.use();

	glUniformMatrix4fv(m_program.get_uniform_location("u_camera_projection"), 1, GL_FALSE, glm::value_ptr(camera_projection));
	glUniformMatrix4fv(m_program.get_uniform_location("u_camera_view"), 1, GL_FALSE, glm::value_ptr(camera_view));

	glUniformMatrix4fv(m_program.get_uniform_location("u_scene_transform"), 1, GL_FALSE, glm::value_ptr(transform));

	for (auto& mesh : scene.m_meshes)
	{
		glUniformMatrix4fv(m_program.get_uniform_location("u_transform"), 1, GL_FALSE, glm::value_ptr(mesh.m_transform));

		glUniform4fv(m_program.get_uniform_location("u_color"), 1, glm::value_ptr(mesh.m_material->get_color(m_view_type)));
		glBindTexture(GL_TEXTURE_2D, mesh.m_material->get_texture(m_view_type));

		glBindVertexArray(mesh.m_vao);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.m_ebo);

		glDrawElements(GL_TRIANGLES, mesh.m_element_count, GL_UNSIGNED_INT, NULL);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
}
