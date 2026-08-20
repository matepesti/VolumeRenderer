#define IMGUI_DEFINE_MATH_OPERATORS

#include "TransferFunction.h"
#include <algorithm>
#include <iostream>
#include <imgui.h>

void TransferFunction::init(){
	glGenTextures(1, &m_textureID);
	glBindTexture(GL_TEXTURE_1D, m_textureID);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

	ControlPoint c1 = { 0.0f, 0.0f, glm::vec3(0.0f, 0.0f, 0.0f) };
	ControlPoint c2 = { 0.4f, 0.0f, glm::vec3(0.0f, 0.0f, 0.0f) };
	ControlPoint c3 = { 0.5f, 0.5f, glm::vec3(1.0f, 0.5f, 0.0f) };
	ControlPoint c4 = { 1.0f, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f) };

	m_controlPoints.push_back(c1);
	m_controlPoints.push_back(c2);
	m_controlPoints.push_back(c3);
	m_controlPoints.push_back(c4);

	bake();
}

void TransferFunction::bake() {
	sortControlPoints();

	std::vector<float> texData;
	texData.resize(256 * 4);
	m_bakedData.resize(256 * 4);

	for (int i = 0; i < 256; i++) {
		float t = i / 255.0f;

		auto first_larger = std::upper_bound(m_controlPoints.begin(), m_controlPoints.end(), t,
			[](float val, const ControlPoint& cp) {
				return val < cp.pos;
			});

		auto last_smaller_equal = (first_larger != m_controlPoints.begin()) ? std::prev(first_larger) : m_controlPoints.end();

		ControlPoint right;
		ControlPoint left;
		bool noLeft = false;
		bool noRight = false;

		if (first_larger != m_controlPoints.end()) {
			right = {first_larger->pos,first_larger->opacity,first_larger->color};
		}
		else {
			noRight = true;
		}

		if (last_smaller_equal != m_controlPoints.end()) {
			left = { last_smaller_equal->pos,last_smaller_equal->opacity,last_smaller_equal->color };
		}
		else {
			std::cout << "No element is smaller than or equal to " << t << "\n";
			noLeft = true;
		}

		if (noRight) {
			right = { left };
		}
		if (noLeft) {
			left = { right };
		}

		float factor;

		if (left.pos == right.pos) {
			factor = 0.0f;
		}
		else {
			factor = (t - left.pos) / (right.pos - left.pos);
		}

		float r = glm::mix(left.color.r,right.color.r,factor);
		float g = glm::mix(left.color.g, right.color.g, factor);
		float b = glm::mix(left.color.b, right.color.b, factor);

		float mixedOpacity = glm::mix(left.opacity, right.opacity, factor);

		texData[i * 4 + 0] = r;
		texData[i * 4 + 1] = g;
		texData[i * 4 + 2] = b;
		texData[i * 4 + 3] = mixedOpacity;
	}

	m_bakedData = texData;

	glBindTexture(GL_TEXTURE_1D, m_textureID);
	glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA32F, TEXTURE_SIZE, 0, GL_RGBA, GL_FLOAT, texData.data());

	m_dirty = false;
}

void TransferFunction::sortControlPoints() {
	auto compareByLength = [&](const ControlPoint& a, const ControlPoint& b) -> bool {
		return a.pos < b.pos;
		};

	std::sort(m_controlPoints.begin(), m_controlPoints.end(), compareByLength);
}

void TransferFunction::drawUI() {
	if (m_dirty) bake();

	ImGui::Begin("Transfer Function");

	ImVec2 canvasPos = ImGui::GetCursorScreenPos();
	ImVec2 canvasSize = ImVec2(ImGui::GetContentRegionAvail().x, m_canvasHeight);

	ImDrawList* drawList = ImGui::GetWindowDrawList();

	drawList->AddRectFilled(canvasPos, canvasPos + canvasSize, IM_COL32(30, 30, 30, 255));

	if (!m_histogram.empty()) {
		float binWidth = canvasSize.x / 256.0f;
		for (int i = 0; i < 256; i++) {
			float barHeight = m_histogram[i] * canvasSize.y;
			ImVec2 barMin(canvasPos.x + i * binWidth,
				canvasPos.y + canvasSize.y - barHeight);
			ImVec2 barMax(canvasPos.x + (i + 1) * binWidth,
				canvasPos.y + canvasSize.y);
			drawList->AddRectFilled(barMin, barMax, IM_COL32(120, 120, 120, 100));
		}
	}

	for (int i = 0; i < (int)canvasSize.x; i++) {
		float t = i / canvasSize.x;
		int index = (int)(t * 255);
		index = std::clamp(index, 0, 255);
		float r = m_bakedData[index * 4 + 0];
		float g = m_bakedData[index * 4 + 1];
		float b = m_bakedData[index * 4 + 2];
		ImVec2 x1 = ImVec2(canvasPos.x + i, canvasPos.y);
		ImVec2 x2 = ImVec2(canvasPos.x + i, canvasPos.y + canvasSize.y);
		drawList->AddLine(x1, x2, IM_COL32(r * 255, g * 255, b * 255, 60), 1.0f);
	}

	for (int i = 1; i < m_controlPoints.size(); i++) {

		ControlPoint point1 = m_controlPoints[i - 1];
		ControlPoint point2 = m_controlPoints[i];

		float pixelXp1 = canvasPos.x + point1.pos * canvasSize.x;
		float pixelYp1 = canvasPos.y + (1.0 - point1.opacity) * canvasSize.y;

		float pixelXp2 = canvasPos.x + point2.pos * canvasSize.x;
		float pixelYp2 = canvasPos.y + (1.0 - point2.opacity) * canvasSize.y;

		ImVec2 p1(pixelXp1, pixelYp1);
		ImVec2 p2(pixelXp2, pixelYp2);

		drawList->AddLine(p1, p2, IM_COL32(255, 255, 255, 200), 2.0f);
	}

	for (int i = 0; i < m_controlPoints.size(); i++) {

		ControlPoint point1 = m_controlPoints[i];

		float pixelX = canvasPos.x + point1.pos * canvasSize.x;
		float pixelY = canvasPos.y + (1.0 - point1.opacity) * canvasSize.y;

		ImVec2 pixelPos(pixelX, pixelY);

		drawList->AddCircleFilled(pixelPos, 6.0f, IM_COL32(point1.color.r*255, point1.color.g*255, point1.color.b*255, 255));
		drawList->AddCircle(pixelPos, 6.0f, IM_COL32(255, 255, 255, 255));
	}

	ImGui::Text("Presets:");
	ImGui::SameLine();
	if (ImGui::Button("EM map")) loadPreset("em");
	ImGui::SameLine();
	if (ImGui::Button("CT bone")) loadPreset("ct_bone");
	ImGui::SameLine();
	if (ImGui::Button("CT soft tissue")) loadPreset("ct_soft");
	ImGui::SameLine();
	if (ImGui::Button("Default")) loadPreset("default");


	ImGui::SetCursorScreenPos(canvasPos);
	ImGui::InvisibleButton("canvas", canvasSize);
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
	ImGui::Button("##resize", ImVec2(ImGui::GetContentRegionAvail().x, 4.0f));
	ImGui::PopStyleColor();
	if (ImGui::IsItemActive()) {
		m_canvasHeight += ImGui::GetIO().MouseDelta.y;
		m_canvasHeight = std::clamp(m_canvasHeight, 80.0f, 400.0f);
	}
	if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
		ImVec2 mousePos = ImGui::GetMousePos();
		float t = (mousePos.x - canvasPos.x) / canvasSize.x;
		float opacity = 1.0f - (mousePos.y - canvasPos.y) / canvasSize.y;
		t = std::clamp(t, 0.f, 1.f);
		opacity = std::clamp(opacity, 0.f, 1.f);

		bool hitCircle = false;
		for (int i = 0; i < (int)m_controlPoints.size(); i++) {
			float cx = canvasPos.x + m_controlPoints[i].pos * canvasSize.x;
			float cy = canvasPos.y + (1.0f - m_controlPoints[i].opacity) * canvasSize.y;
			float dx = mousePos.x - cx;
			float dy = mousePos.y - cy;
			if (std::sqrt(dx * dx + dy * dy) < 8.0f) {
				m_selectedPoint = i;
				hitCircle = true;
				break;
			}
		}

		if (!hitCircle) {
			ControlPoint newPoint = { t, opacity, glm::vec3(1.0f, 1.0f, 1.0f) };
			m_controlPoints.push_back(newPoint);
			m_dirty = true;
			sortControlPoints();
		}
	}

	if(ImGui::IsItemHovered() && ImGui::IsItemClicked(ImGuiMouseButton_Right)) {

		ImVec2 mousePos = ImGui::GetMousePos();
		float t = (mousePos.x - canvasPos.x) / canvasSize.x;
		float opacity = 1.0 - (mousePos.y - canvasPos.y) / canvasSize.y;
		t = std::clamp(t, 0.f, 1.f);
		opacity = std::clamp(opacity, 0.f, 1.f);
		float nearest = std::fabs(m_controlPoints[0].pos - t);
		ControlPoint nearestPoint = m_controlPoints[0];
		int nearestidx = 0;

		for (int i = 1; i < m_controlPoints.size();i++) {
			float potentiallyNearest = std::fabs((m_controlPoints[i].pos - t));
			if (potentiallyNearest < nearest) {
				nearest = potentiallyNearest;
				nearestPoint = m_controlPoints[i];
				nearestidx = i;
			}
		}
		if (m_controlPoints.size() > 2) {
			m_controlPoints.erase(m_controlPoints.begin() + nearestidx);
			m_dirty = true;
			m_selectedPoint = -1;
		}
	}

	if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
		if (m_dragIndex == -1) {
			ImVec2 mousePos = ImGui::GetMousePos();
			float t = (mousePos.x - canvasPos.x) / canvasSize.x;
			float opacity = 1.0 - (mousePos.y - canvasPos.y) / canvasSize.y;
			t = std::clamp(t, 0.f, 1.f);
			opacity = std::clamp(opacity, 0.f, 1.f);
			float nearest = std::fabs(m_controlPoints[0].pos - t);
			ControlPoint nearestPoint = m_controlPoints[0];
			m_dragIndex = 0;

			for (int i = 1; i < m_controlPoints.size();i++) {
				float potentiallyNearest = std::fabs((m_controlPoints[i].pos - t));
				if (potentiallyNearest < nearest) {
					nearest = potentiallyNearest;
					nearestPoint = m_controlPoints[i];
					m_dragIndex = i;
				}
			}
		}

		if (m_dragIndex >= 0) {
			ImVec2 mousePos = ImGui::GetMousePos();
			float newT = ((mousePos.x - canvasPos.x) / canvasSize.x);
			newT = std::clamp(newT, 0.f, 1.f);
			float newOpacity = (1.0 - (mousePos.y - canvasPos.y) / canvasSize.y);
			newOpacity = std::clamp(newOpacity, 0.f, 1.f);
			m_controlPoints[m_dragIndex].pos = newT;
			m_controlPoints[m_dragIndex].opacity = newOpacity;
			m_dirty = true;
		}

	}
	if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
		if (m_dragIndex >= 0) {
			sortControlPoints();
		}
		m_dragIndex = -1;
	}

	if (m_selectedPoint >= 0 && m_selectedPoint < m_controlPoints.size()) {
		ImGui::Text("Point color:");
		float col[3] = { m_controlPoints[m_selectedPoint].color.r,m_controlPoints[m_selectedPoint].color.g,m_controlPoints[m_selectedPoint].color.b };
		if (ImGui::ColorPicker3("##pointcolor", col)) {
			m_controlPoints[m_selectedPoint].color = glm::vec3(col[0], col[1], col[2]);
			m_dirty = true;
		}
	}

	ImGui::End();
}

void TransferFunction::bind(int textureUnit) {
	glActiveTexture(GL_TEXTURE0 + textureUnit);
	glBindTexture(GL_TEXTURE_1D, m_textureID);
}

void TransferFunction::release() {
	if (m_textureID != 0) {
		glDeleteTextures(1, &m_textureID);
		m_textureID = 0;
	}
}

void TransferFunction::computeHistogram(const std::vector<float>& volumeData) {
	m_histogram.assign(256, 0.0f);

	for (float v : volumeData) {
		int bin = (int)(v * 255.0f);
		bin = std::clamp(bin, 0, 255);
		m_histogram[bin] += 1.0f;
	}

	float maxVal = *std::max_element(m_histogram.begin(), m_histogram.end());
	if (maxVal > 0.0f) {
		for (float& h : m_histogram) {
			h /= maxVal;
		}
	}
}

void TransferFunction::loadPreset(const std::string& name) {
	m_controlPoints.clear();

	if (name == "em") {
		m_controlPoints = {
			{0.0f,  0.0f, glm::vec3(0.0f, 0.0f, 0.0f)},
			{0.4f,  0.0f, glm::vec3(0.0f, 0.0f, 0.8f)},
			{0.55f, 0.3f, glm::vec3(1.0f, 0.5f, 0.0f)},
			{0.7f,  0.8f, glm::vec3(1.0f, 0.9f, 0.7f)},
			{1.0f,  1.0f, glm::vec3(1.0f, 1.0f, 1.0f)}
		};
	}
	else if (name == "ct_bone") {
		m_controlPoints = {
			{0.0f, 0.0f, glm::vec3(0.0f, 0.0f, 0.0f)},
			{0.5f, 0.0f, glm::vec3(0.0f, 0.0f, 0.0f)},
			{0.6f, 0.0f, glm::vec3(0.8f, 0.6f, 0.5f)},
			{0.7f, 0.4f, glm::vec3(1.0f, 0.9f, 0.8f)},
			{1.0f, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f)}
		};
	}
	else if (name == "ct_soft") {
		m_controlPoints = {
			{0.0f,  0.0f,  glm::vec3(0.0f, 0.0f, 0.0f)},
			{0.25f, 0.0f,  glm::vec3(0.8f, 0.3f, 0.2f)},
			{0.35f, 0.04f, glm::vec3(1.0f, 0.6f, 0.4f)},
			{0.5f,  0.08f, glm::vec3(1.0f, 0.8f, 0.6f)},
			{0.6f,  0.0f,  glm::vec3(1.0f, 1.0f, 1.0f)},
			{1.0f,  0.0f,  glm::vec3(1.0f, 1.0f, 1.0f)}
		};
	}
	else if (name == "default") {
		m_controlPoints = {
			{ 0.0f, 0.0f, glm::vec3(0.0f, 0.0f, 0.0f) },
			{ 0.4f, 0.0f, glm::vec3(0.0f, 0.0f, 0.0f) },
			{ 0.5f, 0.5f, glm::vec3(1.0f, 0.5f, 0.0f) },
			{ 1.0f, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f) }
		};
	}

	m_dirty = true;
}