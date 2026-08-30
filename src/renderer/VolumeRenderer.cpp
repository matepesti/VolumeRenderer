#include "VolumeRenderer.h"
#include <vector>
#include "imgui.h"

void VolumeRenderer::init(const std::string& exeDir) {
	glm::vec2 tri1_bl = glm::vec2(-1.0f, -1.0f);
	glm::vec2 tri1_br = glm::vec2(1.0f, -1.0f);
	glm::vec2 tri1_tr = glm::vec2(1.0f, 1.0f);
	glm::vec2 tri2_bl = glm::vec2(-1.0f, -1.0f);
	glm::vec2 tri2_tr = glm::vec2(1.0f, 1.0f);
	glm::vec2 tri2_tl = glm::vec2(-1.0f, 1.0f);

	std::vector<glm::vec2> vertecies;
	vertecies.reserve(6);
	vertecies.push_back(tri1_bl);
	vertecies.push_back(tri1_br);
	vertecies.push_back(tri1_tr);
	vertecies.push_back(tri2_bl);
	vertecies.push_back(tri2_tr);
	vertecies.push_back(tri2_tl);

	glGenVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);

	glGenBuffers(1, &m_vbo);
	glBindBuffer(GL_ARRAY_BUFFER,m_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * vertecies.size(), vertecies.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

	glBindVertexArray(0);

	m_shader = Shader(exeDir + "/shaders/raycast.vert", exeDir + "/shaders/raycast.frag");
	m_transferFunction.init();
	m_smoothPass.init(exeDir + "/shaders/smooth.comp");
}

void VolumeRenderer::render(const Camera& cam, Volume& vol) {
	if (!vol.isValid())
		return;

	m_shader.use();

	m_shader.setMat4("uInvView", cam.getInverseViewMatrix());
	m_shader.setMat4("uInvProj", cam.getInverseProjectionMatrix());
	m_shader.setVec3("uCameraPos", cam.getPosition());
	m_shader.setInt("uVolume", 0);
	m_shader.setFloat("uStepSize", 0.005f);
	m_shader.setInt("uMaxSteps", 500);
	glm::vec3 halfSize = vol.getNormalizedSize() * 0.5f;
	m_shader.setVec3("uVolumeHalfSize", halfSize);
	m_shader.setInt("uTransferFunction", 1);
	m_transferFunction.bind(1);
	m_shader.setVec3("uLightDir", glm::normalize(glm::vec3(1.f, 1.f, 1.f)));
	m_shader.setVec3("uLightColor",glm::vec3(1.f,1.f,1.f));
	m_shader.setInt("uRenderMode", m_renderMode);
	m_shader.setFloat("uIsoValue", m_isoValue);
	
	glm::vec3 normalizedClip;
	switch (m_clipAxis) {
		case 0: normalizedClip = glm::vec3(1.f, 0.f, 0.f);
				break;
		case 1: normalizedClip = glm::vec3(0.f, 1.f, 0.f);
				break;
		case 2: normalizedClip = glm::vec3(0.f, 0.f, 1.f);
				break;
		default: normalizedClip = glm::vec3(0.f, 0.f, 0.f);
	}

	m_shader.setBool("uClipEnabled", m_clipEnabled);
	m_shader.setVec3("uClipNormal", normalizedClip);
	m_shader.setFloat("uClipOffset", m_clipOffset);

	GLuint volumeToRender = vol.getTextureID();

	if (m_smoothSigma > 0.01f) {
		if (m_smoothSigma != m_lastSmoothedSigma) {
			m_smoothedTexID = m_smoothPass.run(vol.getTextureID(),vol.getNx(),vol.getNy(),vol.getNz(),m_smoothSigma);
			m_lastSmoothedSigma = m_smoothSigma;
		}

		if (m_smoothedTexID != 0) {
			volumeToRender = m_smoothedTexID;
		}
	}
	else if (m_smoothedTexID != 0) {
		releaseSmoothingResources();
	}

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, volumeToRender);

	glBindVertexArray(m_vao);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

}

void VolumeRenderer::drawRenderControls(Camera& m_camera) {
	int currentMode = m_renderMode;

	ImGui::RadioButton("DVR", &m_renderMode, 0);
	ImGui::RadioButton("MIP", &m_renderMode, 1);
	ImGui::RadioButton("MinIP", &m_renderMode, 2);
	ImGui::RadioButton("Isosurface", &m_renderMode, 3);

	if (m_renderMode == 3) {
		ImGui::SliderFloat("Iso value", &m_isoValue, 0.0f, 1.0f);
	}

	ImGui::Separator();
	ImGui::Text("Clipping Plane");
	ImGui::Checkbox("Enable clip", &m_clipEnabled);
	if (m_clipEnabled) {
		
		ImGui::RadioButton("X axis", &m_clipAxis, 0);
		ImGui::RadioButton("Y axis", &m_clipAxis, 1);
		ImGui::RadioButton("Z axis", &m_clipAxis, 2);
		if (m_clipAxis != -1) {
			ImGui::SliderFloat("Offset", &m_clipOffset, -1.0f, 1.0f);
		}
	}

	ImGui::Separator();
	ImGui::Text("Camera presets");

	if(ImGui::Button("Axial")) {
		m_camera.setView(glm::vec3(0.f, -1.f, 0.f), glm::vec3(0.f, 0.f, -1.f));
	}
	ImGui::SameLine();
	if(ImGui::Button("Coronal")) {
		m_camera.setView(glm::vec3(0.f, 0.f, -1.f), glm::vec3(0.f, 1.f, 0.f));
	}
	ImGui::SameLine();
	if (ImGui::Button("Sagittal")) {
		m_camera.setView(glm::vec3(-1.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
	}

	if (m_clipEnabled) {
		ImGui::Text("Clip active (Shift+drag to reposition)");
		if (ImGui::Button("Clear clip")) {
			m_clipEnabled = false;
		}
	}

	ImGui::Separator();
	ImGui::Text("Smoothing");
	ImGui::SliderFloat("Sigma", &m_smoothSigma, 0.0f, 3.0f);

}

void VolumeRenderer::offsetClip(float worldDelta) {
	m_clipOffset += worldDelta;
}

void VolumeRenderer::setClipPlane(glm::vec3 normal, float offset) {
	m_clipNormal = normal;
	m_clipOffset = offset;
}	

void VolumeRenderer::resetSmoothing() {
	m_smoothSigma = 0.0f;
	releaseSmoothingResources();
}

void VolumeRenderer::releaseSmoothingResources() {
	m_smoothPass.release();
	m_smoothedTexID = 0;
	m_lastSmoothedSigma = -1.0f;
}