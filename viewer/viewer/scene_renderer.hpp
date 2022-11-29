#pragma once

#include <voxelizer/scene.hpp>
#include <voxelizer/util/camera.hpp>
#include <voxelizer/util/gl.hpp>

namespace voxelizer
{
	class scene_renderer
	{
	private:
		Program m_program;

	public:
		Material::Type m_view_type = Material::Type::DIFFUSE; // TODO

		scene_renderer();

		void render(
			glm::mat4 const& camera_projection,
			glm::mat4 const& camera_view,
			glm::mat4 const& transform,
			voxelizer::scene const& scene
		);
	};
}
