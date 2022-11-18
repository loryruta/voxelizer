#include "octree_builder.hpp"

#include <iostream>

#include <shinji.hpp>

// ------------------------------------------------------------------------------------------------ octree_builder

voxelizer::octree_builder::octree_builder()
{
	// node_flag
	{
		Shader shader(GL_COMPUTE_SHADER);
		shader.source_from_string(shinji::load_resource_from_bundle("resources/shaders/svo_node_flag.comp").first);
		shader.compile();

		m_node_flag.attach_shader(shader);
		m_node_flag.link();
	}

	// node_alloc
	{
		Shader shader(GL_COMPUTE_SHADER);
		shader.source_from_string(shinji::load_resource_from_bundle("resources/shaders/svo_node_alloc.comp").first);
		shader.compile();

		m_node_alloc.attach_shader(shader);
		m_node_alloc.link();
	}

	// node_init
	{
		Shader shader(GL_COMPUTE_SHADER);
		shader.source_from_string(shinji::load_resource_from_bundle("resources/shaders/svo_node_init.comp").first);
		shader.compile();

		m_node_init.attach_shader(shader);
		m_node_init.link();
	}

	// store_leaf
	{
		Shader shader(GL_COMPUTE_SHADER);
		shader.source_from_string(shinji::load_resource_from_bundle("resources/shaders/svo_store_leaf.comp").first);
		shader.compile();

		m_store_leaf.attach_shader(shader);
		m_store_leaf.link();
	}
}

void voxelizer::octree_builder::clear(voxelizer::octree const& octree, uint32_t start, uint32_t count)
{
	m_node_init.use();

	glUniform1ui(m_node_init.get_uniform_location("u_start"), start);
	glUniform1ui(m_node_init.get_uniform_location("u_count"), count);

	glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 1, octree.m_buffer, (GLintptr)octree.m_offset, (GLintptr)octree.get_bytesize());

	//rgc::renderdoc::watch(false, [&] {
		glDispatchCompute(glm::ceil(count / float(32)), 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	//});

	Program::unuse();
}

void voxelizer::octree_builder::build(
	VoxelList const& voxel_list,
	uint32_t resolution,
	GLuint buffer,
	size_t offset,
	voxelizer::octree& octree
)
{
	octree.m_buffer = buffer;
	octree.m_offset = offset;
	octree.m_resolution = resolution;

	AtomicCounter alloc_counter;

	unsigned int start = 0, count = 8;
	unsigned int alloc_start = start + count;

	// node init
	clear(octree, start, count);

	for (int level = 1; level < resolution; level++)
	{
		// node flag
		m_node_flag.use();

		glUniform1i(m_node_flag.get_uniform_location("u_max_level"), (int)octree.m_resolution);
		glUniform1i(m_node_flag.get_uniform_location("u_level"), level);

		glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 1, octree.m_buffer, (GLintptr)octree.m_offset, (GLintptr)octree.get_bytesize());
		voxel_list.bind(2, 3);

		glDispatchCompute(glm::ceil(float(voxel_list.m_size) / float(32)), 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		Program::unuse();

		// node alloc
		m_node_alloc.use();

		glUniform1ui(m_node_alloc.get_uniform_location("u_start"), start);
		glUniform1ui(m_node_alloc.get_uniform_location("u_count"), count);
		glUniform1ui(m_node_alloc.get_uniform_location("u_alloc_start"), alloc_start);

		glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 1, octree.m_buffer, (GLintptr)octree.m_offset, (GLintptr)octree.get_bytesize());
		alloc_counter.bind(2);

		alloc_counter.set_value(0);

		//rgc::renderdoc::watch(false, [&] {
			glDispatchCompute(glm::ceil(count / float(32)), 1, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT);
		//});

		Program::unuse();

		GLuint alloc_count = alloc_counter.get_value();
		start = alloc_start;
		count = alloc_count * 8;
		alloc_start = start + count;

		// node init
		clear(octree, start, count);
	}

	// store leaf
	m_store_leaf.use();

	glUniform1i(m_store_leaf.get_uniform_location("u_max_level"), octree.m_resolution);

	glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 1, octree.m_buffer, (GLintptr)octree.m_offset, (GLintptr)octree.get_bytesize());
	voxel_list.bind(2, 3);

	int workgroup_count = glm::ceil(voxel_list.m_size / float(32));
	glDispatchCompute(workgroup_count, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	Program::unuse();
}
