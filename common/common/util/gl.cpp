#include "gl.hpp"

#include <fstream>
#include <vector>
#include <iostream>

#include <glm/gtc/type_ptr.hpp>

void rgc::create_texture_buffer(rgc::texture_buffer& texture_buffer, GLuint buffer_name, GLenum internal_format)
{
	glGenTextures(1, &texture_buffer.m_texture_name);
	glBindTexture(GL_TEXTURE_BUFFER, texture_buffer.m_texture_name);

	glTexBuffer(GL_TEXTURE_BUFFER, internal_format, buffer_name);
	texture_buffer.m_buffer_name = buffer_name;
}

GLuint rgc::measure_time_elapsed(GLuint query_object, std::function<void()> const& f)
{
	glBeginQuery(GL_TIME_ELAPSED, query_object);

	f();

	glEndQuery(GL_TIME_ELAPSED);

	GLuint time_elapsed; glGetQueryObjectuiv(query_object, GL_QUERY_RESULT, &time_elapsed);
	return time_elapsed;
}

// ================================================================================================
// Shader
// ================================================================================================

Shader::Shader(GLenum type)
	:
	id(glCreateShader(type))
{}

Shader::~Shader()
{
	glDeleteShader(this->id);
}

void Shader::source_from_string(const GLchar* source)
{
	glShaderSource(this->id, 1, &source, nullptr);
}

void Shader::source_from_file(const GLchar* path)
{
	std::ifstream f(path);
	if (!f.is_open())
	{
		throw std::invalid_argument("Couldn't open the file at the given path.");
	}
	std::string string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

	this->source_from_string(string.c_str());
}

void Shader::compile()
{
	glCompileShader(this->id);

	GLint status{};
	glGetShaderiv(this->id, GL_COMPILE_STATUS, &status);

	if (status == GL_FALSE)
	{
		std::cerr << get_log() << std::endl;
		throw std::runtime_error("Shader failed to compile. See logs in console");
	}
}

std::string Shader::get_log()
{
	GLint log_length = 0;
	glGetShaderiv(this->id, GL_INFO_LOG_LENGTH, &log_length);

	std::vector<GLchar> log(log_length);
	glGetShaderInfoLog(this->id, log_length, NULL, log.data());

	return std::string(log.begin(), log.end());
}

// ================================================================================================
// Program
// ================================================================================================

Program::Program()
	:
	id(glCreateProgram())
{}

Program::~Program()
{
	glDeleteProgram(this->id);
}

void Program::attach_shader(const Shader& shader)
{
	glAttachShader(this->id, shader.id);
}

void Program::detach_shader(const Shader& shader)
{
	glDetachShader(this->id, shader.id);
}

bool Program::link()
{
	glLinkProgram(this->id);

	GLint status;
	glGetProgramiv(this->id, GL_LINK_STATUS, &status);

	if (status == GL_FALSE)
	{
		std::cerr << get_log() << std::endl;
		throw std::runtime_error("Program failed to compile. See logs on console.");
	}

	return true;
}

void Program::use()
{
	glUseProgram(this->id);
}

void Program::unuse()
{
	glUseProgram(0);
}

std::string Program::get_log()
{
	GLint log_length = 0;
	glGetProgramiv(this->id, GL_INFO_LOG_LENGTH, &log_length);

	std::vector<GLchar> log(log_length);
	glGetProgramInfoLog(this->id, log_length, NULL, log.data());

	return std::string(log.begin(), log.end());
}

GLint Program::get_attrib_location(const GLchar* name, GLboolean safe)
{
	GLint loc = glGetAttribLocation(this->id, name);
	if (safe && loc < 0)
		throw std::invalid_argument("Attrib location is negative.");
}

GLint Program::get_uniform_location(const GLchar* name, GLboolean safe) const
{
	GLint loc = glGetUniformLocation(this->id, name);
	if (loc < 0) {
		//throw std::runtime_error("Invalid uniform location.");
		//std::cerr << "Invalid uniform: " << name << std::endl;
	}

	return loc;
}

// ================================================================================================
// TextureBuffer
// ================================================================================================

TextureBuffer::TextureBuffer()
	:
	buffer_name([]()
				{
					GLuint name;
					glGenBuffers(1, &name);
					return name;
				}()),
	texture_name([]()
				 {
					 GLuint name;
					 glGenTextures(1, &name);
					 return name;
				 }())
{}

TextureBuffer::~TextureBuffer()
{
	glDeleteBuffers(1, &this->buffer_name);
	glDeleteTextures(1, &this->texture_name);
}

void TextureBuffer::load_data(GLsizei size, const void* data, GLenum usage)
{
	glBindBuffer(GL_TEXTURE_BUFFER, this->buffer_name);

	glBufferData(GL_TEXTURE_BUFFER, size, data, usage);

	glBindBuffer(GL_TEXTURE_BUFFER, 0);
}

void TextureBuffer::set_format(GLenum format)
{
	glBindBuffer(GL_TEXTURE_BUFFER, this->buffer_name);
	glBindTexture(GL_TEXTURE_BUFFER, this->texture_name);

	glTexBuffer(GL_TEXTURE_BUFFER, format, this->buffer_name);

	glBindTexture(GL_TEXTURE_BUFFER, 0);
	glBindBuffer(GL_TEXTURE_BUFFER, 0);
}

void TextureBuffer::bind(GLuint binding, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format) const
{
	glBindImageTexture(binding, this->texture_name, level, layered, layer, access, format);
}

// ================================================================================================
// AtomicCounter
// ================================================================================================

AtomicCounter::AtomicCounter()
	:
	name([]()
		 {
			 GLuint name;
			 glGenBuffers(1, &name);
			 return name;
		 }())
{
	set_value(0);
}

AtomicCounter::~AtomicCounter()
{
	glDeleteBuffers(1, &this->name);
}

GLuint AtomicCounter::get_value()
{
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, this->name);

	GLuint value;
	glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &value);

	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

	return value;
}

void AtomicCounter::set_value(GLuint value)
{
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, this->name);
	glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &value, GL_DYNAMIC_DRAW);

	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
}

void AtomicCounter::bind(GLuint binding)
{
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, binding, this->name);
}

// ================================================================================================
// SSBO
// ================================================================================================

ShaderStorageBuffer::ShaderStorageBuffer()
	:
	name([]()
		 {
			 GLuint name;
			 glGenBuffers(1, &name);
			 return name;
		 }())
{}

ShaderStorageBuffer::~ShaderStorageBuffer()
{
	glDeleteBuffers(1, &this->name);
}

void ShaderStorageBuffer::load_data(GLsizei size, const void* data, GLenum usage) const
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->name);
	glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, usage);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void ShaderStorageBuffer::bind(GLuint binding) const
{
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, this->name);
}

// ================================================================================================
// FrameBuffer
// ================================================================================================

FrameBuffer::FrameBuffer()
	:
	name([]()
		 {
			 GLuint res;
			 glGenFramebuffers(1, &res);
			 return res;
		 }())
{}

FrameBuffer::~FrameBuffer()
{
	glDeleteFramebuffers(1, &this->name);
}

void FrameBuffer::set_default_size(GLuint width, GLuint height)
{
	glBindFramebuffer(GL_FRAMEBUFFER, this->name);

	glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_WIDTH, width);
	glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_HEIGHT, height);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FrameBuffer::use()
{
	glBindFramebuffer(GL_FRAMEBUFFER, this->name);
}

void FrameBuffer::unuse()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// ================================================================================================
// Texture3d
// ================================================================================================

Texture3d::Texture3d()
	:
	name([]()
		 {
			 GLuint res;
			 glGenTextures(1, &res);
			 return res;
		 }())
{}

Texture3d::~Texture3d()
{
	glDeleteTextures(1, &this->name);
}

void Texture3d::bind() const
{
	glBindTexture(GL_TEXTURE_3D, this->name);
}

void Texture3d::unbind()
{
	glBindTexture(GL_TEXTURE_3D, 0);
}

void Texture3d::set_storage(GLenum format, GLuint width, GLuint height, GLuint depth) const
{
	this->bind();

	glTexStorage3D(GL_TEXTURE_3D, 1, format, width, height, depth);

	this->unbind();
}

// ------------------------------------------------------------------------------------------------

char const* g_dd_vert_shader_src =
	"#version 330\n"
	"\n"
	"layout(location = 0) in vec3 v_pos;\n"
	"layout(location = 1) in vec3 v_col;\n"
	"\n"
	"uniform mat4 u_proj;\n"
	"uniform mat4 u_view;\n"
	"\n"
	"out vec3 f_col;\n"
	"\n"
	"void main()\n"
	"{\n"
	"gl_Position = u_proj * u_view * vec4(v_pos, 1.0);\n"
	"f_col = v_col;\n"
	"}\n"
	"";

char const* g_dd_frag_shader_src =
	"#version 330\n"
	"\n"
	"in vec3 f_col;\n"
	"\n"
	"void main()\n"
	"{\n"
	"gl_FragColor = vec4(f_col, 1.0);\n"
	"}\n"
	"";

std::vector<float> g_lines;
size_t g_lines_num = 0;

GLuint g_buf = NULL;
size_t g_buf_size = 0;

GLuint g_program = NULL;

void rgc::debug_renderer::cube(glm::vec3 min, glm::vec3 max, glm::vec3 color)
{
	// Bottom
	debug_renderer::line(glm::vec3(min.x, min.y, min.z), glm::vec3(max.x, min.y, min.z), color);
	debug_renderer::line(glm::vec3(min.x, min.y, min.z), glm::vec3(min.x, min.y, max.z), color);
	debug_renderer::line(glm::vec3(max.x, min.y, min.z), glm::vec3(max.x, min.y, max.z), color);
	debug_renderer::line(glm::vec3(min.x, min.y, max.z), glm::vec3(max.x, min.y, max.z), color);

	// Top
	debug_renderer::line(glm::vec3(min.x, max.y, min.z), glm::vec3(max.x, max.y, min.z), color);
	debug_renderer::line(glm::vec3(min.x, max.y, min.z), glm::vec3(min.x, max.y, max.z), color);
	debug_renderer::line(glm::vec3(max.x, max.y, min.z), glm::vec3(max.x, max.y, max.z), color);
	debug_renderer::line(glm::vec3(min.x, max.y, max.z), glm::vec3(max.x, max.y, max.z), color);

	// Side
	debug_renderer::line(glm::vec3(min.x, min.y, min.z), glm::vec3(min.x, max.y, min.z), color);
	debug_renderer::line(glm::vec3(max.x, min.y, min.z), glm::vec3(max.x, max.y, min.z), color);
	debug_renderer::line(glm::vec3(max.x, min.y, max.z), glm::vec3(max.x, max.y, max.z), color);
	debug_renderer::line(glm::vec3(min.x, min.y, max.z), glm::vec3(min.x, max.y, max.z), color);
}

void rgc::debug_renderer::line(glm::vec3 from, glm::vec3 to, glm::vec3 col)
{
	g_lines.push_back(from.x);g_lines.push_back(from.y);g_lines.push_back(from.z);
	g_lines.push_back(col.r);g_lines.push_back(col.g);g_lines.push_back(col.b);
	g_lines.push_back(to.x);g_lines.push_back(to.y);g_lines.push_back(to.z);
	g_lines.push_back(col.r);g_lines.push_back(col.g);g_lines.push_back(col.b);

	g_lines_num += 2;
}

void rgc::debug_renderer::flush(glm::mat4 const& proj_mtx, glm::mat4 const& view_mtx)
{
	if (g_lines.empty()) {
		return;
	}

	glEnable(GL_DEPTH_TEST);

	size_t req_buf_size = g_lines.size() * sizeof(float);
	if (req_buf_size > g_buf_size)
	{
		glGenBuffers(1, &g_buf);
		glBindBuffer(GL_ARRAY_BUFFER, g_buf);
		glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr) req_buf_size, nullptr, GL_DYNAMIC_DRAW);

		g_buf_size = req_buf_size;

		//std::cout << "DebugDraw buffer has been resized to: " << g_buf_size << std::endl;
	}

	glUseProgram(g_program);

	glUniformMatrix4fv(glGetUniformLocation(g_program, "u_view"), 1, GL_FALSE, glm::value_ptr(view_mtx));
	glUniformMatrix4fv(glGetUniformLocation(g_program, "u_proj"), 1, GL_FALSE, glm::value_ptr(proj_mtx));

	glBindBuffer(GL_ARRAY_BUFFER, g_buf);
	glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr) (g_lines.size() * sizeof(float)), g_lines.data());

	glEnableVertexAttribArray(0); // pos
	glVertexAttribPointer(0, 3, GL_FLOAT, false, 2 * sizeof(glm::vec3), nullptr);

	glEnableVertexAttribArray(1); // col
	glVertexAttribPointer(1, 3, GL_FLOAT, false, 2 * sizeof(glm::vec3), (void*) sizeof(glm::vec3));

	glDrawArrays(GL_LINES, 0, g_lines_num);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glUseProgram(0);

	g_lines.clear();
	g_lines_num = 0;
}

void rgc::debug_renderer::init()
{
	g_program = glCreateProgram();

	GLuint v_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(v_shader, 1, &g_dd_vert_shader_src, nullptr);
	glCompileShader(v_shader);
	glAttachShader(g_program, v_shader);

	GLuint f_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(f_shader, 1, &g_dd_frag_shader_src, nullptr);
	glCompileShader(f_shader);
	glAttachShader(g_program, f_shader);

	glLinkProgram(g_program);
}

void rgc::debug_renderer::destroy()
{
	glDeleteProgram(g_program);
	if (g_buf != NULL) {
		glDeleteBuffers(1, &g_buf);
	}
}
