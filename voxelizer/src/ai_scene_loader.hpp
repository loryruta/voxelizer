#pragma once

#include <memory>
#include <filesystem>

#include "scene.hpp"

namespace voxelizer
{
	class assimp_scene_loader
	{
	public:
		assimp_scene_loader();

		void load(scene& scene, std::filesystem::path const& path);
	};
}
