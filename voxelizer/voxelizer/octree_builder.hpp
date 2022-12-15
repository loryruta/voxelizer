#pragma once

#include "util/gl.hpp"
#include "octree.hpp"
#include "voxel_list.hpp"

namespace voxelizer
{
	class octree_builder
	{
	private:
		Program m_node_flag;
		Program m_node_alloc;
		Program m_node_init;
		Program m_store_leaf;

	public:
		octree_builder();

		void clear(octree const& octree, uint32_t start, uint32_t count);
		void build(VoxelList const& voxel_list, uint32_t resolution, GLuint buffer, size_t offset, octree& result);
	};
}
