#include "voxel_list.hpp"

#include <iostream>

voxelizer::VoxelList::VoxelList()
{}

void voxelizer::VoxelList::alloc(size_t size)
{
	m_size = size;

	this->position_buffer.load_data(size * sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
	this->position_buffer.set_format(GL_R32UI);

	this->color_buffer.load_data(size * sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
	this->color_buffer.set_format(GL_RGBA8);
}

void voxelizer::VoxelList::bind(GLuint position_binding, GLuint color_binding) const
{
	this->position_buffer.bind(position_binding, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGB10_A2UI);
	this->color_buffer.bind(color_binding, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
}
