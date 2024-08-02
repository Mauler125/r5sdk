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
	m_hitPosSet(0),
	m_bidir(true),
	m_jumpType(0),
	m_oldFlags(0)
{
}

OffMeshConnectionTool::~OffMeshConnectionTool()
{
	if (m_editor)
	{
		m_editor->setNavMeshDrawFlags(m_oldFlags);
	}
}

void OffMeshConnectionTool::init(Editor* editor)
{
	if (m_editor != editor)
	{
		m_editor = editor;
		m_oldFlags = m_editor->getNavMeshDrawFlags();
		m_editor->setNavMeshDrawFlags(m_oldFlags & ~DU_DRAWNAVMESH_OFFMESHCONS);
	}
}

void OffMeshConnectionTool::reset()
{
	m_hitPosSet = false;
}

void OffMeshConnectionTool::handleMenu()
{
	bool isOneWay = !m_bidir;

	if (ImGui::Checkbox("One Way", &isOneWay))
		m_bidir = false;

	bool isBiDirectional = m_bidir;

	if (ImGui::Checkbox("Bidirectional", &isBiDirectional))
		m_bidir = true;

	ImGui::PushItemWidth(140);
	ImGui::SliderInt("Jump Type", &m_jumpType, 0, 31);
	ImGui::PopItemWidth();
}

void OffMeshConnectionTool::handleClick(const float* /*s*/, const float* p, bool shift)
{
	if (!m_editor) return;
	InputGeom* geom = m_editor->getInputGeom();
	if (!geom) return;

	if (shift)
	{
		// Delete
		// Find nearest link end-point
		float nearestDist = FLT_MAX;
		int nearestIndex = -1;
		const float* verts = geom->getOffMeshConnectionVerts();
		for (int i = 0; i < geom->getOffMeshConnectionCount()*2; ++i)
		{
			const float* v = &verts[i*3];
			float d = rdVdistSqr(p, v);
			if (d < nearestDist)
			{
				nearestDist = d;
				nearestIndex = i/2; // Each link has two vertices.
			}
		}
		// If end point close enough, delete it.
		if (nearestIndex != -1 &&
			sqrtf(nearestDist) < m_editor->getAgentRadius())
		{
			geom->deleteOffMeshConnection(nearestIndex);
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
			const unsigned char area = EDITOR_POLYAREA_JUMP;
			const unsigned short flags = EDITOR_POLYFLAGS_WALK;
			geom->addOffMeshConnection(m_hitPos, p, m_editor->getAgentRadius(), m_bidir ? 1 : 0, (unsigned char)m_jumpType, area, flags);
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
}

void OffMeshConnectionTool::handleRender()
{
	duDebugDraw& dd = m_editor->getDebugDraw();
	const float s = m_editor->getAgentRadius();
	
	if (m_hitPosSet)
		duDebugDrawCross(&dd, m_hitPos[0],m_hitPos[1],m_hitPos[2]+0.1f, s, duRGBA(0,0,0,128), 2.0f, nullptr);

	InputGeom* geom = m_editor->getInputGeom();
	if (geom)
		geom->drawOffMeshConnections(&dd, m_editor->getRecastDrawOffset(), true);
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
			ImVec2(280, 40), ImVec4(1.0f,1.0f,1.0f,0.75f), "LMB: Create new connection.  SHIFT+LMB: Delete existing connection, click close to start or end point.");
	}
	else
	{
		ImGui_RenderText(ImGuiTextAlign_e::kAlignLeft, 
			ImVec2(280, 40), ImVec4(1.0f,1.0f,1.0f,0.75f), "LMB: Set connection end point and finish.");
	}
}
