#pragma once

#include <glad/glad.h>

#include "gl.hpp"

namespace voxelizer
{
	struct voxel_list
	{
		voxelizer::texture_buffer m_position_buffer;
		voxelizer::texture_buffer m_color_buffer;
		size_t m_size;

		voxel_list();

		void alloc(size_t size);
		void bind(GLuint position_binding, GLuint color_binding) const;
	};
}
