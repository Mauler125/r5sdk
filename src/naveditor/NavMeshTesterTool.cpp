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

#include "Pch.h"
#include "Shared/Include/SharedCommon.h"
#include "Recast/Include/Recast.h"
#include "Detour/Include/DetourNavMesh.h"
#include "Detour/Include/DetourNavMeshBuilder.h"
#include "DetourCrowd/Include/DetourPathCorridor.h"
#include "DebugUtils/Include/DetourDebugDraw.h"
#include "DebugUtils/Include/RecastDebugDraw.h"
#include "NavEditor/Include/NavMeshTesterTool.h"
#include "NavEditor/Include/Editor.h"

// Uncomment this to dump all the requests in stdout.
#define DUMP_REQS

// Returns a random number [0..1]
static float frand()
{
//	return ((float)(rand() & 0xffff)/(float)0xffff);
	return (float)rand()/(float)RAND_MAX;
}

inline bool inRange(const float* v1, const float* v2, const float r, const float h)
{
	const float dx = v2[0] - v1[0];
	const float dy = v2[1] - v1[1];
	const float dz = v2[2] - v1[2];
	return (dx*dx + dz*dz) < r*r && fabsf(dy) < h;
}

// This function checks if the path has a small U-turn, that is,
// a polygon further in the path is adjacent to the first polygon
// in the path. If that happens, a shortcut is taken.
// This can happen if the target (T) location is at tile boundary,
// and we're (S) approaching it parallel to the tile edge.
// The choice at the vertex can be arbitrary, 
//  +---+---+
//  |:::|:::|
//  +-S-+-T-+
//  |:::|   | <-- the step can end up in here, resulting U-turn path.
//  +---+---+
static int fixupShortcuts(dtPolyRef* path, int npath, dtNavMeshQuery* navQuery)
{
	if (npath < 3)
		return npath;

	// Get connected polygons
	static const int maxNeis = 16;
	dtPolyRef neis[maxNeis];
	int nneis = 0;

	const dtMeshTile* tile = 0;
	const dtPoly* poly = 0;
	if (dtStatusFailed(navQuery->getAttachedNavMesh()->getTileAndPolyByRef(path[0], &tile, &poly)))
		return npath;
	
	for (unsigned int k = poly->firstLink; k != DT_NULL_LINK; k = tile->links[k].next)
	{
		const dtLink* link = &tile->links[k];
		if (link->ref != 0)
		{
			if (nneis < maxNeis)
				neis[nneis++] = link->ref;
		}
	}

	// If any of the neighbour polygons is within the next few polygons
	// in the path, short cut to that polygon directly.
	static const int maxLookAhead = 6;
	int cut = 0;
	for (int i = rdMin(maxLookAhead, npath) - 1; i > 1 && cut == 0; i--) {
		for (int j = 0; j < nneis; j++)
		{
			if (path[i] == neis[j]) {
				cut = i;
				break;
			}
		}
	}
	if (cut > 1)
	{
		int offset = cut-1;
		npath -= offset;
		for (int i = 1; i < npath; i++)
			path[i] = path[i+offset];
	}

	return npath;
}

static bool getSteerTarget(dtNavMeshQuery* navQuery, const float* startPos, const float* endPos,
						   const float minTargetDist,
						   const dtPolyRef* path, const int pathSize,
						   float* steerPos, unsigned char& steerPosFlag, dtPolyRef& steerPosRef,
						   float* outPoints = 0, int* outPointCount = 0)							 
{
	// Find steer target.
	static const int MAX_STEER_POINTS = 3;
	float steerPath[MAX_STEER_POINTS*3];
	unsigned char steerPathFlags[MAX_STEER_POINTS];
	dtPolyRef steerPathPolys[MAX_STEER_POINTS];
	int nsteerPath = 0;
	navQuery->findStraightPath(startPos, endPos, path, pathSize,
							   steerPath, steerPathFlags, steerPathPolys, &nsteerPath, MAX_STEER_POINTS);
	if (!nsteerPath)
		return false;
		
	if (outPoints && outPointCount)
	{
		*outPointCount = nsteerPath;
		for (int i = 0; i < nsteerPath; ++i)
			rdVcopy(&outPoints[i*3], &steerPath[i*3]);
	}

	
	// Find vertex far enough to steer to.
	int ns = 0;
	while (ns < nsteerPath)
	{
		// Stop at Off-Mesh link or when point is further than slop away.
		if ((steerPathFlags[ns] & DT_STRAIGHTPATH_OFFMESH_CONNECTION) ||
			!inRange(&steerPath[ns*3], startPos, minTargetDist, 1000.0f))
			break;
		ns++;
	}
	// Failed to find good point to steer to.
	if (ns >= nsteerPath)
		return false;
	
	rdVcopy(steerPos, &steerPath[ns*3]);
	steerPos[2] = startPos[2];
	steerPosFlag = steerPathFlags[ns];
	steerPosRef = steerPathPolys[ns];
	
	return true;
}


NavMeshTesterTool::NavMeshTesterTool() :
	m_editor(0),
	m_navMesh(0),
	m_navQuery(0),
	m_pathFindStatus(DT_FAILURE),
	m_toolMode(TOOLMODE_PATHFIND_FOLLOW),
	m_traverseAnimType(ANIMTYPE_NONE),
	m_straightPathOptions(0),
	m_startRef(0),
	m_endRef(0),
	m_npolys(0),
	m_nstraightPath(0),
	m_nsmoothPath(0),
	m_nrandPoints(0),
	m_randPointsInCircle(false),
	m_hitResult(false),
	m_distanceToWall(0),
	m_sposSet(false),
	m_eposSet(false),
	m_pathIterNum(0),
	m_pathIterPolyCount(0),
	m_steerPointCount(0)
{
	m_filter.setIncludeFlags(EDITOR_POLYFLAGS_ALL ^ EDITOR_POLYFLAGS_DISABLED);
	m_filter.setExcludeFlags(0);

	m_polyPickExt[0] = 2;
	m_polyPickExt[1] = 2;
	m_polyPickExt[2] = 4;
	
	m_neighbourhoodRadius = 2.5f;
	m_randomRadius = 5.0f;
}

void NavMeshTesterTool::init(Editor* editor)
{
	m_editor = editor;
	m_navMesh = editor->getNavMesh();
	m_navQuery = editor->getNavMeshQuery();
	recalc();

	if (m_navQuery)
	{
		// Change costs.
		m_filter.setAreaCost(EDITOR_POLYAREA_GROUND, 1.0f);
		m_filter.setAreaCost(EDITOR_POLYAREA_WATER, 10.0f);
		m_filter.setAreaCost(EDITOR_POLYAREA_ROAD, 1.0f);
		m_filter.setAreaCost(EDITOR_POLYAREA_DOOR, 1.0f);
		m_filter.setAreaCost(EDITOR_POLYAREA_GRASS, 2.0f);
		m_filter.setAreaCost(EDITOR_POLYAREA_JUMP, 1.5f);
	}
	
	m_neighbourhoodRadius = editor->getAgentRadius() * 20.0f;
	m_randomRadius = editor->getAgentRadius() * 30.0f;
	m_traverseAnimType = NavMesh_GetFirstTraverseAnimTypeForType(editor->getLoadedNavMeshType());
}

void NavMeshTesterTool::handleMenu()
{
	bool isEnabled = m_toolMode == TOOLMODE_PATHFIND_FOLLOW;

	if (ImGui::Checkbox("Pathfind Follow", &isEnabled))
	{
		m_toolMode = TOOLMODE_PATHFIND_FOLLOW;
		recalc();
	}

	isEnabled = m_toolMode == TOOLMODE_PATHFIND_STRAIGHT;

	if (ImGui::Checkbox("Pathfind Straight", &isEnabled))
	{
		m_toolMode = TOOLMODE_PATHFIND_STRAIGHT;
		recalc();
	}

	if (m_toolMode == TOOLMODE_PATHFIND_STRAIGHT)
	{
		ImGui::Indent();
		ImGui::Text("Vertices at crossings");

		isEnabled = m_straightPathOptions == 0;

		if (ImGui::Checkbox("None", &isEnabled))
		{
			m_straightPathOptions = 0;
			recalc();
		}

		isEnabled = m_straightPathOptions == DT_STRAIGHTPATH_AREA_CROSSINGS;

		if (ImGui::Checkbox("Area", &isEnabled))
		{
			m_straightPathOptions = DT_STRAIGHTPATH_AREA_CROSSINGS;
			recalc();
		}

		isEnabled = m_straightPathOptions == DT_STRAIGHTPATH_ALL_CROSSINGS;

		if (ImGui::Checkbox("All", &isEnabled))
		{
			m_straightPathOptions = DT_STRAIGHTPATH_ALL_CROSSINGS;
			recalc();
		}

		ImGui::Unindent();
	}

	isEnabled = m_toolMode == TOOLMODE_PATHFIND_SLICED;

	if (ImGui::Checkbox("Pathfind Sliced", &isEnabled))
	{
		m_toolMode = TOOLMODE_PATHFIND_SLICED;
		recalc();
	}

	ImGui::Separator();

	isEnabled = m_toolMode == TOOLMODE_DISTANCE_TO_WALL;

	if (ImGui::Checkbox("Distance to Wall", &isEnabled))
	{
		m_toolMode = TOOLMODE_DISTANCE_TO_WALL;
		recalc();
	}

	isEnabled = m_toolMode == TOOLMODE_RAYCAST;

	if (ImGui::Checkbox("Raycast", &isEnabled))
	{
		m_toolMode = TOOLMODE_RAYCAST;
		recalc();
	}

	ImGui::Separator();

	isEnabled = m_toolMode == TOOLMODE_FIND_POLYS_IN_CIRCLE;

	if (ImGui::Checkbox("Find Polys in Circle", &isEnabled))
	{
		m_toolMode = TOOLMODE_FIND_POLYS_IN_CIRCLE;
		recalc();
	}

	isEnabled = m_toolMode == TOOLMODE_FIND_POLYS_IN_SHAPE;

	if (ImGui::Checkbox("Find Polys in Shape", &isEnabled))
	{
		m_toolMode = TOOLMODE_FIND_POLYS_IN_SHAPE;
		recalc();
	}

	isEnabled = m_toolMode == TOOLMODE_FIND_LOCAL_NEIGHBOURHOOD;

	if (ImGui::Checkbox("Find Local Neighbourhood", &isEnabled))
	{
		m_toolMode = TOOLMODE_FIND_LOCAL_NEIGHBOURHOOD;
		recalc();
	}

	ImGui::Separator();
	
	if (ImGui::Button("Set Random Start"))
	{
		dtStatus status = m_navQuery->findRandomPoint(&m_filter, frand, &m_startRef, m_spos);
		if (dtStatusSucceed(status))
		{
			m_sposSet = true;
			recalc();
		}
	}

	ImGui::BeginDisabled(!m_sposSet || !m_startRef);

	if (ImGui::Button("Set Random End"))
	{
		dtStatus status = m_navQuery->findRandomPointAroundCircle(m_startRef, m_spos, m_randomRadius, &m_filter, frand, &m_endRef, m_epos);
		if (dtStatusSucceed(status))
		{
			m_eposSet = true;
			recalc();
		}
	}

	ImGui::EndDisabled();

	ImGui::Separator();

	if (ImGui::Button("Make Random Points"))
	{
		m_randPointsInCircle = false;
		m_nrandPoints = 0;
		for (int i = 0; i < MAX_RAND_POINTS; i++)
		{
			float pt[3];
			dtPolyRef ref;
			dtStatus status = m_navQuery->findRandomPoint(&m_filter, frand, &ref, pt);
			if (dtStatusSucceed(status))
			{
				rdVcopy(&m_randPoints[m_nrandPoints*3], pt);
				m_nrandPoints++;
			}
		}
	}

	ImGui::BeginDisabled(!m_sposSet);

	if (ImGui::Button("Make Random Points Around"))
	{
		if (m_sposSet)
		{
			m_nrandPoints = 0;
			m_randPointsInCircle = true;
			for (int i = 0; i < MAX_RAND_POINTS; i++)
			{
				float pt[3];
				dtPolyRef ref;
				dtStatus status = m_navQuery->findRandomPointAroundCircle(m_startRef, m_spos, m_randomRadius, &m_filter, frand, &ref, pt);
				if (dtStatusSucceed(status))
				{
					rdVcopy(&m_randPoints[m_nrandPoints*3], pt);
					m_nrandPoints++;
				}
			}
		}
	}

	ImGui::EndDisabled();
	
	ImGui::Separator();

	ImGui::Text("Include Flags");

	ImGui::Indent();

	isEnabled = (m_filter.getIncludeFlags() & EDITOR_POLYFLAGS_WALK) != 0;

	if (ImGui::Checkbox("Walk##IncludeFlags", &isEnabled))
	{
		m_filter.setIncludeFlags(m_filter.getIncludeFlags() ^ EDITOR_POLYFLAGS_WALK);
		recalc();
	}

	isEnabled = (m_filter.getIncludeFlags() & EDITOR_POLYFLAGS_SWIM) != 0;

	if (ImGui::Checkbox("Swim##IncludeFlags", &isEnabled))
	{
		m_filter.setIncludeFlags(m_filter.getIncludeFlags() ^ EDITOR_POLYFLAGS_SWIM);
		recalc();
	}

	isEnabled = (m_filter.getIncludeFlags() & EDITOR_POLYFLAGS_DOOR) != 0;

	if (ImGui::Checkbox("Door##IncludeFlags", &isEnabled))
	{
		m_filter.setIncludeFlags(m_filter.getIncludeFlags() ^ EDITOR_POLYFLAGS_DOOR);
		recalc();
	}

	isEnabled = (m_filter.getIncludeFlags() & EDITOR_POLYFLAGS_JUMP) != 0;

	if (ImGui::Checkbox("Jump##IncludeFlags", &isEnabled))
	{
		m_filter.setIncludeFlags(m_filter.getIncludeFlags() ^ EDITOR_POLYFLAGS_JUMP);
		recalc();
	}
	ImGui::Unindent();

	ImGui::Separator();
	ImGui::Text("Exclude Flags");
	
	ImGui::Indent();

	isEnabled = (m_filter.getExcludeFlags() & EDITOR_POLYFLAGS_WALK) != 0;

	if (ImGui::Checkbox("Walk##ExcludeFlags", &isEnabled))
	{
		m_filter.setExcludeFlags(m_filter.getExcludeFlags() ^ EDITOR_POLYFLAGS_WALK);
		recalc();
	}

	isEnabled = (m_filter.getExcludeFlags() & EDITOR_POLYFLAGS_SWIM) != 0;

	if (ImGui::Checkbox("Swim##ExcludeFlags", &isEnabled))
	{
		m_filter.setExcludeFlags(m_filter.getExcludeFlags() ^ EDITOR_POLYFLAGS_SWIM);
		recalc();
	}

	isEnabled = (m_filter.getExcludeFlags() & EDITOR_POLYFLAGS_DOOR) != 0;

	if (ImGui::Checkbox("Door##ExcludeFlags", &isEnabled))
	{
		m_filter.setExcludeFlags(m_filter.getExcludeFlags() ^ EDITOR_POLYFLAGS_DOOR);
		recalc();
	}

	isEnabled = (m_filter.getExcludeFlags() & EDITOR_POLYFLAGS_JUMP) != 0;

	if (ImGui::Checkbox("Jump##ExcludeFlags", &isEnabled))
	{
		m_filter.setExcludeFlags(m_filter.getExcludeFlags() ^ EDITOR_POLYFLAGS_JUMP);
		recalc();
	}
	ImGui::Unindent();

	ImGui::Separator();
	ImGui::Text("Traverse Anim Type");

	ImGui::Indent();

	const NavMeshType_e loadedNavMeshType = m_editor->getLoadedNavMeshType();

	// TODO: perhaps clamp with m_nav->m_params.traversalTableCount? Technically a navmesh should 
	// contain all the traversal tables it supports, so if we crash the navmesh is technically corrupt.
	const int traverseTableCount = NavMesh_GetTraversalTableCountForNavMeshType(loadedNavMeshType);
	const TraverseAnimType_e baseType = NavMesh_GetFirstTraverseAnimTypeForType(loadedNavMeshType);

	for (int i = ANIMTYPE_NONE; i < traverseTableCount; i++)
	{
		const bool noAnimtype = i == ANIMTYPE_NONE;

		const TraverseAnimType_e animTypeIndex = noAnimtype ? ANIMTYPE_NONE : TraverseAnimType_e((int)baseType + i);
		const char* animtypeName = noAnimtype ? "none" : g_traverseAnimTypeNames[animTypeIndex];

		isEnabled = m_traverseAnimType == animTypeIndex;

		if (ImGui::Checkbox(animtypeName, &isEnabled))
		{
			m_traverseAnimType = animTypeIndex;
		}
	}

	ImGui::Unindent();
	ImGui::Separator();
}

void NavMeshTesterTool::handleClick(const float* /*s*/, const float* p, bool shift)
{
	if (!shift)
	{
		m_sposSet = true;
		rdVcopy(m_spos, p);
	}
	else
	{
		m_eposSet = true;
		rdVcopy(m_epos, p);
	}
	recalc();
}

void NavMeshTesterTool::handleStep()
{
}

void NavMeshTesterTool::handleToggle()
{
	// TODO: merge separate to a path iterator. Use same code in recalc() too.
	if (m_toolMode != TOOLMODE_PATHFIND_FOLLOW)
		return;
		
	if (!m_sposSet || !m_eposSet || !m_startRef || !m_endRef)
		return;

	const bool hasAnimType = m_traverseAnimType != ANIMTYPE_NONE;
	const int traversalTableIndex = hasAnimType
		? NavMesh_GetTraversalTableIndexForAnimType(m_traverseAnimType)
		: NULL;

	if (!m_navMesh->isGoalPolyReachable(m_startRef, m_endRef, !hasAnimType, traversalTableIndex))
	{
		printf("%s: end poly '%d' is unreachable from start poly '%d'\n", "m_navMesh->isGoalPolyReachable", m_startRef, m_endRef);

		m_npolys = 0;
		m_nsmoothPath = 0;
		return;
	}

	static const float STEP_SIZE = 10.0f;
	static const float SLOP = 2.0f;

	if (m_pathIterNum == 0)
	{
		m_navQuery->findPath(m_startRef, m_endRef, m_spos, m_epos, &m_filter, m_polys, &m_npolys, MAX_POLYS);
		m_nsmoothPath = 0;

		m_pathIterPolyCount = m_npolys;
		if (m_pathIterPolyCount)
			memcpy(m_pathIterPolys, m_polys, sizeof(dtPolyRef)*m_pathIterPolyCount); 
		
		if (m_pathIterPolyCount)
		{
			// Iterate over the path to find smooth path on the detail mesh surface.
			m_navQuery->closestPointOnPoly(m_startRef, m_spos, m_iterPos, 0);
			m_navQuery->closestPointOnPoly(m_pathIterPolys[m_pathIterPolyCount-1], m_epos, m_targetPos, 0);
			
			m_nsmoothPath = 0;
			
			rdVcopy(&m_smoothPath[m_nsmoothPath*3], m_iterPos);
			m_nsmoothPath++;
		}
	}
	
	rdVcopy(m_prevIterPos, m_iterPos);

	m_pathIterNum++;

	if (!m_pathIterPolyCount)
		return;

	if (m_nsmoothPath >= MAX_SMOOTH)
		return;

	// Move towards target a small advancement at a time until target reached or
	// when ran out of memory to store the path.

	// Find location to steer towards.
	float steerPos[3];
	unsigned char steerPosFlag;
	dtPolyRef steerPosRef;
		
	if (!getSteerTarget(m_navQuery, m_iterPos, m_targetPos, SLOP,
						m_pathIterPolys, m_pathIterPolyCount, steerPos, steerPosFlag, steerPosRef,
						m_steerPoints, &m_steerPointCount))
		return;
		
	rdVcopy(m_steerPos, steerPos);
	
	bool endOfPath = (steerPosFlag & DT_STRAIGHTPATH_END) ? true : false;
	bool offMeshConnection = (steerPosFlag & DT_STRAIGHTPATH_OFFMESH_CONNECTION) ? true : false;
		
	// Find movement delta.
	float delta[3], len;
	rdVsub(delta, steerPos, m_iterPos);
	len = sqrtf(rdVdot(delta,delta));
	// If the steer target is end of path or off-mesh link, do not move past the location.
	if ((endOfPath || offMeshConnection) && len < STEP_SIZE)
		len = 1;
	else
		len = STEP_SIZE / len;
	float moveTgt[3];
	rdVmad(moveTgt, m_iterPos, delta, len);
		
	// Move
	float result[3];
	dtPolyRef visited[16];
	int nvisited = 0;
	m_navQuery->moveAlongSurface(m_pathIterPolys[0], m_iterPos, moveTgt, &m_filter,
								 result, visited, &nvisited, 16);
	m_pathIterPolyCount = dtMergeCorridorStartMoved(m_pathIterPolys, m_pathIterPolyCount, MAX_POLYS, visited, nvisited);
	m_pathIterPolyCount = fixupShortcuts(m_pathIterPolys, m_pathIterPolyCount, m_navQuery);

	float h = 0;
	m_navQuery->getPolyHeight(m_pathIterPolys[0], result, &h);
	result[2] = h;
	rdVcopy(m_iterPos, result);
	
	// Handle end of path and off-mesh links when close enough.
	if (endOfPath && inRange(m_iterPos, steerPos, SLOP, 1.0f))
	{
		// Reached end of path.
		rdVcopy(m_iterPos, m_targetPos);
		if (m_nsmoothPath < MAX_SMOOTH)
		{
			rdVcopy(&m_smoothPath[m_nsmoothPath*3], m_iterPos);
			m_nsmoothPath++;
		}
		return;
	}
	else if (offMeshConnection && inRange(m_iterPos, steerPos, SLOP, 1.0f))
	{
		// Reached off-mesh connection.
		float startPos[3], endPos[3];
		
		// Advance the path up to and over the off-mesh connection.
		dtPolyRef prevRef = 0, polyRef = m_pathIterPolys[0];
		int npos = 0;
		while (npos < m_pathIterPolyCount && polyRef != steerPosRef)
		{
			prevRef = polyRef;
			polyRef = m_pathIterPolys[npos];
			npos++;
		}
		for (int i = npos; i < m_pathIterPolyCount; ++i)
			m_pathIterPolys[i-npos] = m_pathIterPolys[i];
		m_pathIterPolyCount -= npos;
				
		// Handle the connection.
		dtStatus status = m_navMesh->getOffMeshConnectionPolyEndPoints(prevRef, polyRef, startPos, endPos);
		if (dtStatusSucceed(status))
		{
			if (m_nsmoothPath < MAX_SMOOTH)
			{
				rdVcopy(&m_smoothPath[m_nsmoothPath*3], startPos);
				m_nsmoothPath++;
				// Hack to make the dotted path not visible during off-mesh connection.
				if (m_nsmoothPath & 1)
				{
					rdVcopy(&m_smoothPath[m_nsmoothPath*3], startPos);
					m_nsmoothPath++;
				}
			}
			// Move position at the other side of the off-mesh link.
			rdVcopy(m_iterPos, endPos);
			float eh = 0.0f;
			m_navQuery->getPolyHeight(m_pathIterPolys[0], m_iterPos, &eh);
			m_iterPos[2] = eh;
		}
	}
	
	// Store results.
	if (m_nsmoothPath < MAX_SMOOTH)
	{
		rdVcopy(&m_smoothPath[m_nsmoothPath*3], m_iterPos);
		m_nsmoothPath++;
	}

}

void NavMeshTesterTool::handleUpdate(const float /*dt*/)
{
	if (m_toolMode == TOOLMODE_PATHFIND_SLICED)
	{
		if (dtStatusInProgress(m_pathFindStatus))
		{
			m_pathFindStatus = m_navQuery->updateSlicedFindPath(1,0, &m_filter);
		}
		if (dtStatusSucceed(m_pathFindStatus))
		{
			m_navQuery->finalizeSlicedFindPath(m_polys, &m_npolys, MAX_POLYS, &m_filter);
			m_nstraightPath = 0;
			if (m_npolys)
			{
				// In case of partial path, make sure the end point is clamped to the last polygon.
				float epos[3];
				rdVcopy(epos, m_epos);
				if (m_polys[m_npolys-1] != m_endRef)
				m_navQuery->closestPointOnPoly(m_polys[m_npolys-1], m_epos, epos, 0);

				m_navQuery->findStraightPath(m_spos, epos, m_polys, m_npolys,
											 m_straightPath, m_straightPathFlags,
											 m_straightPathPolys, &m_nstraightPath, MAX_POLYS, DT_STRAIGHTPATH_ALL_CROSSINGS);
			}
			 
			m_pathFindStatus = DT_FAILURE;
		}
	}
}

void NavMeshTesterTool::reset()
{
	m_startRef = 0;
	m_endRef = 0;
	m_npolys = 0;
	m_nstraightPath = 0;
	m_nsmoothPath = 0;
	memset(m_hitPos, 0, sizeof(m_hitPos));
	memset(m_hitNormal, 0, sizeof(m_hitNormal));
	m_distanceToWall = 0;
}


void NavMeshTesterTool::recalc()
{
	if (!m_navMesh)
		return;
	
	if (m_sposSet)
		m_navQuery->findNearestPoly(m_spos, m_polyPickExt, &m_filter, &m_startRef, 0);
	else
		m_startRef = 0;
	
	if (m_eposSet)
		m_navQuery->findNearestPoly(m_epos, m_polyPickExt, &m_filter, &m_endRef, 0);
	else
		m_endRef = 0;
	
	m_pathFindStatus = DT_FAILURE;

	const bool everythingSet = m_sposSet && m_eposSet && m_startRef && m_endRef;
	bool isReachable = true;

	if (m_startRef && m_endRef)
	{
		const bool hasAnimType = m_traverseAnimType != ANIMTYPE_NONE;
		const int traversalTableIndex = hasAnimType
			? NavMesh_GetTraversalTableIndexForAnimType(m_traverseAnimType)
			: NULL;

		isReachable = m_navMesh->isGoalPolyReachable(m_startRef, m_endRef, !hasAnimType, traversalTableIndex);

		if (!isReachable)
			printf("%s: end poly '%d' is unreachable from start poly '%d'\n", "m_navMesh->isGoalPolyReachable", m_startRef, m_endRef);
	}
	
	if (m_toolMode == TOOLMODE_PATHFIND_FOLLOW)
	{
		m_pathIterNum = 0;
		if (everythingSet && isReachable)
		{
#ifdef DUMP_REQS
			printf("pi  %f %f %f  %f %f %f  0x%x 0x%x\n",
				   m_spos[0],m_spos[1],m_spos[2], m_epos[0],m_epos[1],m_epos[2],
				   m_filter.getIncludeFlags(), m_filter.getExcludeFlags()); 
#endif

			m_navQuery->findPath(m_startRef, m_endRef, m_spos, m_epos, &m_filter, m_polys, &m_npolys, MAX_POLYS);

			m_nsmoothPath = 0;

			if (m_npolys)
			{
				// Iterate over the path to find smooth path on the detail mesh surface.
				dtPolyRef polys[MAX_POLYS];
				memcpy(polys, m_polys, sizeof(dtPolyRef)*m_npolys); 
				int npolys = m_npolys;
				
				float iterPos[3], targetPos[3];
				m_navQuery->closestPointOnPoly(m_startRef, m_spos, iterPos, 0);
				m_navQuery->closestPointOnPoly(polys[npolys-1], m_epos, targetPos, 0);
				
				static const float STEP_SIZE = 40.0f;
				static const float SLOP = 2.0f;
				
				m_nsmoothPath = 0;
				
				rdVcopy(&m_smoothPath[m_nsmoothPath*3], iterPos);
				m_nsmoothPath++;
				
				// Move towards target a small advancement at a time until target reached or
				// when ran out of memory to store the path.
				while (npolys && m_nsmoothPath < MAX_SMOOTH)
				{
					// Find location to steer towards.
					float steerPos[3];
					unsigned char steerPosFlag;
					dtPolyRef steerPosRef;
					
					if (!getSteerTarget(m_navQuery, iterPos, targetPos, SLOP,
										polys, npolys, steerPos, steerPosFlag, steerPosRef))
						break;
					
					bool endOfPath = (steerPosFlag & DT_STRAIGHTPATH_END) ? true : false;
					bool offMeshConnection = (steerPosFlag & DT_STRAIGHTPATH_OFFMESH_CONNECTION) ? true : false;
					
					// Find movement delta.
					float delta[3], len;
					rdVsub(delta, steerPos, iterPos);
					len = rdMathSqrtf(rdVdot(delta, delta));
					// If the steer target is end of path or off-mesh link, do not move past the location.
					if ((endOfPath || offMeshConnection) && len < STEP_SIZE)
						len = 1;
					else
						len = STEP_SIZE / len;
					float moveTgt[3];
					rdVmad(moveTgt, iterPos, delta, len);
					
					// Move
					float result[3];
					dtPolyRef visited[16];
					int nvisited = 0;
					m_navQuery->moveAlongSurface(polys[0], iterPos, moveTgt, &m_filter,
												 result, visited, &nvisited, 16);

					npolys = dtMergeCorridorStartMoved(polys, npolys, MAX_POLYS, visited, nvisited);
					npolys = fixupShortcuts(polys, npolys, m_navQuery);

					float h = 0;
					m_navQuery->getPolyHeight(polys[0], result, &h);
					result[2] = h;
					rdVcopy(iterPos, result);

					// Handle end of path and off-mesh links when close enough.
					if (endOfPath && inRange(iterPos, steerPos, SLOP, 1.0f))
					{
						// Reached end of path.
						rdVcopy(iterPos, targetPos);
						if (m_nsmoothPath < MAX_SMOOTH)
						{
							rdVcopy(&m_smoothPath[m_nsmoothPath*3], iterPos);
							m_nsmoothPath++;
						}
						break;
					}
					else if (offMeshConnection && inRange(iterPos, steerPos, SLOP, 1.0f))
					{
						// Reached off-mesh connection.
						float startPos[3], endPos[3];
						
						// Advance the path up to and over the off-mesh connection.
						dtPolyRef prevRef = 0, polyRef = polys[0];
						int npos = 0;
						while (npos < npolys && polyRef != steerPosRef)
						{
							prevRef = polyRef;
							polyRef = polys[npos];
							npos++;
						}
						for (int i = npos; i < npolys; ++i)
							polys[i-npos] = polys[i];
						npolys -= npos;
						
						// Handle the connection.
						dtStatus status = m_navMesh->getOffMeshConnectionPolyEndPoints(prevRef, polyRef, startPos, endPos);
						if (dtStatusSucceed(status))
						{
							if (m_nsmoothPath < MAX_SMOOTH)
							{
								rdVcopy(&m_smoothPath[m_nsmoothPath*3], startPos);
								m_nsmoothPath++;
								// Hack to make the dotted path not visible during off-mesh connection.
								if (m_nsmoothPath & 1)
								{
									rdVcopy(&m_smoothPath[m_nsmoothPath*3], startPos);
									m_nsmoothPath++;
								}
							}
							// Move position at the other side of the off-mesh link.
							rdVcopy(iterPos, endPos);
							float eh = 0.0f;
							m_navQuery->getPolyHeight(polys[0], iterPos, &eh);
							iterPos[2] = eh;
						}
					}
					
					// Store results.
					if (m_nsmoothPath < MAX_SMOOTH)
					{
						rdVcopy(&m_smoothPath[m_nsmoothPath*3], iterPos);
						m_nsmoothPath++;
					}
				}
			}

		}
		else
		{
			m_npolys = 0;
			m_nsmoothPath = 0;
		}
	}
	else if (m_toolMode == TOOLMODE_PATHFIND_STRAIGHT)
	{
		if (everythingSet && isReachable)
		{
#ifdef DUMP_REQS
			printf("ps  %f %f %f  %f %f %f  0x%x 0x%x\n",
				   m_spos[0],m_spos[1],m_spos[2], m_epos[0],m_epos[1],m_epos[2],
				   m_filter.getIncludeFlags(), m_filter.getExcludeFlags()); 
#endif
			m_navQuery->findPath(m_startRef, m_endRef, m_spos, m_epos, &m_filter, m_polys, &m_npolys, MAX_POLYS);
			m_nstraightPath = 0;
			if (m_npolys)
			{
				// In case of partial path, make sure the end point is clamped to the last polygon.
				float epos[3];
				rdVcopy(epos, m_epos);
				if (m_polys[m_npolys-1] != m_endRef)
					m_navQuery->closestPointOnPoly(m_polys[m_npolys-1], m_epos, epos, 0);
				
				m_navQuery->findStraightPath(m_spos, epos, m_polys, m_npolys,
											 m_straightPath, m_straightPathFlags,
											 m_straightPathPolys, &m_nstraightPath, MAX_POLYS, m_straightPathOptions);
			}
		}
		else
		{
			m_npolys = 0;
			m_nstraightPath = 0;
		}
	}
	else if (m_toolMode == TOOLMODE_PATHFIND_SLICED)
	{
		if (everythingSet && isReachable)
		{
#ifdef DUMP_REQS
			printf("ps  %f %f %f  %f %f %f  0x%x 0x%x\n",
				   m_spos[0],m_spos[1],m_spos[2], m_epos[0],m_epos[1],m_epos[2],
				   m_filter.getIncludeFlags(), m_filter.getExcludeFlags()); 
#endif
			m_npolys = 0;
			m_nstraightPath = 0;
			
			m_pathFindStatus = m_navQuery->initSlicedFindPath(m_startRef, m_endRef, m_spos, m_epos, DT_FINDPATH_ANY_ANGLE);
		}
		else
		{
			m_npolys = 0;
			m_nstraightPath = 0;
		}
	}
	else if (m_toolMode == TOOLMODE_RAYCAST)
	{
		m_nstraightPath = 0;
		if (m_sposSet && m_eposSet && m_startRef && isReachable)
		{
#ifdef DUMP_REQS
			printf("rc  %f %f %f  %f %f %f  0x%x 0x%x\n",
				   m_spos[0],m_spos[1],m_spos[2], m_epos[0],m_epos[1],m_epos[2],
				   m_filter.getIncludeFlags(), m_filter.getExcludeFlags()); 
#endif
			float t = 0;
			m_npolys = 0;
			m_nstraightPath = 2;
			m_straightPath[0] = m_spos[0];
			m_straightPath[1] = m_spos[1];
			m_straightPath[2] = m_spos[2];
			m_navQuery->raycast(m_startRef, m_spos, m_epos, &m_filter, &t, m_hitNormal, m_polys, &m_npolys, MAX_POLYS);
			if (t > 1)
			{
				// No hit
				rdVcopy(m_hitPos, m_epos);
				m_hitResult = false;
			}
			else
			{
				// Hit
				rdVlerp(m_hitPos, m_spos, m_epos, t);
				m_hitResult = true;
			}
			// Adjust height.
			if (m_npolys > 0)
			{
				float h = 0;
				m_navQuery->getPolyHeight(m_polys[m_npolys-1], m_hitPos, &h);
				m_hitPos[2] = h;
			}
			rdVcopy(&m_straightPath[3], m_hitPos);
		}
	}
	else if (m_toolMode == TOOLMODE_DISTANCE_TO_WALL)
	{
		m_distanceToWall = 0;
		if (m_sposSet && m_startRef)
		{
#ifdef DUMP_REQS
			printf("dw  %f %f %f  %f  0x%x 0x%x\n",
				   m_spos[0],m_spos[1],m_spos[2], 100.0f,
				   m_filter.getIncludeFlags(), m_filter.getExcludeFlags()); 
#endif
			m_distanceToWall = 0.0f;
			m_navQuery->findDistanceToWall(m_startRef, m_spos, 100.0f, &m_filter, &m_distanceToWall, m_hitPos, m_hitNormal);
		}
	}
	else if (m_toolMode == TOOLMODE_FIND_POLYS_IN_CIRCLE)
	{
		if (m_sposSet && m_startRef && m_eposSet)
		{
			const float dx = m_epos[0] - m_spos[0];
			const float dy = m_epos[1] - m_spos[1];
			float dist = sqrtf(dx*dx + dy*dy);
#ifdef DUMP_REQS
			printf("fpc  %f %f %f  %f  0x%x 0x%x\n",
				   m_spos[0],m_spos[1],m_spos[2], dist,
				   m_filter.getIncludeFlags(), m_filter.getExcludeFlags());
#endif
			m_navQuery->findPolysAroundCircle(m_startRef, m_spos, dist, &m_filter,
											  m_polys, m_parent, 0, &m_npolys, MAX_POLYS);
		}
	}
	else if (m_toolMode == TOOLMODE_FIND_POLYS_IN_SHAPE)
	{
		if (m_sposSet && m_startRef && m_eposSet)
		{
			const float nx = -(m_epos[1] - m_spos[1])*0.25f;
			const float ny = (m_epos[0] - m_spos[0])*0.25f;
			const float agentHeight = m_editor ? m_editor->getAgentHeight() : 0;

			m_queryPoly[0] = m_spos[0] + nx*1.2f;
			m_queryPoly[1] = m_spos[1] + ny*1.2f;
			m_queryPoly[2] = m_spos[2] + agentHeight/2;

			m_queryPoly[3] = m_spos[0] - nx*1.3f;
			m_queryPoly[4] = m_spos[1] - ny*1.3f;
			m_queryPoly[5] = m_spos[2] + agentHeight/2;

			m_queryPoly[6] = m_epos[0] - nx*0.8f;
			m_queryPoly[7] = m_epos[1] - ny*0.8f;
			m_queryPoly[8] = m_epos[2] + agentHeight/2;

			m_queryPoly[9] = m_epos[0] + nx;
			m_queryPoly[10] = m_epos[1] + ny;
			m_queryPoly[11] = m_epos[2] + agentHeight/2;

#ifdef DUMP_REQS
			printf("fpp  %f %f %f  %f %f %f  %f %f %f  %f %f %f  0x%x 0x%x\n",
				   m_queryPoly[0],m_queryPoly[1],m_queryPoly[2],
				   m_queryPoly[3],m_queryPoly[4],m_queryPoly[5],
				   m_queryPoly[6],m_queryPoly[7],m_queryPoly[8],
				   m_queryPoly[9],m_queryPoly[10],m_queryPoly[11],
				   m_filter.getIncludeFlags(), m_filter.getExcludeFlags());
#endif
			m_navQuery->findPolysAroundShape(m_startRef, m_queryPoly, 4, &m_filter,
											 m_polys, m_parent, 0, &m_npolys, MAX_POLYS);
		}
	}
	else if (m_toolMode == TOOLMODE_FIND_LOCAL_NEIGHBOURHOOD)
	{
		if (m_sposSet && m_startRef)
		{
#ifdef DUMP_REQS
			printf("fln  %f %f %f  %f  0x%x 0x%x\n",
				   m_spos[0],m_spos[1],m_spos[2], m_neighbourhoodRadius,
				   m_filter.getIncludeFlags(), m_filter.getExcludeFlags());
#endif
			m_navQuery->findLocalNeighbourhood(m_startRef, m_spos, m_neighbourhoodRadius, &m_filter,
											   m_polys, m_parent, &m_npolys, MAX_POLYS);
		}
	}
}

static void getPolyCenter(dtNavMesh* navMesh, dtPolyRef ref, float* center)
{
	center[0] = 0;
	center[1] = 0;
	center[2] = 0;
	
	const dtMeshTile* tile = 0;
	const dtPoly* poly = 0;
	dtStatus status = navMesh->getTileAndPolyByRef(ref, &tile, &poly);
	if (dtStatusFailed(status))
		return;

	rdVcopy(center, poly->center);
}



void NavMeshTesterTool::handleRender()
{
	duDebugDraw& dd = m_editor->getDebugDraw();
	
	static const unsigned int startCol = duRGBA(128,25,0,192);
	static const unsigned int endCol = duRGBA(51,102,0,129);
	static const unsigned int pathCol = duRGBA(0,0,0,64);
	
	const float agentRadius = m_editor->getAgentRadius();
	const float agentHeight = m_editor->getAgentHeight();
	const float agentClimb = m_editor->getAgentClimb();

	const unsigned int drawFlags = m_editor->getNavMeshDrawFlags();
	
	dd.depthMask(false);
	if (m_sposSet)
		drawAgent(m_spos, agentRadius, agentHeight, agentClimb, startCol);
	if (m_eposSet)
		drawAgent(m_epos, agentRadius, agentHeight, agentClimb, endCol);
	dd.depthMask(true);
	
	if (!m_navMesh)
	{
		return;
	}

	const float* drawOffset = m_editor->getDetourDrawOffset();

	if (m_toolMode == TOOLMODE_PATHFIND_FOLLOW)
	{
		duDebugDrawNavMeshPoly(&dd, *m_navMesh, m_startRef, drawOffset, drawFlags, startCol);
		duDebugDrawNavMeshPoly(&dd, *m_navMesh, m_endRef, drawOffset, drawFlags, endCol);
		
		if (m_npolys)
		{
			for (int i = 0; i < m_npolys; ++i)
			{
				if (m_polys[i] == m_startRef || m_polys[i] == m_endRef)
					continue;
				duDebugDrawNavMeshPoly(&dd, *m_navMesh, m_polys[i], drawOffset, drawFlags, pathCol);
			}
		}
				
		if (m_nsmoothPath)
		{
			dd.depthMask(false);
			const unsigned int spathCol = duRGBA(0,0,0,220);
			dd.begin(DU_DRAW_LINES, 3.0f, drawOffset);
			for (int i = 0; i < m_nsmoothPath; ++i)
				dd.vertex(m_smoothPath[i*3], m_smoothPath[i*3+1], m_smoothPath[i*3+2]+0.1f, spathCol);
			dd.end();
			dd.depthMask(true);
		}
		
		if (m_pathIterNum)
		{
			duDebugDrawNavMeshPoly(&dd, *m_navMesh, m_pathIterPolys[0], drawOffset, drawFlags, duRGBA(255,255,255,128));

			dd.depthMask(false);
			dd.begin(DU_DRAW_LINES, 1.0f, drawOffset);
			
			const unsigned int prevCol = duRGBA(255,192,0,220);
			const unsigned int curCol = duRGBA(255,255,255,220);
			const unsigned int steerCol = duRGBA(0,192,255,220);

			dd.vertex(m_prevIterPos[0],m_prevIterPos[1],m_prevIterPos[2]-0.3f, prevCol);
			dd.vertex(m_prevIterPos[0],m_prevIterPos[1],m_prevIterPos[2]+0.3f, prevCol);

			dd.vertex(m_iterPos[0],m_iterPos[1],m_iterPos[2]-0.3f, curCol);
			dd.vertex(m_iterPos[0],m_iterPos[1],m_iterPos[2]+0.3f, curCol);

			dd.vertex(m_prevIterPos[0],m_prevIterPos[1],m_prevIterPos[2]+0.3f,prevCol);
			dd.vertex(m_iterPos[0],m_iterPos[1],m_iterPos[2]+0.3f,prevCol);

			dd.vertex(m_prevIterPos[0],m_prevIterPos[1],m_prevIterPos[2]+0.3f,steerCol);
			dd.vertex(m_steerPos[0],m_steerPos[1],m_steerPos[2]+0.3f,steerCol);
			
			for (int i = 0; i < m_steerPointCount-1; ++i)
			{
				dd.vertex(m_steerPoints[i*3+0],m_steerPoints[i*3+1],m_steerPoints[i*3+2] + 0.2f, duDarkenCol(steerCol));
				dd.vertex(m_steerPoints[(i+1)*3+0],m_steerPoints[(i+1)*3+1],m_steerPoints[(i+1)*3+2]+0.2f,duDarkenCol(steerCol));
			}
			
			dd.end();
			dd.depthMask(true);
		}
	}
	else if (m_toolMode == TOOLMODE_PATHFIND_STRAIGHT ||
			 m_toolMode == TOOLMODE_PATHFIND_SLICED)
	{
		duDebugDrawNavMeshPoly(&dd, *m_navMesh, m_startRef, drawOffset, drawFlags, startCol);
		duDebugDrawNavMeshPoly(&dd, *m_navMesh, m_endRef, drawOffset, drawFlags, endCol);
		
		if (m_npolys)
		{
			for (int i = 0; i < m_npolys; ++i)
			{
				if (m_polys[i] == m_startRef || m_polys[i] == m_endRef)
					continue;
				duDebugDrawNavMeshPoly(&dd, *m_navMesh, m_polys[i], drawOffset, drawFlags, pathCol);
			}
		}
		
		if (m_nstraightPath)
		{
			dd.depthMask(false);
			const unsigned int spathCol = duRGBA(64,16,0,220);
			const unsigned int offMeshCol = duRGBA(128,96,0,220);
			dd.begin(DU_DRAW_LINES, 2.0f, drawOffset);
			for (int i = 0; i < m_nstraightPath-1; ++i)
			{
				unsigned int col;
				if (m_straightPathFlags[i] & DT_STRAIGHTPATH_OFFMESH_CONNECTION)
					col = offMeshCol;
				else
					col = spathCol;
				
				dd.vertex(m_straightPath[i*3], m_straightPath[i*3+1], m_straightPath[i*3+2] + 0.4f, col);
				dd.vertex(m_straightPath[(i+1)*3], m_straightPath[(i+1)*3+1], m_straightPath[(i+1)*3+2] + 0.4f, col);
			}
			dd.end();
			dd.begin(DU_DRAW_POINTS, 6.0f, drawOffset);
			for (int i = 0; i < m_nstraightPath; ++i)
			{
				unsigned int col;
				if (m_straightPathFlags[i] & DT_STRAIGHTPATH_START)
					col = startCol;
				else if (m_straightPathFlags[i] & DT_STRAIGHTPATH_END)
					col = endCol;
				else if (m_straightPathFlags[i] & DT_STRAIGHTPATH_OFFMESH_CONNECTION)
					col = offMeshCol;
				else
					col = spathCol;
				dd.vertex(m_straightPath[i*3], m_straightPath[i*3+1], m_straightPath[i*3+2] + 0.4f, col);
			}
			dd.end();
			dd.depthMask(true);
		}
	}
	else if (m_toolMode == TOOLMODE_RAYCAST)
	{
		duDebugDrawNavMeshPoly(&dd, *m_navMesh, m_startRef, drawOffset, drawFlags, startCol);
		
		if (m_nstraightPath)
		{
			for (int i = 1; i < m_npolys; ++i)
				duDebugDrawNavMeshPoly(&dd, *m_navMesh, m_polys[i], drawOffset, drawFlags, pathCol);
			
			dd.depthMask(false);
			const unsigned int spathCol = m_hitResult ? duRGBA(64,16,0,220) : duRGBA(240,240,240,220);
			dd.begin(DU_DRAW_LINES, 2.0f, drawOffset);
			for (int i = 0; i < m_nstraightPath-1; ++i)
			{
				dd.vertex(m_straightPath[i*3], m_straightPath[i*3+1], m_straightPath[i*3+2] + 0.4f, spathCol);
				dd.vertex(m_straightPath[(i+1)*3], m_straightPath[(i+1)*3+1], m_straightPath[(i+1)*3+2] + 0.4f, spathCol);
			}
			dd.end();
			dd.begin(DU_DRAW_POINTS, 4.0f, drawOffset);
			for (int i = 0; i < m_nstraightPath; ++i)
				dd.vertex(m_straightPath[i*3], m_straightPath[i*3+1], m_straightPath[i*3+2] + 0.4f, spathCol);
			dd.end();

			if (m_hitResult)
			{
				const unsigned int hitCol = duRGBA(0,0,0,128);
				dd.begin(DU_DRAW_LINES, 2.0f, drawOffset);
				dd.vertex(m_hitPos[0],m_hitPos[1],m_hitPos[2]+0.4f,hitCol);
				dd.vertex(m_hitPos[0]+m_hitNormal[0]*agentRadius,
						  m_hitPos[1]+m_hitNormal[1]*agentRadius,
						  m_hitPos[2]+0.4f+m_hitNormal[2]*agentRadius,hitCol);
				dd.end();
			}
			dd.depthMask(true);
		}
	}
	else if (m_toolMode == TOOLMODE_DISTANCE_TO_WALL)
	{
		duDebugDrawNavMeshPoly(&dd, *m_navMesh, m_startRef, drawOffset, drawFlags, startCol);
		dd.depthMask(false);
		duDebugDrawCircle(&dd, m_spos[0], m_spos[1], m_spos[2] + agentHeight / 2, m_distanceToWall, duRGBA(64,16,0,220), 2.0f, drawOffset);
		dd.begin(DU_DRAW_LINES, 3.0f, drawOffset);
		dd.vertex(m_hitPos[0], m_hitPos[1] , m_hitPos[2] + 0.02f, duRGBA(0,0,0,192));
		dd.vertex(m_hitPos[0], m_hitPos[1] , m_hitPos[2] + agentHeight, duRGBA(0,0,0,192));
		dd.end();
		dd.depthMask(true);
	}
	else if (m_toolMode == TOOLMODE_FIND_POLYS_IN_CIRCLE)
	{
		for (int i = 0; i < m_npolys; ++i)
		{
			duDebugDrawNavMeshPoly(&dd, *m_navMesh, m_polys[i], drawOffset, drawFlags, pathCol);
			dd.depthMask(false);
			if (m_parent[i])
			{
				float p0[3], p1[3];
				dd.depthMask(false);
				getPolyCenter(m_navMesh, m_parent[i], p0);
				getPolyCenter(m_navMesh, m_polys[i], p1);
				duDebugDrawArc(&dd, p0[0],p0[1],p0[2], p1[0],p1[1],p1[2], 0.25f, 0.0f, 30.0f, duRGBA(0,0,0,128), 2.0f, drawOffset);
				dd.depthMask(true);
			}
			dd.depthMask(true);
		}
		
		if (m_sposSet && m_eposSet)
		{
			dd.depthMask(false);
			const float dx = m_epos[0] - m_spos[0];
			const float dy = m_epos[1] - m_spos[1];
			const float dist = sqrtf(dx*dx + dy*dy);
			duDebugDrawCircle(&dd, m_spos[0], m_spos[1], m_spos[2] + agentHeight / 2, dist, duRGBA(64,16,0,220), 2.0f, drawOffset);
			dd.depthMask(true);
		}
	}	
	else if (m_toolMode == TOOLMODE_FIND_POLYS_IN_SHAPE)
	{
		for (int i = 0; i < m_npolys; ++i)
		{
			duDebugDrawNavMeshPoly(&dd, *m_navMesh, m_polys[i], drawOffset, drawFlags, pathCol);
			dd.depthMask(false);
			if (m_parent[i])
			{
				float p0[3], p1[3];
				dd.depthMask(false);
				getPolyCenter(m_navMesh, m_parent[i], p0);
				getPolyCenter(m_navMesh, m_polys[i], p1);
				duDebugDrawArc(&dd, p0[0],p0[1],p0[2], p1[0],p1[1],p1[2], 0.25f, 0.0f, 30.0f, duRGBA(0,0,0,128), 2.0f, drawOffset);
				dd.depthMask(true);
			}
			dd.depthMask(true);
		}
		
		if (m_sposSet && m_eposSet)
		{
			dd.depthMask(false);
			const unsigned int col = duRGBA(64,16,0,220);
			dd.begin(DU_DRAW_LINES, 2.0f, drawOffset);
			for (int i = 0, j = 3; i < 4; j=i++)
			{
				const float* p0 = &m_queryPoly[j*3];
				const float* p1 = &m_queryPoly[i*3];
				dd.vertex(p0, col);
				dd.vertex(p1, col);
			}
			dd.end();
			dd.depthMask(true);
		}
	}
	else if (m_toolMode == TOOLMODE_FIND_LOCAL_NEIGHBOURHOOD)
	{
		for (int i = 0; i < m_npolys; ++i)
		{
			duDebugDrawNavMeshPoly(&dd, *m_navMesh, m_polys[i], drawOffset, drawFlags, pathCol);
			dd.depthMask(false);
			if (m_parent[i])
			{
				float p0[3], p1[3];
				dd.depthMask(false);
				getPolyCenter(m_navMesh, m_parent[i], p0);
				getPolyCenter(m_navMesh, m_polys[i], p1);
				duDebugDrawArc(&dd, p0[0],p0[1],p0[2], p1[0],p1[1],p1[2], 0.25f, 0.0f, 30.0f, duRGBA(0,0,0,128), 2.0f, drawOffset);
				dd.depthMask(true);
			}

			static const int MAX_SEGS = DT_VERTS_PER_POLYGON*4;
			float segs[MAX_SEGS*6];
			dtPolyRef refs[MAX_SEGS];
			memset(refs, 0, sizeof(dtPolyRef)*MAX_SEGS); 
			int nsegs = 0;
			m_navQuery->getPolyWallSegments(m_polys[i], &m_filter, segs, refs, &nsegs, MAX_SEGS);
			dd.begin(DU_DRAW_LINES, 2.0f, drawOffset);
			for (int j = 0; j < nsegs; ++j)
			{
				const float* s = &segs[j*6];
				
				// Skip too distant segments.
				float tseg;
				float distSqr = rdDistancePtSegSqr2D(m_spos, s, s+3, tseg);
				if (distSqr > rdSqr(m_neighbourhoodRadius))
					continue;
				
				float delta[3], norm[3], p0[3], p1[3];
				rdVsub(delta, s+3,s);
				rdVmad(p0, s, delta, 0.5f);
				norm[0] = -delta[1];
				norm[1] = delta[0];
				norm[2] = 0;
				rdVnormalize(norm);
				rdVmad(p1, p0, norm, agentRadius*0.5f);

				// Skip backfacing segments.
				if (refs[j])
				{
					unsigned int col = duRGBA(255,255,255,32);
					dd.vertex(s[0],s[1],s[2]+agentClimb,col);
					dd.vertex(s[3],s[4],s[5]+agentClimb,col);
				}
				else
				{
					unsigned int col = duRGBA(192,32,16,192);
					if (rdTriArea2D(m_spos, s, s+3) < 0.0f)
						col = duRGBA(96,32,16,192);
					
					dd.vertex(p0[0],p0[1],p0[2]+agentClimb,col);
					dd.vertex(p1[0],p1[1],p1[2]+agentClimb,col);

					dd.vertex(s[0],s[1],s[2]+agentClimb,col);
					dd.vertex(s[3],s[4],s[5]+agentClimb,col);
				}
			}
			dd.end();
			
			dd.depthMask(true);
		}
		
		if (m_sposSet)
		{
			dd.depthMask(false);
			duDebugDrawCircle(&dd, m_spos[0], m_spos[1], m_spos[2] + agentHeight / 2, m_neighbourhoodRadius, duRGBA(64,16,0,220), 2.0f, drawOffset);
			dd.depthMask(true);
		}
	}
	
	if (m_nrandPoints > 0)
	{
		dd.begin(DU_DRAW_POINTS, 6.0f, drawOffset);
		for (int i = 0; i < m_nrandPoints; i++)
		{
			const float* p = &m_randPoints[i*3];
			dd.vertex(p[0],p[1],p[2] + 0.1f, duRGBA(220,32,16,192));
		} 
		dd.end();
		
		if (m_randPointsInCircle && m_sposSet)
		{
			duDebugDrawCircle(&dd, m_spos[0], m_spos[1], m_spos[2] + agentHeight / 2, m_randomRadius, duRGBA(64,16,0,220), 2.0f, drawOffset);
		}
	}
}

void NavMeshTesterTool::handleRenderOverlay(double* proj, double* model, int* view)
{
	GLdouble x, y, z;
	const int h = view[3];
	const float* drawOffset = m_editor->getDetourDrawOffset();

	// Draw start and end point labels
	if (m_sposSet && gluProject((GLdouble)m_spos[0]+drawOffset[0], (GLdouble)m_spos[1]+drawOffset[1], (GLdouble)m_spos[2]+drawOffset[2],
								model, proj, view, &x, &y, &z))
	{
		ImGui_RenderText(ImGuiTextAlign_e::kAlignCenter,
			ImVec2((float)x, h-((float)y-25)), ImVec4(0,0,0,0.8f), "Start");
	}
	if (m_eposSet && gluProject((GLdouble)m_epos[0]+drawOffset[0], (GLdouble)m_epos[1]+drawOffset[1], (GLdouble)m_epos[2]+drawOffset[2],
								model, proj, view, &x, &y, &z))
	{
		ImGui_RenderText(ImGuiTextAlign_e::kAlignCenter,
			ImVec2((float)x, h-((float)y-25)), ImVec4(0,0,0,0.8f), "End");
	}
	
	// Tool help
	ImGui_RenderText(ImGuiTextAlign_e::kAlignLeft, ImVec2(280, 40),
		ImVec4(1.0f,1.0f,1.0f,0.75f), "LMB+SHIFT: Set start location  LMB: Set end location");
}

void NavMeshTesterTool::drawAgent(const float* pos, float r, float h, float c, const unsigned int col)
{
	duDebugDraw& dd = m_editor->getDebugDraw();
	
	dd.depthMask(false);

	const float* drawOffset = m_editor->getDetourDrawOffset();
	
	// Agent dimensions.	
	duDebugDrawCylinderWire(&dd, pos[0]-r, pos[1]-r, pos[2]+0.02f, pos[0]+r, pos[1]+r, pos[2]+h, col, 2.0f, drawOffset);

	duDebugDrawCircle(&dd, pos[0],pos[1],pos[2] + c,r,duRGBA(0,0,0,64),1.0f, drawOffset);

	unsigned int colb = duRGBA(0,0,0,196);
	dd.begin(DU_DRAW_LINES, 1.0f, drawOffset);
	dd.vertex(pos[0],pos[1],pos[2]-c,colb);
	dd.vertex(pos[0],pos[1],pos[2]+c,colb);
	dd.vertex(pos[0]-r/2,pos[1],pos[2]+0.02f,colb);
	dd.vertex(pos[0]+r/2,pos[1],pos[2]+0.02f,colb);
	dd.vertex(pos[0],pos[1]-r/2,pos[2]+0.02f,colb);
	dd.vertex(pos[0],pos[1]+r/2,pos[2]+0.02f,colb);
	dd.end();
	
	dd.depthMask(true);
}
