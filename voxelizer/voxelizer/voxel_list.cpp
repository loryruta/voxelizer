#include "voxel_list.hpp"

#include <iostream>

voxelizer::voxel_list::voxel_list()
{}

void voxelizer::voxel_list::alloc(size_t size)
{
	m_size = size;

	m_position_buffer.load_data(size * sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
	m_position_buffer.set_format(GL_R32UI);

	m_color_buffer.load_data(size * sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
	m_color_buffer.set_format(GL_RGBA8);
}

void voxelizer::voxel_list::bind(GLuint position_binding, GLuint color_binding) const
{
	m_position_buffer.bind(position_binding, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGB10_A2UI);
	m_color_buffer.bind(color_binding, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
}
