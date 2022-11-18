#pragma once

#include <vector>
#include <memory>

#include "mesh.hpp"

namespace voxelizer
{
	struct scene
	{
		glm::vec3 m_transformed_min, m_transformed_max;
		std::vector<std::shared_ptr<Mesh>> m_meshes;

		inline glm::vec3 get_transformed_size() const
		{
			return m_transformed_max - m_transformed_min;
		}

		inline size_t get_triangles_count()
		{
			size_t num_of_triangles = 0;
			for (auto const& mesh : m_meshes) {
				num_of_triangles += mesh->m_num_of_triangles;
			}
			return num_of_triangles;
		}
	};
}
