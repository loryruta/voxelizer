#pragma once

#include <array>
#include <memory>

#include <glad/glad.h>

#include <glm/glm.hpp>

// ================================================================================================
// Material
// ================================================================================================

class Material
{
public:
	enum Type
	{
		NONE,
		DIFFUSE,
		AMBIENT,
		SPECULAR,
		EMISSIVE,

		size
	};

private:
	glm::vec4 color[Material::Type::size];
	GLuint texture[Material::Type::size];

public:
	Material();
	Material(const Material&) = delete;
	Material(const Material&&) = delete;

	~Material();

	glm::vec4& get_color(Material::Type type) { return color[type]; }
	GLuint get_texture(Material::Type type) { return texture[type]; }
};

// ================================================================================================
// Mesh
// ================================================================================================

struct Mesh
{
	enum Attribute
	{
		POSITION = 0,
		NORMAL = 1,
		UV = 2,
		COLOR = 3,

		size
	};

	bool m_valid = true;

	GLuint m_vao;
	std::array<GLuint, Mesh::Attribute::size> m_vbos;
	GLuint m_ebo;

	size_t m_triangle_count;
	size_t m_element_count;

	glm::mat4 m_transform;

	std::shared_ptr<Material> m_material;

	glm::vec3 m_transformed_min, m_transformed_max;

	Mesh();
	Mesh(Mesh const&) = delete;
	Mesh(Mesh&& other) noexcept;

	~Mesh();
};

