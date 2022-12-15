#include "voxelize.hpp"

#include <iostream>

#include <glm/gtc/type_ptr.hpp>
#include <shinji.hpp>

#include "render_doc.hpp"
#include "voxel_list.hpp"

voxelizer::voxelize::voxelize()
{
	// Program
	shader vertex(GL_VERTEX_SHADER);
	vertex.source_from_string(shinji::load_resource_from_bundle("resources/shaders/voxelize.vert").m_data);
	vertex.compile();
	m_program.attach_shader(vertex);

	shader geometry(GL_GEOMETRY_SHADER);
	geometry.source_from_string(shinji::load_resource_from_bundle("resources/shaders/voxelize.geom").m_data);
	geometry.compile();
	m_program.attach_shader(geometry);

	shader fragment(GL_FRAGMENT_SHADER);
	fragment.source_from_string(shinji::load_resource_from_bundle("resources/shaders/voxelize.frag").m_data);
	fragment.compile();
	m_program.attach_shader(fragment);

	m_program.link();

	// Errors counter
	glGenBuffers(1, &m_errors_counter);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_errors_counter);
	glBufferStorage(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), nullptr, GL_DYNAMIC_STORAGE_BIT);
}

glm::uvec3 voxelizer::voxelize::calc_proportional_grid(glm::vec3 size, uint32_t voxels_on_y)
{
	glm::uvec3 grid;
	grid.y = voxels_on_y;
	grid.x = (uint32_t) glm::ceil((float(voxels_on_y) / size.y) * size.x);
	grid.z = (uint32_t) glm::ceil((float(voxels_on_y) / size.y) * size.z);
	return grid;
}

glm::mat4 voxelizer::voxelize::create_scene_normalization_matrix(glm::vec3 area_position, glm::vec3 area_size)
{
	auto result = glm::identity<glm::mat4>();
	//result = glm::scale(result, glm::vec3(1.0f) / area_size);
	float max_side = glm::max(area_size.x, glm::max(area_size.y, area_size.z));
	result = glm::scale(result, glm::vec3(1.0f / max_side));
	result = glm::translate(result, -area_position);
	return result;
}

void voxelizer::voxelize::create_projection_matrices(glm::mat4 m[3])
{
	glm::mat4 ortho = glm::ortho(0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 2.0f);
	m[0] = ortho * glm::lookAt(glm::vec3(-1.0f, 0, 0), glm::vec3(0), glm::vec3(0, 1.0f, 0));
	m[1] = ortho * glm::lookAt(glm::vec3(0, -1.0f, 0), glm::vec3(0), glm::vec3(0, 0, 1.0f));
	m[2] = ortho * glm::lookAt(glm::vec3(0, 0, +2.0f), glm::vec3(0), glm::vec3(0, 1.0f, 0));
}

void voxelizer::voxelize::invoke(voxelizer::scene const& scene)
{
	size_t voxel_list_offset = 0;

	for (uint32_t mesh_idx = 0; mesh_idx < scene.m_meshes.size(); mesh_idx++)
	{
		voxelizer::mesh const& mesh = scene.m_meshes[mesh_idx];

		// Transform
		glUniformMatrix4fv(m_program.get_uniform_location("u_mesh_transform"), 1, GL_FALSE, glm::value_ptr(mesh.m_transform));

		// Color
		glm::vec4 color = mesh.m_material->get_color(material::type::DIFFUSE);
		glUniform4fv(m_program.get_uniform_location("u_color"), 1, glm::value_ptr(color));

		GLuint texture = mesh.m_material->get_texture(material::type::DIFFUSE);
		glBindTexture(GL_TEXTURE_2D, texture);

		glBindVertexArray(mesh.m_vao);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.m_ebo);

		// Reset errors counter
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_errors_counter);
		glClearBufferSubData(GL_ATOMIC_COUNTER_BUFFER, GL_R32UI, 0, sizeof(GLuint), GL_RED, GL_UNSIGNED_INT, nullptr);

		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 4, m_errors_counter);

		voxelizer::renderdoc::watch(true, [&]
		{
			glDrawElements(GL_TRIANGLES, (GLsizei) mesh.m_element_count, GL_UNSIGNED_INT, nullptr);
		});

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT);

		size_t current_voxel_list_offset = m_atomic_counter.get_value();

		printf("[voxelize] Mesh %d voxelized, voxels: %zu, offset: %zu\n",
			mesh_idx,
			(current_voxel_list_offset - voxel_list_offset),
			current_voxel_list_offset
		);

		voxel_list_offset = current_voxel_list_offset;

		GLuint errors_count{};
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_errors_counter);
		glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &errors_count);

		if (errors_count > 0)
		{
			fprintf(stderr, "[voxelize] Errors: %d\n", errors_count);
			fflush(stderr);
		}
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void voxelizer::voxelize::operator()(
	voxelizer::voxel_list& voxel_list,
	voxelizer::scene const& scene,
	uint32_t voxels_on_y,
	glm::vec3 area_position,
	glm::vec3 area_size
)
{
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_CULL_FACE);

	m_program.use();

	// Prepares a matrix responsible of framing the model in the said area.
	// The min point will correspond to (0, 0, 0) while the max point to (1, 1, 1).

	glm::mat4 scene_norm_mtx = voxelizer::voxelize::create_scene_normalization_matrix(area_position, area_size);
	glUniformMatrix4fv(m_program.get_uniform_location("u_transform"), 1, GL_FALSE, glm::value_ptr(scene_norm_mtx));

	// Creates the matrices that will project the triangles to the plane that gives
	// out their widest area (to achieve Conservative Rasterization).

	glm::mat4 proj[3];
	create_projection_matrices(proj);

	glUniformMatrix4fv(m_program.get_uniform_location("u_x_ortho_projection"), 1, GL_FALSE, glm::value_ptr(proj[0]));
	glUniformMatrix4fv(m_program.get_uniform_location("u_y_ortho_projection"), 1, GL_FALSE, glm::value_ptr(proj[1]));
	glUniformMatrix4fv(m_program.get_uniform_location("u_z_ortho_projection"), 1, GL_FALSE, glm::value_ptr(proj[2]));

	// Finds out the size of the grid given just the number of voxels along the Y axis,
	// other dimensions are found proportionally to that.

	glm::uvec3 grid = voxelizer::voxelize::calc_proportional_grid(area_size, voxels_on_y);

	uint32_t max_side = glm::max(grid.x, glm::max(grid.y, grid.z)); // The viewport is always a square, its side is the max side of the grid.
	glUniform1ui(m_program.get_uniform_location("u_viewport"), max_side);

	glUniform3uiv(m_program.get_uniform_location("u_grid"), 1, glm::value_ptr(grid));

	printf("[voxelize] Viewport of size (%d, %d)\n", max_side, max_side);

	GLuint framebuffer{};
	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_WIDTH, max_side);
	glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_HEIGHT, max_side);

	glViewport(0, 0, (GLsizei) max_side, (GLsizei) max_side);

	// COUNT
	// Runs the program and counts how many voxels are generated in order to allocate the buffer first
	// and then fill it with the voxels.

	glUniform1ui(m_program.get_uniform_location("u_can_store"), 0);

	m_atomic_counter.set_value(0);
	m_atomic_counter.bind(3);

	invoke(scene);

	GLuint voxel_count = m_atomic_counter.get_value();

	printf("[voxelize] Allocating a voxel-list of %d (~%zu bytes)\n", voxel_count, voxel_count * sizeof(GLuint) * 2);

	voxel_list.alloc(voxel_count);

	// STORE
	// Now we can actually store the voxel list inside of the just-allocated buffer.

	glUniform1ui(m_program.get_uniform_location("u_can_store"), 1);

	m_atomic_counter.set_value(0);
	m_atomic_counter.bind(3);

	voxel_list.bind(1, 2);

	invoke(scene);

	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	printf("[voxelize] Voxel-list stored\n");

	//

	voxelizer::program::unuse();

	glDeleteFramebuffers(1, &framebuffer);
}
