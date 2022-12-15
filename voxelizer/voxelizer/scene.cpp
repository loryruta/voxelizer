#include "scene.hpp"

// ------------------------------------------------------------------------------------------------
// material
// ------------------------------------------------------------------------------------------------

voxelizer::material::material()
{
	glGenTextures(material::type::Count, m_textures);
}

voxelizer::material::~material()
{
	glDeleteTextures(material::type::Count, m_textures);
}

// ------------------------------------------------------------------------------------------------
// mesh
// ------------------------------------------------------------------------------------------------

voxelizer::mesh::mesh()
{
	glGenVertexArrays(1, &m_vao);
	glGenBuffers(mesh::attribute::Count, m_vbos.data());
	glGenBuffers(1, &m_ebo);
}

voxelizer::mesh::mesh(voxelizer::mesh&& other) noexcept :
	m_valid(true),

	m_vao(other.m_vao),
	m_vbos(other.m_vbos),
	m_ebo(other.m_ebo),
	m_triangle_count(other.m_triangle_count),
	m_element_count(other.m_element_count),
	m_transform(other.m_transform),
	m_material(other.m_material),
	m_transformed_min(other.m_transformed_min),
	m_transformed_max(other.m_transformed_max)
{
	other.m_valid = false;
}

voxelizer::mesh::~mesh()
{
	if (m_valid)
	{
		glDeleteVertexArrays(1, &m_vao);
		glDeleteBuffers(mesh::attribute::Count, m_vbos.data());
		glDeleteBuffers(1, &m_ebo);
	}
}
