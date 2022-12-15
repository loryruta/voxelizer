#pragma once

#include <voxelizer/scene.hpp>
#include <voxelizer/gl.hpp>

namespace voxelizer
{
	class scene_renderer
	{
	private:
		program m_program;

	public:
		material::type m_view_type = material::type::DIFFUSE; // TODO

		scene_renderer();

		void render(
			glm::mat4 const& camera_projection,
			glm::mat4 const& camera_view,
			glm::mat4 const& transform,
			voxelizer::scene const& scene
		);
	};
}
