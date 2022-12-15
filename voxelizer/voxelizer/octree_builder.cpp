#include "octree_builder.hpp"

#include <iostream>

#include <shinji.hpp>

#include "render_doc.hpp"

voxelizer::octree_builder::octree_builder()
{
	// node_flag
	{
		shader shader(GL_COMPUTE_SHADER);
		shader.source_from_string(shinji::load_resource_from_bundle("resources/shaders/svo_node_flag.comp").m_data);
		shader.compile();

		m_node_flag.attach_shader(shader);
		m_node_flag.link();
	}

	// node_alloc
	{
		shader shader(GL_COMPUTE_SHADER);
		shader.source_from_string(shinji::load_resource_from_bundle("resources/shaders/svo_node_alloc.comp").m_data);
		shader.compile();

		m_node_alloc.attach_shader(shader);
		m_node_alloc.link();
	}

	// node_init
	{
		shader shader(GL_COMPUTE_SHADER);
		shader.source_from_string(shinji::load_resource_from_bundle("resources/shaders/svo_node_init.comp").m_data);
		shader.compile();

		m_node_init.attach_shader(shader);
		m_node_init.link();
	}

	// store_leaf
	{
		shader shader(GL_COMPUTE_SHADER);
		shader.source_from_string(shinji::load_resource_from_bundle("resources/shaders/svo_store_leaf.comp").m_data);
		shader.compile();

		m_store_leaf.attach_shader(shader);
		m_store_leaf.link();
	}
}

void voxelizer::octree_builder::clear(voxelizer::octree const& octree, uint32_t start, uint32_t count)
{
	printf("[octree_builder] Clearing - start: %d, count: %d\n", start, count);

	m_node_init.use();

	glUniform1ui(m_node_init.get_uniform_location("u_start"), start);
	glUniform1ui(m_node_init.get_uniform_location("u_count"), count);

	glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 1, octree.m_buffer, (GLintptr)octree.m_offset, (GLintptr)octree.get_bytesize());

	voxelizer::renderdoc::watch(false, [&]
	{
		glDispatchCompute(glm::ceil(count / float(32)), 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	});

	program::unuse();
}

void voxelizer::octree_builder::build(
	voxelizer::voxel_list const& voxel_list,
	uint32_t resolution,
	GLuint buffer,
	size_t offset,
	voxelizer::octree& octree
)
{
	octree.m_buffer = buffer;
	octree.m_offset = offset;
	octree.m_resolution = resolution;

	atomic_counter alloc_counter{};

	unsigned int start = 0, count = 8;
	unsigned int alloc_start = start + count;

	// node init
	clear(octree, start, count);

	for (int level = 1; level < resolution; level++)
	{
		printf("[octree_builder] Level: %d\n", level);

		// node flag
		m_node_flag.use();

		glUniform1i(m_node_flag.get_uniform_location("u_max_level"), (int)octree.m_resolution);
		glUniform1i(m_node_flag.get_uniform_location("u_level"), level);

		glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 1, octree.m_buffer, (GLintptr)octree.m_offset, (GLintptr)octree.get_bytesize());
		voxel_list.bind(2, 3);

		glDispatchCompute(glm::ceil(float(voxel_list.m_size) / float(32)), 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		program::unuse();

		printf("[octree_builder] Flagging - max_level: %d, level: %d, octree offset: %zu, octree size: %zu\n", octree.m_resolution, level, octree.m_offset, octree.get_bytesize());

		// node alloc
		m_node_alloc.use();

		glUniform1ui(m_node_alloc.get_uniform_location("u_start"), start);
		glUniform1ui(m_node_alloc.get_uniform_location("u_count"), count);
		glUniform1ui(m_node_alloc.get_uniform_location("u_alloc_start"), alloc_start);

		glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 1, octree.m_buffer, (GLintptr)octree.m_offset, (GLintptr)octree.get_bytesize());
		alloc_counter.bind(2);

		alloc_counter.set_value(0);

		renderdoc::watch(false, [&]
		{
			glDispatchCompute(glm::ceil(count / float(32)), 1, 1);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_ATOMIC_COUNTER_BARRIER_BIT);
		});

		program::unuse();

		GLuint alloc_count = alloc_counter.get_value();
		start = alloc_start;
		count = alloc_count * 8;
		alloc_start = start + count;

		printf("[octree_builder] Node alloc - start: %d, count: %d, alloc_start: %d, octree offset: %zu, octree size: %zu\n",
			start,
			count,
			alloc_start,
			octree.m_offset,
			octree.get_bytesize()
		);

		// node init
		clear(octree, start, count);
	}

	// store leaf
	m_store_leaf.use();

	glUniform1i(m_store_leaf.get_uniform_location("u_max_level"), octree.m_resolution);

	glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 1, octree.m_buffer, (GLintptr)octree.m_offset, (GLintptr)octree.get_bytesize());
	voxel_list.bind(2, 3);

	int workgroup_count = glm::ceil(voxel_list.m_size / float(32));

	renderdoc::watch(true, [&] {
		glDispatchCompute(workgroup_count, 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	});


	printf("[octree_builder] Store leaves - max_level: %d, octree offset: %zu, octree size: %zu\n",
		octree.m_resolution,
		octree.m_offset,
		octree.get_bytesize()
	);

	program::unuse();
}
