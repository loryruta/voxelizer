#include "screen_quad.hpp"

const GLfloat vertices[]{
	-1.0f, -1.0f,
	1.0f, -1.0f,
	1.0f, 1.0f,
	1.0f, 1.0f,
	-1.0f, 1.0f,
	-1.0f, -1.0f
};

ScreenQuad::ScreenQuad()
{
	// VBO
	glGenBuffers(1, &this->vbo);

	glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// VAO
	glGenVertexArrays(1, &this->vao);

	glBindVertexArray(this->vao);
	glBindBuffer(GL_ARRAY_BUFFER, this->vbo);

	glEnableVertexArrayAttrib(this->vao, 0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), 0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

ScreenQuad::~ScreenQuad()
{
	glDeleteBuffers(1, &this->vbo);
}

void ScreenQuad::render()
{
	glDisable(GL_CULL_FACE); // todo

	glBindVertexArray(this->vao);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glBindVertexArray(0);
}
