//
// Copyright (c) 2009-2010 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#include "Recast/Include/Recast.h"
#include "DebugUtils/Include/RecastDebugDraw.h"
#include "DebugUtils/Include/DetourDebugDraw.h"
#include "NavEditor/Include/OffMeshConnectionTool.h"
#include "NavEditor/Include/InputGeom.h"
#include "NavEditor/Include/Editor.h"

#ifdef WIN32
#	define snprintf _snprintf
#endif

OffMeshConnectionTool::OffMeshConnectionTool() :
	m_editor(0),
	m_lastSelectedAgentRadius(0),
	m_radius(0),
	m_hitPosSet(0),
	m_bidir(true),
	m_invertVertexLookupOrder(false),
	m_traverseType(0),
	m_oldFlags(0),
	m_selectedOffMeshIndex(-1),
	m_copiedOffMeshIndex(-1)
{
	rdVset(m_hitPos, 0.0f,0.0f,0.0f);
	rdVset(m_refOffset, 0.0f,0.0f,0.0f);
	memset(&m_copyOffMeshInstance, 0, sizeof(OffMeshConnection));
}

OffMeshConnectionTool::~OffMeshConnectionTool()
{
	if (m_editor)
	{
		if (m_oldFlags & DU_DRAW_DETOURMESH_OFFMESHCONS)
		{
			const unsigned int curFlags = m_editor->getNavMeshDrawFlags();
			m_editor->setNavMeshDrawFlags(curFlags | DU_DRAW_DETOURMESH_OFFMESHCONS);
		}
	}
}

void OffMeshConnectionTool::init(Editor* editor)
{
	if (m_editor != editor)
	{
		m_editor = editor;
		m_oldFlags = m_editor->getNavMeshDrawFlags();
		m_editor->setNavMeshDrawFlags(m_oldFlags & ~DU_DRAW_DETOURMESH_OFFMESHCONS);

		const float agentRadius = m_editor->getAgentRadius();
		m_radius = agentRadius;
		m_lastSelectedAgentRadius = agentRadius;

		rdVset(m_refOffset, 0.0f,0.0f, agentRadius);
	}
}

void OffMeshConnectionTool::reset()
{
	const float agentRadius = m_editor->getAgentRadius();
	m_radius = agentRadius;
	m_lastSelectedAgentRadius = agentRadius;
	m_hitPosSet = false;
}

#define VALUE_ADJUST_WINDOW 200

void OffMeshConnectionTool::renderModifyMenu()
{
	InputGeom* geom = m_editor->getInputGeom();
	if (!geom) return;

	ImGui::Separator();
	ImGui::Text("Modify Off-Mesh Connection");

	ImGui::SliderInt("Selected##OffMeshConnectionModify", &m_selectedOffMeshIndex, -1, geom->getOffMeshConnectionCount()-1);

	if (m_selectedOffMeshIndex == -1)
		return;

	float* verts = &geom->getOffMeshConnectionVerts()[m_selectedOffMeshIndex*6];
	float* refs = &geom->getOffMeshConnectionRefPos()[m_selectedOffMeshIndex*3];
	float& rad = geom->getOffMeshConnectionRads()[m_selectedOffMeshIndex];
	float& yaw = geom->getOffMeshConnectionRefYaws()[m_selectedOffMeshIndex];
	unsigned char& dir = geom->getOffMeshConnectionDirs()[m_selectedOffMeshIndex];
	unsigned char& jump = geom->getOffMeshConnectionJumps()[m_selectedOffMeshIndex];
	unsigned char& order = geom->getOffMeshConnectionOrders()[m_selectedOffMeshIndex];
	unsigned char& area = geom->getOffMeshConnectionAreas()[m_selectedOffMeshIndex];
	unsigned short& flags = geom->getOffMeshConnectionFlags()[m_selectedOffMeshIndex];

	if (m_copiedOffMeshIndex != m_selectedOffMeshIndex)
	{
		rdVcopy(&m_copyOffMeshInstance.pos[0], &verts[0]);
		rdVcopy(&m_copyOffMeshInstance.pos[3], &verts[3]);
		rdVcopy(m_copyOffMeshInstance.refPos, refs);

		rdVset(m_refOffset, 0.f,0.f,rad);

		m_copyOffMeshInstance.rad = rad;
		m_copyOffMeshInstance.refYaw = yaw;
		m_copyOffMeshInstance.dir = dir;
		m_copyOffMeshInstance.jump = jump;
		m_copyOffMeshInstance.order = order;
		m_copyOffMeshInstance.area = area;
		m_copyOffMeshInstance.flags = flags;

		m_copiedOffMeshIndex = m_selectedOffMeshIndex;
	}

	ImGui::PushItemWidth(60);

	ImGui::SliderFloat("##OffMeshConnectionModifyStartX", &verts[0], m_copyOffMeshInstance.pos[0]-VALUE_ADJUST_WINDOW, m_copyOffMeshInstance.pos[0]+VALUE_ADJUST_WINDOW);
	ImGui::SameLine();
	ImGui::SliderFloat("##OffMeshConnectionModifyStartY", &verts[1], m_copyOffMeshInstance.pos[1]-VALUE_ADJUST_WINDOW, m_copyOffMeshInstance.pos[1]+VALUE_ADJUST_WINDOW);
	ImGui::SameLine();
	ImGui::SliderFloat("##OffMeshConnectionModifyStartZ", &verts[2], m_copyOffMeshInstance.pos[2]-VALUE_ADJUST_WINDOW, m_copyOffMeshInstance.pos[2]+VALUE_ADJUST_WINDOW);
	ImGui::SameLine();
	ImGui::Text("Start");

	ImGui::SliderFloat("##OffMeshConnectionModifyEndX", &verts[3], m_copyOffMeshInstance.pos[3]-VALUE_ADJUST_WINDOW, m_copyOffMeshInstance.pos[3]+VALUE_ADJUST_WINDOW);
	ImGui::SameLine();
	ImGui::SliderFloat("##OffMeshConnectionModifyEndY", &verts[4], m_copyOffMeshInstance.pos[4]-VALUE_ADJUST_WINDOW, m_copyOffMeshInstance.pos[4]+VALUE_ADJUST_WINDOW);
	ImGui::SameLine();
	ImGui::SliderFloat("##OffMeshConnectionModifyEndZ", &verts[5], m_copyOffMeshInstance.pos[5]-VALUE_ADJUST_WINDOW, m_copyOffMeshInstance.pos[5]+VALUE_ADJUST_WINDOW);
	ImGui::SameLine();
	ImGui::Text("End");

	ImGui::SliderFloat("##OffMeshConnectionModifyRefX", &refs[0], m_copyOffMeshInstance.refPos[0]-VALUE_ADJUST_WINDOW, m_copyOffMeshInstance.refPos[0]+VALUE_ADJUST_WINDOW);
	ImGui::SameLine();
	ImGui::SliderFloat("##OffMeshConnectionModifyRefY", &refs[1], m_copyOffMeshInstance.refPos[1]-VALUE_ADJUST_WINDOW, m_copyOffMeshInstance.refPos[1]+VALUE_ADJUST_WINDOW);
	ImGui::SameLine();
	ImGui::SliderFloat("##OffMeshConnectionModifyRefZ", &refs[2], m_copyOffMeshInstance.refPos[2]-VALUE_ADJUST_WINDOW, m_copyOffMeshInstance.refPos[2]+VALUE_ADJUST_WINDOW);
	ImGui::SameLine();
	ImGui::Text("Ref");

	ImGui::PopItemWidth();

	// On newer navmesh sets, off-mesh links are always bidirectional.
#if DT_NAVMESH_SET_VERSION < 7
	ImGui::Checkbox("Bidirectional##OffMeshConnectionModify", (bool*)&dir);
#endif

	ImGui::SliderFloat("Radius##OffMeshConnectionModify", &rad, 0, 512);
	ImGui::SliderFloat("Yaw##OffMeshConnectionModify", &yaw, -180, 180);

	int traverseType = jump;
	ImGui::SliderInt("Jump##OffMeshConnectionModify", &traverseType, 0, DT_MAX_TRAVERSE_TYPES-1, "%d", ImGuiSliderFlags_NoInput);

	if (traverseType != jump)
		jump = (unsigned char)traverseType;

	ImGui::PushItemWidth(60);
	ImGui::SliderFloat("##OffMeshConnectionModifyRefOffsetX", &m_refOffset[0], -VALUE_ADJUST_WINDOW, VALUE_ADJUST_WINDOW);
	ImGui::SameLine();
	ImGui::SliderFloat("##OffMeshConnectionModifyRefOffsetY", &m_refOffset[1], -VALUE_ADJUST_WINDOW, VALUE_ADJUST_WINDOW);
	ImGui::SameLine();
	ImGui::SliderFloat("##OffMeshConnectionModifyRefOffsetZ", &m_refOffset[2], -VALUE_ADJUST_WINDOW, VALUE_ADJUST_WINDOW);
	ImGui::SameLine();
	ImGui::Text("Ref Offset");
	ImGui::PopItemWidth();

	if (ImGui::Button("Recalculate Reference##OffMeshConnectionModify"))
	{
		yaw = dtCalcOffMeshRefYaw(&verts[0], &verts[3]);
		dtCalcOffMeshRefPos(verts, yaw, m_refOffset, refs);
	}

	if (ImGui::Button("Reset Connection##OffMeshConnectionModify"))
	{
		rdVcopy(&verts[0], &m_copyOffMeshInstance.pos[0]);
		rdVcopy(&verts[3], &m_copyOffMeshInstance.pos[3]);
		rdVcopy(refs, m_copyOffMeshInstance.refPos);

		rad = m_copyOffMeshInstance.rad;
		yaw = m_copyOffMeshInstance.refYaw;
		dir = m_copyOffMeshInstance.dir;
		jump = m_copyOffMeshInstance.jump;
		order = m_copyOffMeshInstance.order;
		area = m_copyOffMeshInstance.area;
		flags = m_copyOffMeshInstance.flags;
	}

	if (ImGui::Button("Delete Connection##OffMeshConnectionModify"))
	{
		geom->deleteOffMeshConnection(m_selectedOffMeshIndex);
		m_selectedOffMeshIndex = -1;
		m_copiedOffMeshIndex = -1;

		return;
	}
}

void OffMeshConnectionTool::handleMenu()
{
	ImGui::Text("Create Off-Mesh Connection");

	// On newer navmesh sets, off-mesh links are always bidirectional.
#if DT_NAVMESH_SET_VERSION < 7
	ImGui::Checkbox("Bidirectional##OffMeshConnectionCreate", &m_bidir);
#endif
	ImGui::Checkbox("Invert Lookup Order##OffMeshConnectionCreate", &m_invertVertexLookupOrder);

	ImGui::PushItemWidth(140);
	ImGui::SliderInt("Jump##OffMeshConnectionCreate", &m_traverseType, 0, DT_MAX_TRAVERSE_TYPES-1, "%d", ImGuiSliderFlags_NoInput);
	ImGui::SliderFloat("Radius##OffMeshConnectionCreate", &m_radius, 0, 512);
	ImGui::PopItemWidth();

	renderModifyMenu();
}

void OffMeshConnectionTool::handleClick(const float* /*s*/, const float* p, const int /*v*/, bool shift)
{
	if (!m_editor) return;
	InputGeom* geom = m_editor->getInputGeom();
	if (!geom) return;

	if (shift)
	{
		// Select
		// Find nearest link end-point
		float nearestDist = FLT_MAX;
		int nearestIndex = -1;
		const float* verts = geom->getOffMeshConnectionVerts();
		for (int i = 0; i < geom->getOffMeshConnectionCount()*2; ++i)
		{
			const float* v = &verts[i*3];
			float d = rdVdist2DSqr(p, v);
			if (d < nearestDist)
			{
				nearestDist = d;
				nearestIndex = i/2; // Each link has two vertices.
			}
		}
		// If end point close enough, select it it.
		if (nearestIndex != -1 &&
			rdMathSqrtf(nearestDist) < m_radius)
		{
			m_selectedOffMeshIndex = nearestIndex;
		}
	}
	else
	{
		// Create	
		if (!m_hitPosSet)
		{
			rdVcopy(m_hitPos, p);
			m_hitPosSet = true;
		}
		else
		{
			const unsigned char area = DT_POLYAREA_JUMP;
			const unsigned short flags = DT_POLYFLAGS_WALK
#if DT_NAVMESH_SET_VERSION >= 7
				| DT_POLYFLAGS_JUMP;
#else
				;
#endif;
			m_selectedOffMeshIndex = geom->addOffMeshConnection(m_hitPos, p, m_radius, m_bidir ? 1 : 0,
				(unsigned char)m_traverseType, m_invertVertexLookupOrder ? 1 : 0, area, flags);
			m_hitPosSet = false;
		}
	}
}

void OffMeshConnectionTool::handleToggle()
{
}

void OffMeshConnectionTool::handleStep()
{
}

void OffMeshConnectionTool::handleUpdate(const float /*dt*/)
{
	const float agentRadius = m_editor->getAgentRadius();

	if (m_lastSelectedAgentRadius < agentRadius || m_lastSelectedAgentRadius > agentRadius)
	{
		m_lastSelectedAgentRadius = agentRadius;
		m_radius = agentRadius;
	}
}

void OffMeshConnectionTool::handleRender()
{
	duDebugDraw& dd = m_editor->getDebugDraw();
	const float s = m_editor->getAgentRadius();
	
	if (m_hitPosSet)
		duDebugDrawCross(&dd, m_hitPos[0],m_hitPos[1],m_hitPos[2]+0.1f, s, duRGBA(0,0,0,128), 2.0f, nullptr);

	InputGeom* geom = m_editor->getInputGeom();
	if (geom)
		geom->drawOffMeshConnections(&dd, m_editor->getRecastDrawOffset(), m_selectedOffMeshIndex);
}

void OffMeshConnectionTool::handleRenderOverlay(double* proj, double* model, int* view)
{
	GLdouble x, y, z;
	const int h = view[3];
	
	// Draw start and end point labels
	if (m_hitPosSet && gluProject((GLdouble)m_hitPos[0], (GLdouble)m_hitPos[1], (GLdouble)m_hitPos[2],
								model, proj, view, &x, &y, &z))
	{
		ImGui_RenderText(ImGuiTextAlign_e::kAlignCenter, ImVec2((float)x, h-((float)y-25)), ImVec4(0,0,0,0.8f), "Start");
	}
	
	// Tool help
	if (!m_hitPosSet)
	{
		ImGui_RenderText(ImGuiTextAlign_e::kAlignLeft,
			ImVec2(300, 40), ImVec4(1.0f,1.0f,1.0f,0.75f), "LMB: Create new connection.  SHIFT+LMB: Delete existing connection, click close to start or end point.");
	}
	else
	{
		ImGui_RenderText(ImGuiTextAlign_e::kAlignLeft, 
			ImVec2(300, 40), ImVec4(1.0f,1.0f,1.0f,0.75f), "LMB: Set connection end point and finish.");
	}
}
