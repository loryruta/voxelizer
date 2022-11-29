#include "octree.hpp"

#include <iostream>
#include <stdexcept>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "util/render_doc.hpp"

// ------------------------------------------------------------------------------------------------ octree

//this->m_buffer.load_data(this->capacity, NULL, GL_DYNAMIC_DRAW);
//this->m_buffer.set_format(GL_R32UI);

bool voxelizer::octree::is_valid() const
{
	return m_buffer != NULL;
}

size_t voxelizer::octree::get_size() const
{
	return voxelizer::octree::get_octree_size(m_resolution);
}

size_t voxelizer::octree::get_bytesize() const
{
	return voxelizer::octree::get_octree_bytesize(m_resolution);
}

uint32_t voxelizer::octree::get_suitable_resolution_for(glm::vec3 grid)
{
	float max_side = glm::max(glm::max(grid.x, grid.y), grid.z);
	return (uint32_t) glm::ceil(glm::log2(max_side));
}

uint32_t voxelizer::octree::get_octree_side(uint32_t resolution)
{
	return (uint32_t) glm::exp2(resolution);
}

constexpr size_t voxelizer::octree::get_octree_size(uint32_t resolution)
{
	size_t result = 0;
	for (int level = 1; level <= resolution; level++) {
		result += (size_t) glm::exp2(3 * level);
	}
	return result;
}

constexpr size_t voxelizer::octree::get_octree_bytesize(uint32_t resolution)
{
	return get_octree_size(resolution) * sizeof(GLuint);
}

bool voxelizer::octree::is_null(uint32_t raw_val)
{
	return raw_val == 0;
}

bool voxelizer::octree::is_leaf(uint32_t raw_val)
{
	return (raw_val & 0x80000000) == 0;
}

bool voxelizer::octree::is_address(uint32_t raw_val)
{
	return !is_leaf(raw_val);
}

uint32_t voxelizer::octree::get_value(uint32_t raw_val)
{
	return raw_val & 0x7fffffff;
}

glm::uvec3 voxelizer::octree::get_voxel_position(uint32_t morton)
{
	glm::uvec3 result(0);

	int pos = 0;
	while (morton != 0)
	{
		result.x |= (morton & 1) << pos; morton >>= 1;
		result.y |= (morton & 1) << pos; morton >>= 1;
		result.z |= (morton & 1) << pos; morton >>= 1;

		pos++;
	}

	return result;
}

void voxelizer::octree::traverse_r(GLuint const* octree, size_t offset, uint32_t depth, voxelizer::octree::on_leaf_t const& on_leaf, uint32_t parent_morton, uint32_t stop_at_lvl)
{
	for (int i = 0; i < 8; i++)
	{
		uint32_t node_idx = offset + i;
		GLuint raw_val = octree[node_idx];
		if (voxelizer::octree::is_null(raw_val)) {
			continue;
		}

		uint32_t morton = parent_morton | (i << (3 * (depth - 1)));
		if (voxelizer::octree::is_leaf(raw_val) || depth == stop_at_lvl) {
			on_leaf(morton, node_idx);
		} else {
			if (voxelizer::octree::is_address(raw_val)) {
				uint32_t addr = voxelizer::octree::get_value(raw_val);
				traverse_r(octree, addr, depth + 1, on_leaf, morton, stop_at_lvl);
			}
		}
	}
}

void voxelizer::octree::traverse(GLuint const* octree, voxelizer::octree::on_leaf_t const& on_leaf, uint32_t stop_at_lvl)
{
	traverse_r(octree, 0, 1, on_leaf, 0, stop_at_lvl);
}

// --------------------------------------------------------------------------------------------------------------------------------
// octree_traverser
// --------------------------------------------------------------------------------------------------------------------------------

voxelizer::octree_traverser::octree_traverser(octree_data_const_t* octree, size_t init_node_address)
{
	m_octree = octree;

	m_current.m_node_address = init_node_address;
	m_current.m_child_num = 0;
	m_current.m_morton_code = 0;

	m_finished = false;
}

bool voxelizer::octree_traverser::has_finished()
{
	return m_finished;
}

voxelizer::octree_traverser::value voxelizer::octree_traverser::next(uint32_t stop_at_level)
{
	while (true)
	{
		while (m_current.m_child_num >= 8)
		{
			if (m_stack.empty()) {
				m_finished = true;
				return {}; // Finished
			}
			m_current = m_stack.top();
			m_stack.pop();

			m_current.m_child_num++;
		}

		m_current.m_morton_code = (m_current.m_morton_code & (~7)) | m_current.m_child_num;

		uint32_t raw_val = m_octree[m_current.m_node_address + m_current.m_child_num];
		uint32_t val = voxelizer::octree::get_value(raw_val);

		bool is_null = voxelizer::octree::is_null(raw_val);
		bool is_leaf = voxelizer::octree::is_leaf(raw_val);
		bool has_reached_stop = (m_stack.size() + 1) == stop_at_level;

		auto result = m_current;

		if (is_null || is_leaf || has_reached_stop)
		{
			m_current.m_child_num++;
		}
		else if (voxelizer::octree::is_address(raw_val))
		{
			m_stack.push(m_current);

			m_current.m_node_address = val;
			m_current.m_child_num = 0;
			m_current.m_morton_code <<= 3;

			continue;
		}

		if ((is_leaf || has_reached_stop) && !is_null)
		{
			return result;
		}
	}
}
