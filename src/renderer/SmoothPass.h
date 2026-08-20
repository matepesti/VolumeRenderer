#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "Shader.h"

class SmoothPass {
private:
	GLuint m_texA = 0;
	GLuint m_texB = 0;
	Shader compShader;
	int m_nx = 0;
	int m_ny = 0;
	int m_nz = 0;
public:
	GLuint run(GLuint sourceTexture, int nx, int ny, int nz, float sigma);
	void init(const std::string& compPath);
};
