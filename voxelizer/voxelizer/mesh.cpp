#include "mesh.hpp"

#include "scene.hpp"

// ------------------------------------------------------------------------------------------------
// Material
// ------------------------------------------------------------------------------------------------

Material::Material()
{
	glGenTextures(Material::Type::size, this->texture);
}

Material::~Material()
{
	glDeleteTextures(Material::Type::size, this->texture);
}

// ------------------------------------------------------------------------------------------------
// Mesh
// ------------------------------------------------------------------------------------------------

Mesh::Mesh()
{
	glGenVertexArrays(1, &m_vao);
	glGenBuffers(Mesh::Attribute::size, m_vbos.data());
	glGenBuffers(1, &m_ebo);
}

Mesh::Mesh(Mesh&& other) noexcept :
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

Mesh::~Mesh()
{
	if (m_valid)
	{
		glDeleteVertexArrays(1, &m_vao);
		glDeleteBuffers(Mesh::Attribute::size, m_vbos.data());
		glDeleteBuffers(1, &m_ebo);
	}
}

