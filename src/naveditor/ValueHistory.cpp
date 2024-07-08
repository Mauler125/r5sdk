#include "Pch.h"
#include "NavEditor/Include/ValueHistory.h"

#ifdef WIN32
#	define snprintf _snprintf
#endif

ValueHistory::ValueHistory() :
	m_hsamples(0)
{
	for (int i = 0; i < MAX_HISTORY; ++i)
		m_samples[i] = 0;
}

float ValueHistory::getSampleMin() const
{
	float val = m_samples[0];
	for (int i = 1; i < MAX_HISTORY; ++i)
		if (m_samples[i] < val)
			val = m_samples[i];
	return val;
} 

float ValueHistory::getSampleMax() const
{
	float val = m_samples[0];
	for (int i = 1; i < MAX_HISTORY; ++i)
		if (m_samples[i] > val)
			val = m_samples[i];
	return val;
}

float ValueHistory::getAverage() const
{
	float val = 0;
	for (int i = 0; i < MAX_HISTORY; ++i)
		val += m_samples[i];
	return val/(float)MAX_HISTORY;
}

void GraphParams::setRect(int ix, int iy, int iw, int ih, int ipad)
{
	x = ix;
	y = iy;
	w = iw;
	h = ih;
	pad = ipad;
}

void GraphParams::setValueRange(float ivmin, float ivmax, int indiv, const char* iunits)
{
	vmin = ivmin;
	vmax = ivmax;
	ndiv = indiv;
	strcpy(units, iunits);
}

const static ImGuiWindowFlags s_graphWindowFlags = ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove;

void drawGraphBackground(const GraphParams* p)
{
	ImGui::Begin("GraphBackGround", nullptr, s_graphWindowFlags);
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	// BG
	drawList->AddRectFilled(
		ImVec2(static_cast<float>(p->x), static_cast<float>(p->y)),
		ImVec2(static_cast<float>(p->x + p->w), static_cast<float>(p->y + p->h)),
		IM_COL32(64, 64, 64, 128), 0.0f);

	const float sy = (p->h - p->pad * 2) / (p->vmax - p->vmin);
	const float oy = p->y + p->pad - p->vmin * sy;

	char text[64];

	// Divider Lines
	for (int i = 0; i <= p->ndiv; ++i)
	{
		const float u = static_cast<float>(i) / static_cast<float>(p->ndiv);
		const float v = p->vmin + (p->vmax - p->vmin) * u;
		snprintf(text, 64, "%.2f %s", v, p->units);
		float fy = oy + v * sy;
		drawList->AddText(
			ImVec2(static_cast<float>(p->x + p->w - p->pad), fy - 4),
			IM_COL32(0, 0, 0, 255), text);
		drawList->AddLine(
			ImVec2(static_cast<float>(p->x + p->pad), fy),
			ImVec2(static_cast<float>(p->x + p->w - p->pad - 50), fy),
			IM_COL32(0, 0, 0, 64), 1.0f);
	}

	ImGui::End();
}

void drawGraph(const GraphParams* p, const ValueHistory* graph,
	int idx, const char* label, const unsigned int col)
{
	ImGui::Begin("Graph", nullptr, s_graphWindowFlags);
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	const float sx = (p->w - p->pad * 2) / static_cast<float>(graph->getSampleCount());
	const float sy = (p->h - p->pad * 2) / (p->vmax - p->vmin);
	const float ox = static_cast<float>(p->x) + static_cast<float>(p->pad);
	const float oy = static_cast<float>(p->y) + static_cast<float>(p->pad) - p->vmin * sy;

	// Values
	float px = 0, py = 0;
	for (int i = 0; i < graph->getSampleCount() - 1; ++i) {
		const float x = ox + i * sx;
		const float y = oy + graph->getSample(i) * sy;
		if (i > 0)
			drawList->AddLine(
				ImVec2(px, py), ImVec2(x, y), col, 2.0f);
		px = x;
		py = y;
	}

	// Label
	const int size = 15;
	const int spacing = 10;
	int ix = p->x + p->w + 5;
	int iy = p->y + p->h - (idx + 1) * (size + spacing);

	drawList->AddRectFilled(
		ImVec2(static_cast<float>(ix), static_cast<float>(iy)),
		ImVec2(static_cast<float>(ix + size), static_cast<float>(iy + size)),
		col);

	char text[64];
	snprintf(text, 64, "%.2f %s", graph->getAverage(), p->units);
	drawList->AddText(
		ImVec2(static_cast<float>(ix + size + 5), static_cast<float>(iy + 3)),
		IM_COL32(255, 255, 255, 192), label);
	drawList->AddText(
		ImVec2(static_cast<float>(ix + size + 150), static_cast<float>(iy + 3)),
		IM_COL32(255, 255, 255, 128), text);

	ImGui::End(); // Zijn toch die enge dinge mannn
}
