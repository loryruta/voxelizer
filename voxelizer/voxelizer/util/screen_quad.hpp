#pragma once

#include <glad/glad.h>

class ScreenQuad
{
private:
	GLuint vao, vbo;

public:
	ScreenQuad();
	~ScreenQuad();

	void render();
};
