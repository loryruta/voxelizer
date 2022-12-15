#pragma once

#include <array>
#include <memory>
#include <vector>

#include <glad/glad.h>

#include <glm/glm.hpp>

namespace voxelizer
{
	// ------------------------------------------------------------------------------------------------
	// material
	// ------------------------------------------------------------------------------------------------

	class material
	{
	public:
		enum type
		{
			NONE,
			DIFFUSE,
			AMBIENT,
			SPECULAR,
			EMISSIVE,

			Count
		};

	private:
		glm::vec4 m_colors[material::type::Count];
		GLuint m_textures[material::type::Count];

	public:
		material();
		material(const material&) = delete;
		material(const material&&) = delete;

		~material();

		glm::vec4& get_color(material::type type)
		{
			return m_colors[type];
		}

		GLuint get_texture(material::type type) const
		{
			return m_textures[type];
		}
	};

	// ------------------------------------------------------------------------------------------------
	// mesh
	// ------------------------------------------------------------------------------------------------

	struct mesh
	{
		enum attribute
		{
			POSITION = 0,
			NORMAL = 1,
			UV = 2,
			COLOR = 3,

			Count
		};

		bool m_valid = true;

		GLuint m_vao;
		std::array<GLuint, mesh::attribute::Count> m_vbos;
		GLuint m_ebo;

		size_t m_triangle_count;
		size_t m_element_count;

		glm::mat4 m_transform;

		std::shared_ptr<material> m_material;

		glm::vec3 m_transformed_min, m_transformed_max;

		mesh();
		mesh(mesh const&) = delete;
		mesh(mesh&& other) noexcept;

		~mesh();
	};

	// ------------------------------------------------------------------------------------------------
	// scene
	// ------------------------------------------------------------------------------------------------

	struct scene
	{
		glm::vec3 m_transformed_min, m_transformed_max;
		std::vector<mesh> m_meshes;

		inline glm::vec3 get_transformed_size() const
		{
			return m_transformed_max - m_transformed_min;
		}

		inline size_t get_triangles_count()
		{
			size_t triangle_count = 0;
			for (mesh const& mesh : m_meshes)
			{
				triangle_count += mesh.m_triangle_count;
			}
			return triangle_count;
		}
	};
}
