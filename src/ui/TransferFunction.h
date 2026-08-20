#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <glad/glad.h>
#include <string>

struct ControlPoint {
	float pos;
	float opacity;
	glm::vec3 color;
};

class TransferFunction {
private:
	std::vector<ControlPoint> m_controlPoints;
	std::vector<float> m_bakedData;
	std::vector<float> m_histogram;
	GLuint m_textureID;
	bool m_dirty = true;
	int m_dragIndex = -1;
	int m_selectedPoint = -1;
	static const int TEXTURE_SIZE = 256;

	void bake();
	void sortControlPoints();

public:
	std::vector<ControlPoint>& getControlPoints() { return m_controlPoints; }
	void init();
	void drawUI();
	void bind(int textureUnit);
	void release();
	void computeHistogram(const std::vector<float>& volumeData);
	void loadPreset(const std::string& name);
};
