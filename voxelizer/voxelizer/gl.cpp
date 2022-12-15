#include "gl.hpp"

#include <fstream>
#include <vector>
#include <iostream>

#include <glm/gtc/type_ptr.hpp>

// ------------------------------------------------------------------------------------------------
// shader
// ------------------------------------------------------------------------------------------------

voxelizer::shader::shader(GLenum type) :
	m_name(glCreateShader(type))
{
}

voxelizer::shader::shader(shader&& other) :
	m_name(other.m_name)
{
	other.m_name = NULL;
}

voxelizer::shader::~shader()
{
	if (m_name != NULL)
		glDeleteShader(m_name);
}

void voxelizer::shader::source_from_string(GLchar const* source)
{
	glShaderSource(m_name, 1, &source, nullptr);
}

void voxelizer::shader::source_from_file(GLchar const* path)
{
	std::ifstream f(path);
	if (!f.is_open())
	{
		throw std::invalid_argument("Failed to open the file");
	}
	std::string string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

	this->source_from_string(string.c_str());
}

void voxelizer::shader::compile()
{
	glCompileShader(m_name);

	GLint status{};
	glGetShaderiv(m_name, GL_COMPILE_STATUS, &status);

	if (status == GL_FALSE)
	{
		std::cerr << get_log() << std::endl;
		throw std::runtime_error("Shader compilation failed");
	}
}

std::string voxelizer::shader::get_log()
{
	GLint log_length = 0;
	glGetShaderiv(m_name, GL_INFO_LOG_LENGTH, &log_length);

	std::vector<GLchar> log(log_length);
	glGetShaderInfoLog(m_name, log_length, NULL, log.data());

	return std::string(log.begin(), log.end());
}

// ------------------------------------------------------------------------------------------------
// program
// ------------------------------------------------------------------------------------------------

voxelizer::program::program() :
	m_name(glCreateProgram())
{
}

voxelizer::program::program(voxelizer::program&& other) :
	m_name(other.m_name)
{
	other.m_name = NULL;
}

voxelizer::program::~program()
{
	if (m_name != NULL)
		glDeleteProgram(m_name);
}

void voxelizer::program::attach_shader(voxelizer::shader const& shader)
{
	glAttachShader(m_name, shader.m_name);
}

void voxelizer::program::detach_shader(voxelizer::shader const& shader)
{
	glDetachShader(m_name, shader.m_name);
}

void voxelizer::program::link()
{
	glLinkProgram(m_name);

	GLint status{};
	glGetProgramiv(m_name, GL_LINK_STATUS, &status);

	if (status == GL_FALSE)
	{
		std::cerr << get_log() << std::endl;
		throw std::runtime_error("Program compilation failed");
	}
}

void voxelizer::program::use()
{
	glUseProgram(m_name);
}

void voxelizer::program::unuse()
{
	glUseProgram(0);
}

std::string voxelizer::program::get_log()
{
	GLint log_length = 0;
	glGetProgramiv(m_name, GL_INFO_LOG_LENGTH, &log_length);

	std::vector<GLchar> log(log_length);
	glGetProgramInfoLog(m_name, log_length, NULL, log.data());

	return std::string(log.begin(), log.end());
}

GLint voxelizer::program::get_attrib_location(GLchar const* attrib_name) const
{
	GLint location = glGetAttribLocation(m_name, attrib_name);
	if (location < 0)
	{
		throw std::invalid_argument("Invalid attrib name");
	}
	return location;
}

GLint voxelizer::program::get_uniform_location(GLchar const* uniform_name) const
{
	GLint location = glGetUniformLocation(m_name, uniform_name);
	if (location < 0)
	{
		throw std::invalid_argument("Invalid uniform name");
	}
	return location;
}

// ------------------------------------------------------------------------------------------------
// texture_buffer
// ------------------------------------------------------------------------------------------------

voxelizer::texture_buffer::texture_buffer()
{
	glGenBuffers(1, &m_buffer_name);
	glGenBuffers(1, &m_texture_name);
}

voxelizer::texture_buffer::texture_buffer(texture_buffer&& other) :
	m_buffer_name(other.m_buffer_name),
	m_texture_name(other.m_texture_name)
{
	other.m_buffer_name = NULL;
	other.m_texture_name = NULL;
}

voxelizer::texture_buffer::~texture_buffer()
{
	if (m_buffer_name != NULL)
		glDeleteBuffers(1, &m_buffer_name);

	if (m_texture_name != NULL)
		glDeleteTextures(1, &m_texture_name);
}

void voxelizer::texture_buffer::load_data(GLsizei size, const void* data, GLenum usage)
{
	glBindBuffer(GL_TEXTURE_BUFFER, m_buffer_name);

	glBufferData(GL_TEXTURE_BUFFER, size, data, usage);

	glBindBuffer(GL_TEXTURE_BUFFER, 0);
}

void voxelizer::texture_buffer::set_format(GLenum format)
{
	glBindBuffer(GL_TEXTURE_BUFFER, m_buffer_name);
	glBindTexture(GL_TEXTURE_BUFFER, m_texture_name);

	glTexBuffer(GL_TEXTURE_BUFFER, format, m_buffer_name);

	glBindTexture(GL_TEXTURE_BUFFER, 0);
	glBindBuffer(GL_TEXTURE_BUFFER, 0);
}

void voxelizer::texture_buffer::bind(GLuint binding, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format) const
{
	glBindImageTexture(binding, m_texture_name, level, layered, layer, access, format);
}

// ------------------------------------------------------------------------------------------------
// atomic_counter
// ------------------------------------------------------------------------------------------------

voxelizer::atomic_counter::atomic_counter()
{
	glGenBuffers(1, &m_name);

	set_value(0);
}

voxelizer::atomic_counter::atomic_counter(atomic_counter&& other) :
	m_name(other.m_name)
{
	other.m_name = NULL;
}

voxelizer::atomic_counter::~atomic_counter()
{
	if (m_name != NULL)
		glDeleteBuffers(1, &m_name);
}

GLuint voxelizer::atomic_counter::get_value()
{
	GLuint value{};
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_name);
	glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &value);

	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

	return value;
}

void voxelizer::atomic_counter::set_value(GLuint value)
{
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, m_name);
	glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &value, GL_DYNAMIC_DRAW);

	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
}

void voxelizer::atomic_counter::bind(GLuint binding)
{
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, binding, m_name);
}

// ------------------------------------------------------------------------------------------------
// debug_renderer
// ------------------------------------------------------------------------------------------------

char const* g_debug_renderer_vertex_shader = R"(
#version 330

layout(location = 0) in vec3 v_pos;
layout(location = 1) in vec3 v_col;

uniform mat4 u_proj;
uniform mat4 u_view;

out vec3 f_col;

void main()
{
	gl_Position = u_proj * u_view * vec4(v_pos, 1.0);
	f_col = v_col;
}
)";

char const* g_debug_renderer_fragment_shader = R"(
#version 330

in vec3 f_col;

void main()
{
	gl_FragColor = vec4(f_col, 1.0);
}
)";

voxelizer::debug_renderer::debug_renderer()
{
	shader vertex_shader(GL_VERTEX_SHADER);
	vertex_shader.source_from_string(g_debug_renderer_vertex_shader);
	vertex_shader.compile();

	m_program.attach_shader(vertex_shader);

	shader fragment_shader(GL_FRAGMENT_SHADER);
	fragment_shader.source_from_string(g_debug_renderer_fragment_shader);
	fragment_shader.compile();

	m_program.attach_shader(fragment_shader);

	m_program.link();
}

voxelizer::debug_renderer::~debug_renderer()
{
	if (m_buffer != NULL)
		glDeleteBuffers(1, &m_buffer);
}

void voxelizer::debug_renderer::line(glm::vec3 from, glm::vec3 to, glm::vec3 color)
{
	m_lines.push_back(from.x);  m_lines.push_back(from.y);  m_lines.push_back(from.z);
	m_lines.push_back(color.r); m_lines.push_back(color.g); m_lines.push_back(color.b);
	m_lines.push_back(to.x);    m_lines.push_back(to.y);    m_lines.push_back(to.z);
	m_lines.push_back(color.r); m_lines.push_back(color.g); m_lines.push_back(color.b);

	m_line_count += 2;
}

void voxelizer::debug_renderer::cube(glm::vec3 min, glm::vec3 max, glm::vec3 color)
{
	// Bottom
	line(glm::vec3(min.x, min.y, min.z), glm::vec3(max.x, min.y, min.z), color);
	line(glm::vec3(min.x, min.y, min.z), glm::vec3(min.x, min.y, max.z), color);
	line(glm::vec3(max.x, min.y, min.z), glm::vec3(max.x, min.y, max.z), color);
	line(glm::vec3(min.x, min.y, max.z), glm::vec3(max.x, min.y, max.z), color);

	// Top
	line(glm::vec3(min.x, max.y, min.z), glm::vec3(max.x, max.y, min.z), color);
	line(glm::vec3(min.x, max.y, min.z), glm::vec3(min.x, max.y, max.z), color);
	line(glm::vec3(max.x, max.y, min.z), glm::vec3(max.x, max.y, max.z), color);
	line(glm::vec3(min.x, max.y, max.z), glm::vec3(max.x, max.y, max.z), color);

	// Side
	line(glm::vec3(min.x, min.y, min.z), glm::vec3(min.x, max.y, min.z), color);
	line(glm::vec3(max.x, min.y, min.z), glm::vec3(max.x, max.y, min.z), color);
	line(glm::vec3(max.x, min.y, max.z), glm::vec3(max.x, max.y, max.z), color);
	line(glm::vec3(min.x, min.y, max.z), glm::vec3(min.x, max.y, max.z), color);
}

void voxelizer::debug_renderer::flush(glm::mat4 const& proj_mtx, glm::mat4 const& view_mtx)
{
	if (m_lines.empty())
		return;

	glEnable(GL_DEPTH_TEST);

	size_t required_buffer_size = m_lines.size() * sizeof(float);
	if (required_buffer_size > m_buffer_size)
	{
		glGenBuffers(1, &m_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, m_buffer);
		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)required_buffer_size, nullptr, GL_DYNAMIC_DRAW);

		m_buffer_size = required_buffer_size;
	}

	m_program.use();

	glUniformMatrix4fv(m_program.get_uniform_location("u_view"), 1, GL_FALSE, glm::value_ptr(view_mtx));
	glUniformMatrix4fv(m_program.get_uniform_location("u_proj"), 1, GL_FALSE, glm::value_ptr(proj_mtx));

	glBindBuffer(GL_ARRAY_BUFFER, m_buffer);
	glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr) (m_lines.size() * sizeof(float)), m_lines.data());

	// Position
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, false, 2 * sizeof(glm::vec3), nullptr);

	// Color
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, false, 2 * sizeof(glm::vec3), (void*) sizeof(glm::vec3));

	glDrawArrays(GL_LINES, 0, m_line_count);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glUseProgram(0);

	m_lines.clear();
	m_line_count = 0;
}

// ------------------------------------------------------------------------------------------------
// screen_quad
// ------------------------------------------------------------------------------------------------

const GLfloat g_screen_quad_vertices[]{
	-1.0f, -1.0f,
	1.0f, -1.0f,
	1.0f, 1.0f,
	1.0f, 1.0f,
	-1.0f, 1.0f,
	-1.0f, -1.0f
};

voxelizer::screen_quad::screen_quad()
{
	// VBO
	glGenBuffers(1, &m_vbo);

	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_screen_quad_vertices), g_screen_quad_vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// VAO
	glGenVertexArrays(1, &m_vao);

	glBindVertexArray(m_vao);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

	glEnableVertexArrayAttrib(m_vao, 0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), 0);

	//
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

voxelizer::screen_quad::~screen_quad()
{
	glDeleteBuffers(1, &m_vbo);
	glDeleteVertexArrays(1, &m_vao);
}

void voxelizer::screen_quad::render()
{
	glDisable(GL_CULL_FACE); // TODO

	glBindVertexArray(m_vao);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glBindVertexArray(0);
}

