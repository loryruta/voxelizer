#pragma once

#include <vector>
#include <memory>

#include "mesh.hpp"

namespace voxelizer
{
	struct scene
	{
		glm::vec3 m_transformed_min, m_transformed_max;
		std::vector<Mesh> m_meshes;

		inline glm::vec3 get_transformed_size() const
		{
			return m_transformed_max - m_transformed_min;
		}

		inline size_t get_triangles_count()
		{
			size_t triangle_count = 0;
			for (Mesh const& mesh : m_meshes)
			{
				triangle_count += mesh.m_triangle_count;
			}
			return triangle_count;
		}
	};
}
