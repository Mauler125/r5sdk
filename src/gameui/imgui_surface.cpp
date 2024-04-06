#include "imgui_surface.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CImguiSurface::CImguiSurface()
	: m_surfaceLabel("Surface")
	, m_fadeAlpha(0.0f)
	, m_surfaceStyle(ImGuiStyle_t::NONE)
	, m_initialized(false)
	, m_activated(false)
	, m_reclaimFocus(false)
{}

//-----------------------------------------------------------------------------
// Purpose: fade surface in and out smoothly
//-----------------------------------------------------------------------------
void CImguiSurface::Animate()
{
	const float deltaTime = ImGui::GetIO().DeltaTime;
	const float animSpeed = m_activated 
		? IMGUI_SURFACE_FADE_ANIM_SPEED 
		: -IMGUI_SURFACE_FADE_ANIM_SPEED;

	m_fadeAlpha += animSpeed * deltaTime;
	m_fadeAlpha = ImClamp(m_fadeAlpha, 0.0f, 1.0f);

	// Reclaim focus the next time we activate the panel
	if (!m_activated)
		m_reclaimFocus = true;
}

//-----------------------------------------------------------------------------
// Purpose: sets the surface front-end style and initial positions/sizes
//-----------------------------------------------------------------------------
void CImguiSurface::SetStyleVar(const float width, const float height, 
	const float x, const float y)
{
	m_surfaceStyle = g_ImGuiConfig.InitStyle();

	ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_FirstUseEver);
	ImGui::SetWindowPos(ImVec2(x, y), ImGuiCond_FirstUseEver);
}
