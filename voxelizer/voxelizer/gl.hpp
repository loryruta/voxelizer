#pragma once

#include <string>
#include <functional>

#include <glad/glad.h>
#include <glm/glm.hpp>

namespace voxelizer
{
	// ------------------------------------------------------------------------------------------------
	// shader
	// ------------------------------------------------------------------------------------------------

	struct shader
	{
		GLuint m_name;

		shader(GLenum type);
		shader(shader const&) = delete;
		shader(shader&&);

		~shader();

		void source_from_string(const GLchar* source);
		void source_from_file(const GLchar* path);

		void compile();
		std::string get_log();
	};

	// ------------------------------------------------------------------------------------------------
	// program
	// ------------------------------------------------------------------------------------------------

	struct program
	{
		GLuint m_name;

		program();
		program(program const&) = delete;
		program(program&&);

		~program();

		void attach_shader(shader const& shader);
		void detach_shader(shader const& shader);

		void link();

		void use();
		static void unuse();

		std::string get_log();

		GLint get_attrib_location(const GLchar* name) const;
		GLint get_uniform_location(const GLchar* name) const;
	};

	// ------------------------------------------------------------------------------------------------
	// texture_buffer
	// ------------------------------------------------------------------------------------------------

	struct texture_buffer
	{
		GLuint m_buffer_name;
		GLuint m_texture_name;

		texture_buffer();
		texture_buffer(texture_buffer const&) = delete;
		texture_buffer(texture_buffer&&);

		~texture_buffer();

		void load_data(GLsizei size, const void* data, GLenum usage);
		void set_format(GLenum format);

		void bind(GLuint binding, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format) const;
	};

	// ------------------------------------------------------------------------------------------------
	// atomic_counter
	// ------------------------------------------------------------------------------------------------

	struct atomic_counter
	{
		GLuint m_name;

		atomic_counter();
		atomic_counter(atomic_counter const&) = delete;
		atomic_counter(atomic_counter&& other);

		~atomic_counter();

		GLuint get_value();
		void set_value(GLuint value);

		void bind(GLuint binding);
	};

	// ------------------------------------------------------------------------------------------------
	// debug_renderer
	// ------------------------------------------------------------------------------------------------

	class debug_renderer
	{
	private:
		program m_program;

		std::vector<float> m_lines;
		uint32_t m_line_count = 0;

		GLuint m_buffer = NULL;
		size_t m_buffer_size = 0;


	public:
		debug_renderer();
		~debug_renderer();

		void cube(glm::vec3 min, glm::vec3 max, glm::vec3 color);
		void line(glm::vec3 from, glm::vec3 to, glm::vec3 col);

		void flush(glm::mat4 const& view, glm::mat4 const& projection);
	};
}
