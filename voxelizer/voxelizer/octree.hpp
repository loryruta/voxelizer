#pragma once

#include <vector>
#include <memory>
#include <functional>
#include <stack>

#include <glm/glm.hpp>

#include "util/gl.hpp"
#include "util/screen_quad.hpp"

namespace voxelizer
{
	constexpr uint32_t get_morton_code_from_voxel_position(glm::uvec3 pos)
	{
		uint32_t morton = 0;

		while (pos.x != 0 || pos.y != 0 || pos.z != 0)
		{
			morton |= pos.x & 1; pos.x >>= 1; morton <<= 1;
			morton |= pos.y & 1; pos.y >>= 1; morton <<= 1;
			morton |= pos.z & 1; pos.z >>= 1; morton <<= 1;
		}

		return morton;
	}

	// ------------------------------------------------------------------------------------------------
	// octree
	// ------------------------------------------------------------------------------------------------

	struct octree
	{
		GLuint m_buffer = NULL;
		size_t m_offset;
		uint32_t m_resolution;

		bool is_valid() const;

		size_t get_size() const;
		size_t get_bytesize() const;

		static uint32_t get_suitable_resolution_for(glm::vec3 grid);
		static uint32_t get_octree_side(uint32_t resolution);
		static constexpr size_t get_octree_size(uint32_t resolution);
		static constexpr size_t get_octree_bytesize(uint32_t resolution);

		static bool is_null(uint32_t raw_val);
		static bool is_leaf(uint32_t raw_val);
		static bool is_address(uint32_t raw_val);
		static uint32_t get_value(uint32_t raw_val);

		static glm::uvec3 get_voxel_position(uint32_t morton);

		using on_leaf_t = std::function<void(uint32_t morton, uint32_t node_idx)>;
		static void traverse_r(GLuint const* octree, size_t offset, uint32_t depth, voxelizer::octree::on_leaf_t const& on_leaf, uint32_t parent_morton = 0, uint32_t stop_at_lvl = 0);
		static void traverse(GLuint const* octree, voxelizer::octree::on_leaf_t const& on_leaf, uint32_t stop_at_lvl = 0);
	};

	using octree_data_t = GLuint;
	using octree_data_const_t = GLuint const;

	// ------------------------------------------------------------------------------------------------
	// octree_struct
	// ------------------------------------------------------------------------------------------------

	template<typename _voxel>
	class octree_struct
	{
	protected:
		uint32_t m_resolution;
		std::vector<_voxel> m_data;

		uint32_t get_offset_from_morton_code(uint32_t morton)
		{
			uint32_t off = 0;
			for (int i = 0; i < m_resolution; i++)
			{
				off = m_data[off + (morton & 7)];
				morton >>= 3;
			}
			return off + (morton & 7);
		}

	public:
		explicit octree_struct(uint32_t resolution) :
			m_resolution(resolution),
			m_data(octree::get_octree_size(m_resolution))
		{}

		_voxel* get_data()
		{
			return m_data.data();
		}

		_voxel& get_voxel(glm::uvec3 const& pos) const
		{
			uint32_t morton = get_morton_code_from_voxel_position(pos);
			uint32_t off = get_offset_from_morton_code(morton);
			return m_data.at(off);
		}

		void set_voxel(glm::uvec3 const& pos, _voxel&& voxel)
		{
			uint32_t morton = get_morton_code_from_voxel_position(pos);
			uint32_t off = get_offset_from_morton_code(morton);
			m_data[off] = std::move(voxel);
		}
	};

	// ------------------------------------------------------------------------------------------------
	// octree_traverser
	// ------------------------------------------------------------------------------------------------

	class octree_traverser
	{
	public:
		struct value
		{
			uint32_t m_morton_code;  // The morton_code for the current node obtained during exploration.
			uint32_t m_node_address; // The address of the node within the octree buffer.
			uint32_t m_child_num;    // The index of the node within its parent, so [0-7].
		};

	private:
		octree_data_const_t* m_octree;
		value m_current;
		std::stack<value> m_stack;
		bool m_finished = false;

	public:
		octree_traverser(octree_data_const_t* octree, size_t init_node_address = 0);

		bool has_finished();
		value next(uint32_t stop_at_level = 0);
	};
}
