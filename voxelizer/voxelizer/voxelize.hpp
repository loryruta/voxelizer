#pragma once

#include <optional>

#include <glm/glm.hpp>

#include "util/gl.hpp"
#include "scene.hpp"
#include "voxel_list.hpp"

namespace voxelizer
{
	struct voxelize
	{
	private:
		void invoke(voxelizer::scene const& scene);

	public:
		Program m_program;
		AtomicCounter m_atomic_counter;
		GLuint m_errors_counter;

		voxelize();

		static glm::uvec3 calc_proportional_grid(glm::vec3 size, uint32_t voxels_on_y);
		static glm::mat4 create_scene_normalization_matrix(glm::vec3 area_position, glm::vec3 area_size);
		static void create_projection_matrices(glm::mat4 matrices[3]);

		/**
		 * @param voxel_list  The resulting list of voxels generated.
		 * @param scene       The scene to voxelize.
		 * @param voxels_on_y Number of voxels along the Y axis.
		 * @param offset      Where to start taking the voxelization area.
		 * @param size        The size of the voxelization area.
		 */
		void operator()(VoxelList& voxel_list, scene const& scene, uint32_t voxels_on_y, glm::vec3 area_position, glm::vec3 area_size);
	};
}
