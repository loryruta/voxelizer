#pragma once

#include "gl.hpp"
#include "octree.hpp"
#include "voxel_list.hpp"

namespace voxelizer
{
	class octree_builder
	{
	private:
		program m_node_flag;
		program m_node_alloc;
		program m_node_init;
		program m_store_leaf;

	public:
		octree_builder();

		void clear(
			voxelizer::octree const& octree,
			uint32_t start,
			uint32_t count
		);

		void build(
			voxelizer::voxel_list const& voxel_list,
			uint32_t resolution,
			GLuint buffer,
			size_t offset,
			octree& result
		);
	};
}
