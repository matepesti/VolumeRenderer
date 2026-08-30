#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "volume/Volume.h"
#include "camera/Camera.h"
#include "renderer/Shader.h"
#include "ui/TransferFunction.h"
#include "SmoothPass.h"

class VolumeRenderer {
private:
	GLuint m_vao;
	GLuint m_vbo;
	GLuint m_smoothedTexID = 0;
	Shader m_shader;
	TransferFunction m_transferFunction;
	SmoothPass m_smoothPass;
	float m_smoothSigma = 0.0f;
	float m_lastSmoothedSigma = -1.0f;
	int m_renderMode = 0;
	float m_isoValue = 0.3;
	bool m_clipEnabled = false;
	glm::vec3 m_clipNormal = glm::vec3(1.f, 0.f, 0.f);
	float m_clipOffset = 0.0;
	int m_clipAxis = -1;
public:
	VolumeRenderer() {}
	void init(const std::string& exeDir);
	void render(const Camera& cam, Volume& vol);
	TransferFunction& getTransferFunction() { return m_transferFunction; }
	bool isClipEnabled() const { return m_clipEnabled; }
	void drawRenderControls(Camera& m_camera);
	void offsetClip(float worldDelta);
	void setClipPlane(glm::vec3 normal, float offset);
	void resetSmoothing();
	void releaseSmoothingResources();

	const float getSmoothSigma() const { return m_smoothSigma; }
	const int getRenderMode() const { return m_renderMode; }
	const float getIsoValue() const { return m_isoValue; }
	const bool getClipEnabled() const { return m_clipEnabled; }
	const float getClipOffset() const { return m_clipOffset; }
	const int getClipAxis() const { return m_clipAxis; }
	void setSmoothSigma(float smoothSigma) { m_smoothSigma = smoothSigma; }
	void setRenderMode(int renderMode) { m_renderMode = renderMode; }
	void setClipEnabled(bool enabled) { m_clipEnabled = enabled; }
	void setIsoValue(float isoValue) { m_isoValue = isoValue; }
	void setClipOffset(float clipOffset) { m_clipOffset = clipOffset; }
	void setClipAxis(int clipAxis) { m_clipAxis = clipAxis; }
};
