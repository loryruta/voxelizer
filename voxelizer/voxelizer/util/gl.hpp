#pragma once

#include <string>
#include <functional>

#include <glad/glad.h>
#include <glm/glm.hpp>

namespace rgc
{
	struct texture_buffer
	{
		GLuint m_texture_name;
		GLuint m_buffer_name;
	};

	void create_texture_buffer(rgc::texture_buffer& texture_buffer, GLuint buffer_name, GLenum internal_format);

	GLuint measure_time_elapsed(GLuint query_object, std::function<void()> const& f);
}

// ================================================================================================
// Shader
// ================================================================================================

struct Shader
{
	const GLuint id;

	Shader(GLenum type);
	Shader(const Shader&) = delete;

	~Shader();

	void source_from_string(const GLchar* source);
	void source_from_file(const GLchar* path);

	void compile();
	std::string get_log();
};

// ================================================================================================
// Program
// ================================================================================================

struct Program
{
	const GLuint id;

	Program();
	Program(const Program&) = delete;
	
	~Program();

	void attach_shader(const Shader& shader);
	void detach_shader(const Shader& shader);

	void link();

	void use();
	static void unuse();

	std::string get_log();

	GLint get_attrib_location(const GLchar* name, GLboolean safe = true);
	GLint get_uniform_location(const GLchar* name, GLboolean safe = true) const;
};

// ================================================================================================
// TextureBuffer
// ================================================================================================

struct TextureBuffer
{
	const GLuint buffer_name, texture_name;

	TextureBuffer();
	//TextureBuffer(const TextureBuffer&) = delete;
	~TextureBuffer();

	void load_data(GLsizei size, const void* data, GLenum usage);
	void set_format(GLenum format);

	void bind(GLuint binding, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format) const;
};

// ================================================================================================
// AtomicCounter
// ================================================================================================

struct AtomicCounter
{
	const GLuint name;

	AtomicCounter();
	AtomicCounter(const AtomicCounter&) = delete;

	~AtomicCounter();

	GLuint get_value();
	void set_value(GLuint value);

	void bind(GLuint binding);
};

// ================================================================================================
// SSBO
// ================================================================================================

struct ShaderStorageBuffer
{
	const GLuint name;

	ShaderStorageBuffer();
	ShaderStorageBuffer(const ShaderStorageBuffer&) = delete;

	~ShaderStorageBuffer();

	void load_data(GLsizei size, const void* data, GLenum usage) const;

	void bind(GLuint binding) const;
};

// ================================================================================================
// FrameBuffer
// ================================================================================================

struct FrameBuffer
{
	const GLuint name;

	FrameBuffer();
	~FrameBuffer();

	FrameBuffer(const FrameBuffer&) = delete;
	FrameBuffer(const FrameBuffer&&) = delete;

	void set_default_size(GLuint width, GLuint height);

	void use();
	void unuse();
};

// ================================================================================================
// Texture3d
// ================================================================================================

struct Texture3d
{
	const GLuint name;

	Texture3d();
	~Texture3d();

	Texture3d(const Texture3d&) = delete;
	Texture3d(const Texture3d&&) = delete;

	void bind() const;
	static void unbind();

	void set_storage(GLenum format, GLuint width, GLuint height, GLuint depth) const;
};

// ------------------------------------------------------------------------------------------------

namespace rgc::debug_renderer
{
	void cube(glm::vec3 min, glm::vec3 max, glm::vec3 color);
	void line(glm::vec3 from, glm::vec3 to, glm::vec3 col);

	void flush(glm::mat4 const& proj_mtx, glm::mat4 const& view_mtx);

	void init();
	void destroy();
}
