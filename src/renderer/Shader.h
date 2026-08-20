#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

class Shader {
private:
	GLuint m_programID = 0;
	GLuint compile(const std::string& path, GLenum type);
public:
	Shader() {}
	Shader(const std::string& vertPath, const std::string& fragPath);
	void compileCompute(const std::string& path);
	void use();
	void setInt(const std::string& name, int value);
	void setFloat(const std::string& name, float value);
	void setVec3(const std::string& name, glm::vec3 value);
	void setMat4(const std::string& name, glm::mat4 value);
	void setBool(const std::string& name, bool value);
};
