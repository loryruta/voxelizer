#pragma once

#include <glad/glad.h>

#include <common/util/gl.hpp>

namespace voxelizer
{
	struct VoxelList
	{
		TextureBuffer position_buffer, color_buffer;
		size_t m_size;

		VoxelList();

		void alloc(size_t size);
		void bind(GLuint position_binding, GLuint color_binding) const;
	};
}

