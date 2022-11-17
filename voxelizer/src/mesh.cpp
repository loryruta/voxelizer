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

Mesh::Mesh() :
	vao([]() {
		GLuint vao;
		glGenVertexArrays(1, &vao);
		return vao;
	}()),
	vbo([]() {
		std::array<GLuint, Mesh::Attribute::size> vbo;
		glGenBuffers(Mesh::Attribute::size, vbo.data());
		return vbo;
	}()),
	ebo([]() {
		GLuint ebo;
		glGenBuffers(1, &ebo);
		return ebo;
	}())
{}

Mesh::~Mesh()
{
	glDeleteVertexArrays(1, &this->vao);
	glDeleteBuffers(Mesh::Attribute::size, this->vbo.data());

	glDeleteBuffers(1, &this->ebo);
}

