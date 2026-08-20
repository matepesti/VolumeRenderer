#include "Shader.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <glm/gtc/type_ptr.hpp>

GLuint Shader::compile(const std::string& path, GLenum type) {
	GLuint id = glCreateShader(type);

	std::ifstream istream(path);
	if (!istream.is_open()) {
		std::cout << "Shader file couldn't be opened and compiled!\n";
		return 0;
	}
	std::string result((std::istreambuf_iterator<char>(istream)),std::istreambuf_iterator<char>());

	const char* source = result.c_str();

	glShaderSource(id, 1, &source, nullptr);
	glCompileShader(id);

	int success;
	glGetShaderiv(id, GL_COMPILE_STATUS, &success);
	if (success == GL_FALSE) {
		char logbuffer[512];
		glGetShaderInfoLog(id, 512, nullptr,logbuffer);
		std::cout << logbuffer << "\n";
		return 0;
	}
	return id;
}

void Shader::use() {
	glUseProgram(m_programID);
}

void Shader::setInt(const std::string& name, int value) {
	int location = glGetUniformLocation(m_programID, name.c_str());
	glUniform1i(location, value);
}
void Shader::setFloat(const std::string& name, float value) {
	int location = glGetUniformLocation(m_programID, name.c_str());
	glUniform1f(location, value);
}
void Shader::setVec3(const std::string& name, glm::vec3 value) {
	int location = glGetUniformLocation(m_programID, name.c_str());
	glUniform3fv(location, 1, glm::value_ptr(value));
}
void Shader::setMat4(const std::string& name, glm::mat4 value) {
	int location = glGetUniformLocation(m_programID, name.c_str());
	glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
}
void Shader::setBool(const std::string& name, bool value) {
	int location = glGetUniformLocation(m_programID, name.c_str());
	glUniform1i(location, value ? 1 : 0);
}

Shader::Shader(const std::string& vertPath, const std::string& fragPath) {
	GLuint vertID = compile(vertPath, GL_VERTEX_SHADER);
	GLuint fragID = compile(fragPath, GL_FRAGMENT_SHADER);

	if ((vertID == 0) || (fragID == 0)) {
		std::cout << "Error with the vert or frag ID\n";
		return;
	}

	m_programID = glCreateProgram();
	glAttachShader(m_programID, vertID);
	glAttachShader(m_programID, fragID);
	glLinkProgram(m_programID);
	int success = -1;
	glGetProgramiv(m_programID, GL_LINK_STATUS, &success);
	if (success == GL_FALSE) {
		char logbuffer[512];
		glGetProgramInfoLog(m_programID, 512, nullptr, logbuffer);
		std::cout << logbuffer << "\n";
	}

	glDeleteShader(vertID);
	glDeleteShader(fragID);
}

void Shader::compileCompute(const std::string& path) {
	std::ifstream istream(path);
	if (!istream.is_open()) {
		std::cout << "Shader file couldn't be opened and compiled!\n";
		return;
	}
	std::string result((std::istreambuf_iterator<char>(istream)), std::istreambuf_iterator<char>());

	const char* source = result.c_str();

	GLuint id = glCreateShader(GL_COMPUTE_SHADER);

	glShaderSource(id, 1, &source, nullptr);
	glCompileShader(id);

	int success;
	glGetShaderiv(id, GL_COMPILE_STATUS, &success);
	if (success == GL_FALSE) {
		char logbuffer[512];
		glGetShaderInfoLog(id, 512, nullptr, logbuffer);
		std::cout << logbuffer << "\n";
		return;
	}

	m_programID = glCreateProgram();
	glAttachShader(m_programID, id);
	glLinkProgram(m_programID);

	glGetShaderiv(id, GL_LINK_STATUS, &success);
	if (success == GL_FALSE) {
		char logbuffer[512];
		glGetShaderInfoLog(id, 512, nullptr, logbuffer);
		std::cout << logbuffer << "\n";
		return;
	}

	glDeleteShader(id);
}

