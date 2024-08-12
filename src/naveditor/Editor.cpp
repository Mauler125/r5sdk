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
#include "Shared/Include/SharedAssert.h"
#include "Shared/Include/SharedCommon.h"
#include "Detour/Include/DetourNavMesh.h"
#include "Detour/Include/DetourNavMeshQuery.h"
#include "Detour/Include/DetourNavMeshBuilder.h"
#include "DetourCrowd/Include/DetourCrowd.h"
#include "DebugUtils/Include/RecastDebugDraw.h"
#include "DebugUtils/Include/DetourDebugDraw.h"
#include "NavEditor/Include/GameUtils.h"
#include "NavEditor/Include/InputGeom.h"
#include "NavEditor/Include/Editor.h"

#include "game/server/ai_navmesh.h"
#include "game/server/ai_hull.h"
#include "coordsize.h"

unsigned int EditorDebugDraw::areaToCol(unsigned int area)
{
	switch(area)
	{
	// Ground : light blue
	case EDITOR_POLYAREA_GROUND: return duRGBA(0, 192, 215, 255);
	// Jump : purple
	case EDITOR_POLYAREA_JUMP: return duRGBA(255, 0, 255, 255);
	// Trigger : light green
	case EDITOR_POLYAREA_DOOR: return duRGBA(20, 245, 0, 255);
	// Unexpected : white
	default: return duRGBA(255, 255, 255, 255);
	}
}

Editor::Editor() :
	m_geom(0),
	m_navMesh(0),
	m_navQuery(0),
	m_crowd(0),
	m_navMeshDrawFlags(
		DU_DRAWNAVMESH_OFFMESHCONS|DU_DRAWNAVMESH_WITH_CLOSED_LIST|
		DU_DRAWNAVMESH_POLY_BOUNDS_OUTER|DU_DRAWNAVMESH_ALPHA),
	m_filterLowHangingObstacles(true),
	m_filterLedgeSpans(true),
	m_filterWalkableLowHeightSpans(true),
	m_selectedNavMeshType(NAVMESH_SMALL),
	m_loadedNavMeshType(NAVMESH_SMALL),
	m_navmeshName(NavMesh_GetNameForType(NAVMESH_SMALL)),
	m_tool(0),
	m_ctx(0)
{
	resetCommonSettings();
	m_navQuery = dtAllocNavMeshQuery();
	m_crowd = dtAllocCrowd();

	for (int i = 0; i < MAX_TOOLS; i++)
		m_toolStates[i] = 0;

	rdVset(m_recastDrawOffset, 0.0f,0.0f,4.0f);
	rdVset(m_detourDrawOffset, 0.0f,0.0f,8.0f);
}

Editor::~Editor()
{
	dtFreeNavMeshQuery(m_navQuery);
	dtFreeNavMesh(m_navMesh);
	dtFreeCrowd(m_crowd);
	delete m_tool;
	for (int i = 0; i < MAX_TOOLS; i++)
		delete m_toolStates[i];
}

void Editor::setTool(EditorTool* tool)
{
	delete m_tool;
	m_tool = tool;
	if (tool)
		m_tool->init(this);
}

void Editor::handleSettings()
{
}

void Editor::handleTools()
{
}

void Editor::handleDebugMode()
{
}

void Editor::handleRender()
{
	if (!m_geom)
		return;
	
	// Draw mesh
	duDebugDrawTriMesh(&m_dd, m_geom->getMesh()->getVerts(), m_geom->getMesh()->getVertCount(),
					   m_geom->getMesh()->getTris(), m_geom->getMesh()->getNormals(), m_geom->getMesh()->getTriCount(), 0, 1.0f, nullptr);
	// Draw bounds
	const float* bmin = m_geom->getMeshBoundsMin();
	const float* bmax = m_geom->getMeshBoundsMax();
	duDebugDrawBoxWire(&m_dd, bmin[0],bmin[1],bmin[2], bmax[0],bmax[1],bmax[2], duRGBA(255,255,255,128), 1.0f, nullptr);
}

void Editor::handleRenderOverlay(double* /*proj*/, double* /*model*/, int* /*view*/)
{
}

void Editor::handleMeshChanged(InputGeom* geom)
{
	m_geom = geom;

	const BuildSettings* buildSettings = geom->getBuildSettings();
	if (buildSettings)
	{
		m_cellSize = buildSettings->cellSize;
		m_cellHeight = buildSettings->cellHeight;
		m_agentHeight = buildSettings->agentHeight;
		m_agentRadius = buildSettings->agentRadius;
		m_agentMaxClimb = buildSettings->agentMaxClimb;
		m_agentMaxSlope = buildSettings->agentMaxSlope;
		m_regionMinSize = buildSettings->regionMinSize;
		m_regionMergeSize = buildSettings->regionMergeSize;
		m_edgeMaxLen = buildSettings->edgeMaxLen;
		m_edgeMaxError = buildSettings->edgeMaxError;
		m_vertsPerPoly = buildSettings->vertsPerPoly;
		m_polyCellRes = buildSettings->polyCellRes;
		m_detailSampleDist = buildSettings->detailSampleDist;
		m_detailSampleMaxError = buildSettings->detailSampleMaxError;
		m_partitionType = buildSettings->partitionType;
	}
}

void Editor::collectSettings(BuildSettings& settings)
{
	settings.cellSize = m_cellSize;
	settings.cellHeight = m_cellHeight;
	settings.agentHeight = m_agentHeight;
	settings.agentRadius = m_agentRadius;
	settings.agentMaxClimb = m_agentMaxClimb;
	settings.agentMaxSlope = m_agentMaxSlope;
	settings.regionMinSize = m_regionMinSize;
	settings.regionMergeSize = m_regionMergeSize;
	settings.edgeMaxLen = m_edgeMaxLen;
	settings.edgeMaxError = m_edgeMaxError;
	settings.vertsPerPoly = m_vertsPerPoly;
	settings.polyCellRes = m_polyCellRes;
	settings.detailSampleDist = m_detailSampleDist;
	settings.detailSampleMaxError = m_detailSampleMaxError;
	settings.partitionType = m_partitionType;
}


void Editor::resetCommonSettings()
{
	selectNavMeshType(NAVMESH_SMALL);

	m_cellSize = 16.0f;
	m_cellHeight = 5.85f;

	// todo(amos): check if this applies for all hulls, and check if this is the
	// actual value used by the game. This seems to generate slopes very close
	// to the walkable slopes in-game. The slopes generated for the map
	// mp_rr_canyonlands_staging.bsp where pretty much identical. If this is
	// confirmed, move this value to a game header instead and define it as a
	// constant. The value originates from here under "Player Collision Hull":
	// https://developer.valvesoftware.com/wiki/Pl/Dimensions
	m_agentMaxSlope = 45.573f;

	m_regionMinSize = 8;
	m_regionMergeSize = 20;
	m_edgeMaxLen = 12;
	m_edgeMaxError = 1.3f;
	m_vertsPerPoly = 6;
	m_detailSampleDist = 6.0f;
	m_detailSampleMaxError = 1.0f;
	m_partitionType = EDITOR_PARTITION_WATERSHED;
}
void Editor::handleCommonSettings()
{
	ImGui::Text("NavMesh Type");
	for (int i = 0; i < NAVMESH_COUNT; i++)
	{
		const NavMeshType_e navMeshType = NavMeshType_e(i);

		if (ImGui::Button(NavMesh_GetNameForType(navMeshType), ImVec2(120, 0)))
		{
			selectNavMeshType(navMeshType);
		}
	}

	ImGui::Separator();

	ImGui::PushItemWidth(180.f);
	ImGui::Text("Rasterization");

	ImGui::SliderFloat("Cell Size", &m_cellSize, 12.1f, 100.0f);
	ImGui::SliderFloat("Cell Height", &m_cellHeight, 0.4f, 100.0f);
	
	if (m_geom)
	{
		const float* bmin = m_geom->getNavMeshBoundsMin();
		const float* bmax = m_geom->getNavMeshBoundsMax();
		int gw = 0, gh = 0;
		rcCalcGridSize(bmin, bmax, m_cellSize, &gw, &gh);
		char text[64];
		snprintf(text, 64, "Voxels: %d x %d", gw, gh);
		ImGui::Text(text);
	}
	
	ImGui::Separator();
	ImGui::Text("Agent");
	ImGui::SliderFloat("Height", &m_agentHeight, 0.1f, 500.0f);
	ImGui::SliderFloat("Radius", &m_agentRadius, 0.0f, 500.0f);
	ImGui::SliderFloat("Max Climb", &m_agentMaxClimb, 0.1f, 250.0f);
	ImGui::SliderFloat("Max Slope", &m_agentMaxSlope, 0.0f, 90.0f);

	ImGui::PopItemWidth();
	ImGui::PushItemWidth(140.f);
	
	ImGui::Separator();
	ImGui::Text("Region");
	ImGui::SliderInt("Min Region Size", &m_regionMinSize, 0, 750); // todo(amos): increase because of larger map scale?
	ImGui::SliderInt("Merged Region Size", &m_regionMergeSize, 0, 750); // todo(amos): increase because of larger map scale?

	ImGui::PopItemWidth();

	if (m_geom)
	{
		ImGui::Separator();
		ImGui::Text("Bounding");

		float* navMeshBMin = m_geom->getNavMeshBoundsMin();
		float* navMeshBMax = m_geom->getNavMeshBoundsMax();

		const float* meshBMin = m_geom->getMeshBoundsMin();
		const float* meshBMax = m_geom->getMeshBoundsMax();

		ImGui::PushItemWidth(75);
		ImGui::SliderFloat("##BoundingMinsX", &navMeshBMin[0], meshBMin[0], rdMin(meshBMax[0], navMeshBMax[0]));
		ImGui::SameLine();
		ImGui::SliderFloat("##BoundingMinsY", &navMeshBMin[1], meshBMin[1], rdMin(meshBMax[1], navMeshBMax[1]));
		ImGui::SameLine();
		ImGui::SliderFloat("##BoundingMinsZ", &navMeshBMin[2], meshBMin[2], rdMin(meshBMax[2], navMeshBMax[2]));
		ImGui::SameLine();
		ImGui::Text("Mins");

		ImGui::SliderFloat("##BoundingMaxsX", &navMeshBMax[0], rdMax(meshBMin[0], navMeshBMin[0]), meshBMax[0]);
		ImGui::SameLine();
		ImGui::SliderFloat("##BoundingMaxsY", &navMeshBMax[1], rdMax(meshBMin[1], navMeshBMin[1]), meshBMax[1]);
		ImGui::SameLine();
		ImGui::SliderFloat("##BoundingMaxsZ", &navMeshBMax[2], rdMax(meshBMin[2], navMeshBMin[2]), meshBMax[2]);
		ImGui::SameLine();
		ImGui::Text("Maxs");
		ImGui::PopItemWidth();

		if (ImGui::Button("Reset##BoundingSettings", ImVec2(120, 0)))
		{
			rdVcopy(navMeshBMin, m_geom->getOriginalNavMeshBoundsMin());
			rdVcopy(navMeshBMax, m_geom->getOriginalNavMeshBoundsMax());
		}
	}

	ImGui::Separator();
	ImGui::Text("Partitioning");

	bool isEnabled = m_partitionType == EDITOR_PARTITION_WATERSHED;

	if (ImGui::Checkbox("Watershed", &isEnabled))
		m_partitionType = EDITOR_PARTITION_WATERSHED;

	isEnabled = m_partitionType == EDITOR_PARTITION_MONOTONE;

	if (ImGui::Checkbox("Monotone", &isEnabled))
		m_partitionType = EDITOR_PARTITION_MONOTONE;

	isEnabled = m_partitionType == EDITOR_PARTITION_LAYERS;

	if (ImGui::Checkbox("Layers", &isEnabled))
		m_partitionType = EDITOR_PARTITION_LAYERS;
	
	ImGui::Separator();
	ImGui::Text("Filtering");
	ImGui::Checkbox("Low Hanging Obstacles", &m_filterLowHangingObstacles);
	ImGui::Checkbox("Ledge Spans", &m_filterLedgeSpans);
	ImGui::Checkbox("Walkable Low Height Spans", &m_filterWalkableLowHeightSpans);

	ImGui::PushItemWidth(145.f);
	ImGui::Separator();

	ImGui::Text("Polygonization");
	ImGui::SliderInt("Max Edge Length", &m_edgeMaxLen, 0, 50); // todo(amos): increase due to larger scale maps?
	ImGui::SliderFloat("Max Edge Error", &m_edgeMaxError, 0.1f, 3.0f);
	ImGui::SliderInt("Verts Per Poly", &m_vertsPerPoly, 3, 6);
	ImGui::SliderInt("Poly Cell Resolution", &m_polyCellRes, 1, 16);

	ImGui::Separator();
	ImGui::Text("Detail Mesh");
	ImGui::SliderFloat("Sample Distance", &m_detailSampleDist, 1.0f, 16.0f);
	ImGui::SliderFloat("Max Sample Error", &m_detailSampleMaxError, 0.0f, 16.0f);

	ImGui::PopItemWidth();
	
	ImGui::Separator();
}

void Editor::handleClick(const float* s, const float* p, bool shift)
{
	if (m_tool)
		m_tool->handleClick(s, p, shift);
}

void Editor::handleToggle()
{
	if (m_tool)
		m_tool->handleToggle();
}

void Editor::handleStep()
{
	if (m_tool)
		m_tool->handleStep();
}

bool Editor::handleBuild()
{
	return true;
}

void Editor::handleUpdate(const float dt)
{
	if (m_tool)
		m_tool->handleUpdate(dt);
	updateToolStates(dt);
}

enum TraverseType_e // todo(amos): move elsewhere
{
	TRAVERSE_UNUSED_0 = 0,

	TRAVERSE_CROSS_GAP_SMALL,
	TRAVERSE_CLIMB_OBJECT_SMALL,
	TRAVERSE_CROSS_GAP_MEDIUM,

	TRAVERSE_UNUSED_4,
	TRAVERSE_UNUSED_5,
	TRAVERSE_UNUSED_6,

	TRAVERSE_CROSS_GAP_LARGE,

	TRAVERSE_CLIMB_WALL_MEDIUM,
	TRAVERSE_CLIMB_WALL_TALL,
	TRAVERSE_CLIMB_BUILDING,

	TRAVERSE_JUMP_SHORT,
	TRAVERSE_JUMP_MEDIUM,
	TRAVERSE_JUMP_LARGE,

	TRAVERSE_UNUSED_14,
	TRAVERSE_UNUSED_15,

	TRAVERSE_UNKNOWN_16, // USED!!!
	TRAVERSE_UNKNOWN_17, // USED!!!

	TRAVERSE_UNKNOWN_18,
	TRAVERSE_UNKNOWN_19, // NOTE: does not exists in MSET5!!!

	TRAVERSE_CLIMB_TARGET_SMALL,
	TRAVERSE_CLIMB_TARGET_LARGE,

	TRAVERSE_UNUSED_22,
	TRAVERSE_UNUSED_23,

	TRAVERSE_UNKNOWN_24,

	TRAVERSE_UNUSED_25,
	TRAVERSE_UNUSED_26,
	TRAVERSE_UNUSED_27,
	TRAVERSE_UNUSED_28,
	TRAVERSE_UNUSED_29,
	TRAVERSE_UNUSED_30,
	TRAVERSE_UNUSED_31,

	// These aren't traverse type!
	NUM_TRAVERSE_TYPES,
	INVALID_TRAVERSE_TYPE = DT_NULL_TRAVERSE_TYPE
};

struct TraverseType_s // todo(amos): move elsewhere
{
	float minSlope; // todo(amos): use height difference instead of slope angles.
	float maxSlope; // todo(amos): use height difference instead of slope angles.

	unsigned char minDist;
	unsigned char maxDist;

	bool forceSamePolyGroup;
	bool forceDifferentPolyGroup;
};

static TraverseType_s s_traverseTypes[NUM_TRAVERSE_TYPES] = // todo(amos): move elsewhere
{
	{0.0f, 0.0f, 0, 0, false, false}, // Unused

	{0.0f, 67.f, 2, 12, false, false },
	{5.0f, 78.f, 5, 16, false, false },
	{0.0f, 38.f, 11, 22, false, false },

	{0.0f, 0.0f, 0, 0, false, false}, // Unused
	{0.0f, 0.0f, 0, 0, false, false}, // Unused
	{0.0f, 0.0f, 0, 0, false, false}, // Unused

	{0.0f, 6.5f, 80, 107, false, true},
	{19.0f, 84.0f, 7, 21, false, false},
	{27.0f, 87.5f, 16, 45, false, false},
	{44.0f, 89.5f, 33, 225, false, false},
	{0.0f, 7.0f, 41, 79, false, false},
	{2.2f, 47.0f, 41, 100, false, false},
	{5.7f, 58.5f, 81, 179, false, false},

	{0.0f, 0.0f, 0, 0, false, false}, // Unused
	{0.0f, 0.0f, 0, 0, false, false}, // Unused

	{0.0f, 12.5f, 22, 41, false, false},
	{4.6f, 53.0f, 21, 58, false, false},

	{0.0f, 0.0f, 0, 0, false, false}, // Unused
	{0.0f, 0.0f, 0, 0, false, false}, // Unused

	{29.0f, 47.0f, 16, 40, false, false}, // Maps to type 19 in MSET 5
	{46.5f, 89.0f, 33, 199, false, false}, // Maps to type 20 in MSET 5

	{0.0f, 0.0f, 0, 0, false, false}, // Unused
	{0.0f, 0.0f, 0, 0, false, false}, // Unused

	{0.0f, 89.0f, 5, 251, false, false}, // Does not exist in MSET 5

	{0.0f, 0.0f, 0, 0, false, false}, // Unused
	{0.0f, 0.0f, 0, 0, false, false}, // Unused
	{0.0f, 0.0f, 0, 0, false, false}, // Unused
	{0.0f, 0.0f, 0, 0, false, false}, // Unused
	{0.0f, 0.0f, 0, 0, false, false}, // Unused
	{0.0f, 0.0f, 0, 0, false, false}, // Unused
	{0.0f, 0.0f, 0, 0, false, false}, // Unused
};

TraverseType_e GetBestTraverseType(const float slopeAngle, const unsigned char traverseDist, const bool samePolyGroup)
{
	TraverseType_e bestTraverseType = INVALID_TRAVERSE_TYPE;

	for (int i = 0; i < NUM_TRAVERSE_TYPES; ++i)
	{
		const TraverseType_s& traverseType = s_traverseTypes[i];

		// Skip unused types...
		if (traverseType.minSlope == 0.0f && traverseType.maxSlope == 0.0f &&
			traverseType.minDist == 0 && traverseType.maxDist == 0)
		{
			continue;
		}

		if (slopeAngle < traverseType.minSlope ||
			slopeAngle > traverseType.maxSlope)
		{
			continue;
		}

		if (traverseDist < traverseType.minDist ||
			traverseDist > traverseType.maxDist)
		{
			continue;
		}

		// NOTE: currently only type 7 is enforced in this check, perhaps we
		// should limit some other types on same/diff poly groups as well?
		if ((traverseType.forceSamePolyGroup && !samePolyGroup) ||
			(traverseType.forceDifferentPolyGroup && samePolyGroup))
		{
			continue;
		}

		bestTraverseType = (TraverseType_e)i;
		break;
	}

	return bestTraverseType;
}

// todo(amos): find the best threshold...
// todo(amos): use height difference instead of slope angles.
#define TRAVERSE_OVERLAP_SLOPE_THRESHOLD 5.0f

bool CanOverlapPoly(const TraverseType_e traverseType)
{
	return s_traverseTypes[traverseType].minSlope >= TRAVERSE_OVERLAP_SLOPE_THRESHOLD;
}

static bool traverseLinkInPolygon(const dtMeshTile* tile, const float* midPoint)
{
	const int polyCount = tile->header->polyCount;

	for (int i = 0; i < polyCount; i++)
	{
		const dtPoly* poly = &tile->polys[i];
		float verts[DT_VERTS_PER_POLYGON*3];

		const int nverts = poly->vertCount;
		for (int j = 0; j < nverts; ++j)
			rdVcopy(&verts[j*3], &tile->verts[poly->verts[j]*3]);

		if (rdPointInPolygon(midPoint, verts, nverts))
			return true;
	}

	return false;
}

static bool traverseLinkInLOS(InputGeom* geom, const float* lowPos, const float* highPos, const float* edgeDir, const float offsetAmount)
{
	// We offset the highest point with at least the
	// walkable radius, and perform a raycast test
	// from the highest point to the lowest. The
	// offsetting is necessary to account for the
	// gap between the edge of the navmesh and the
	// edge of the geometry shown below:
	// 
	//                          geom    upper navmesh
	//                          ^       ^
	//                     gap  |       |
	//                     ^    |       |
	//                     |    |       |
	//           offset <-----> | +++++++++++++...
	//                / =======================...
	//               /
	//     ray <----/     lower navmesh
	//             /      ^
	//     geom   /       |
	//     ^     /        |
	//     |    +++++++++++++++++++++++++++++++...
	// ========================================...
	// 
	// We only want the raycast test to fail if the
	// ledge is larger than usual, when the low and
	// high positions are angled in such way no LOS
	// is possible, or when there's an actual object
	// between the 2 positions.
	float perp[3];
	rdPerpDirEdge2D(edgeDir, false, perp);

	float targetRayPos[3] = {
		highPos[0] + perp[0] * offsetAmount,
		highPos[1] + perp[1] * offsetAmount,
		highPos[2]
	};

	float hitTime;

	// note(amos): perform 2 raycasts as we have to take the
	// face normal into account. Path must be clear from both
	// directions. We cast from the upper position first as
	// an optimization attempt because if there's a ledge, the
	// raycast test from the lower pos is more likely to pass
	// due to mesh normals, e.g. when the higher mesh is generated
	// on a single sided plane, and we have a ledge between our
	// lower and higher pos, the test from below will pass while
	// the test from above won't. Doing the test from below won't
	// matter besides burning CPU time as we will never get here if
	// the mesh normals of that plane were flipped as there
	// won't be any navmesh on the higher pos in the first place.
	// Its still possible there's something blocking on the lower
	// pos' side, but this is a lot less likely to happen.
	if (geom->raycastMesh(targetRayPos, lowPos, hitTime) ||
		geom->raycastMesh(lowPos, targetRayPos, hitTime))
		return false;

	return true;
}

// TODO: create lookup table and look for distance + slope to determine the
// correct jumpType.
// TODO: make sure we don't generate duplicate pairs of jump types between
// 2 polygons.
void Editor::connectTileTraverseLinks(dtMeshTile* const baseTile, const bool linkToNeighbor)
{
	for (int i = 0; i < baseTile->header->polyCount; ++i)
	{
		dtPoly* const basePoly = &baseTile->polys[i];

		for (int j = 0; j < basePoly->vertCount; ++j)
		{
			// Hard edges only!
			if (basePoly->neis[j] != 0)
				continue;

			// Polygon 1 edge
			const float* const basePolySpos = &baseTile->verts[basePoly->verts[j] * 3];
			const float* const basePolyEpos = &baseTile->verts[basePoly->verts[(j + 1) % basePoly->vertCount] * 3];

			float basePolyEdgeMid[3];
			rdVsad(basePolyEdgeMid, basePolySpos, basePolyEpos, 0.5f);

			unsigned char side = (unsigned char)rdOppositeTile(rdClassifyPointInsideBounds(basePolyEdgeMid, baseTile->header->bmin, baseTile->header->bmax));
			const int MAX_NEIS = 32; // Max neighbors

			dtMeshTile* neis[MAX_NEIS];
			int nneis;

			if (linkToNeighbor) // Retrieve the neighboring tiles on the side of our base poly edge.
				nneis = m_navMesh->getNeighbourTilesAt(baseTile->header->x, baseTile->header->y, side, neis, MAX_NEIS);
			else
			{
				// Internal links.
				nneis = 1;
				neis[0] = baseTile;
			}

			for (int k = 0; k < nneis; ++k)
			{
				dtMeshTile* landTile = neis[k];
				const bool external = baseTile != landTile;

				if (!external && i == k) continue; // Skip self

				for (int m = 0; m < landTile->header->polyCount; ++m)
				{
					dtPoly* const landPoly = &landTile->polys[m];

					for (int n = 0; n < landPoly->vertCount; ++n)
					{
						if (landPoly->neis[n] != 0)
							continue;

						// Polygon 2 edge
						const float* const landPolySpos = &landTile->verts[landPoly->verts[n] * 3];
						const float* const landPolyEpos = &landTile->verts[landPoly->verts[(n + 1) % landPoly->vertCount] * 3];

						float landPolyEdgeMid[3];
						rdVsad(landPolyEdgeMid, landPolySpos, landPolyEpos, 0.5f);

						const unsigned char distance = dtCalcLinkDistance(basePolyEdgeMid, landPolyEdgeMid);

						if (distance == 0)
							continue;

						float baseEdgeDir[3], landEdgeDir[3];
						rdVsub(baseEdgeDir, basePolyEpos, basePolySpos);
						rdVsub(landEdgeDir, landPolyEpos, landPolySpos);

						// todo(amos): use height difference instead of slope angles.
						const float slopeAngle = rdMathFabsf(rdCalcSlopeAngle(basePolyEdgeMid, landPolyEdgeMid));

						if (slopeAngle < TRAVERSE_OVERLAP_SLOPE_THRESHOLD)
						{

							const float dotProduct = rdVdot(baseEdgeDir, landEdgeDir);

							// Edges facing the same direction should not be linked.
							// Doing so causes links to go through from underneath
							// geometry. E.g. we have an HVAC on a roof, and we try
							// to link our roof poly edge facing north to the edge
							// of the poly on the HVAC also facing north, the link
							// will go through the HVAC and thus cause the NPC to
							// jump through it.
							// Another case where this is necessary is when having
							// a land edge that connects with the base edge, this
							// prevents the algorithm from establishing a parallel
							// traverse link.
							if (dotProduct > 0)
								continue;
						}

						float t, s;
						if (rdIntersectSegSeg2D(basePolySpos, basePolyEpos, landPolySpos, landPolyEpos, t, s))
							continue;

						const bool samePolyGroup = basePoly->groupId == landPoly->groupId;

						const TraverseType_e traverseType = GetBestTraverseType(slopeAngle, distance, samePolyGroup);

						if (traverseType == DT_NULL_TRAVERSE_TYPE)
							continue;

						if (!CanOverlapPoly(traverseType))
						{
							float linkMidPoint[3];
							rdVsad(linkMidPoint, basePolyEdgeMid, landPolyEdgeMid, 0.5f);

							if (traverseLinkInPolygon(baseTile, linkMidPoint) || traverseLinkInPolygon(landTile, linkMidPoint))
								continue;
						}

						const bool basePolyHigher = basePolyEdgeMid[2] > landPolyEdgeMid[2];
						float* const lowerEdgeMid = basePolyHigher ? landPolyEdgeMid : basePolyEdgeMid;
						float* const higherEdgeMid = basePolyHigher ? basePolyEdgeMid : landPolyEdgeMid;
						float* const higherEdgeDir = basePolyHigher ? baseEdgeDir : landEdgeDir;
						float walkableRadius = basePolyHigher ? baseTile->header->walkableRadius : landTile->header->walkableRadius;

						if (!traverseLinkInLOS(m_geom, lowerEdgeMid, higherEdgeMid, higherEdgeDir, walkableRadius))
							continue;

						// Need at least 2 links
						// todo(amos): perhaps optimize this so we check this before raycasting
						// etc.. must also check if the tile isn't external because if so, we need
						// space for 2 links in the same tile.
						const unsigned int forwardIdx = baseTile->allocLink();

						if (forwardIdx == DT_NULL_LINK) // TODO: should move on to next tile.
							continue;

						const unsigned int reverseIdx = landTile->allocLink();

						if (reverseIdx == DT_NULL_LINK) // TODO: should move on to next tile.
						{
							baseTile->freeLink(forwardIdx);
							continue;
						}

						dtLink* const forwardLink = &baseTile->links[forwardIdx];

						forwardLink->ref = m_navMesh->getPolyRefBase(landTile) | (dtPolyRef)m;
						forwardLink->edge = (unsigned char)j;
						forwardLink->side = side;
						forwardLink->bmin = 0;
						forwardLink->bmax = 255;
						forwardLink->next = basePoly->firstLink;
						basePoly->firstLink = forwardIdx;
						forwardLink->traverseType = (unsigned char)traverseType;
						forwardLink->traverseDist = distance;
						forwardLink->reverseLink = (unsigned short)reverseIdx;

						dtLink* const reverseLink = &landTile->links[reverseIdx];

						reverseLink->ref = m_navMesh->getPolyRefBase(baseTile) | (dtPolyRef)i;
						reverseLink->edge = (unsigned char)n;
						reverseLink->side = (unsigned char)rdOppositeTile(side);
						reverseLink->bmin = 0;
						reverseLink->bmax = 255;
						reverseLink->next = landPoly->firstLink;
						landPoly->firstLink = reverseIdx;
						reverseLink->traverseType = (unsigned char)traverseType;
						reverseLink->traverseDist = distance;
						reverseLink->reverseLink = (unsigned short)forwardIdx;
					}
				}
			}
		}
	}
}

bool Editor::createTraverseLinks()
{
	rdAssert(m_navMesh);
	const int maxTiles = m_navMesh->getMaxTiles();

	for (int i = 0; i < maxTiles; i++)
	{
		dtMeshTile* baseTile = m_navMesh->getTile(i);
		if (!baseTile || !baseTile->header)
			continue;

		connectTileTraverseLinks(baseTile, true);
		connectTileTraverseLinks(baseTile, false);
	}

	return true;
}

void Editor::buildStaticPathingData()
{
	if (!m_navMesh) return;

	dtDisjointSet data;

	if (!dtCreateDisjointPolyGroups(m_navMesh, data))
	{
		m_ctx->log(RC_LOG_ERROR, "buildStaticPathingData: Failed to build disjoint poly groups.");
	}

	if (!createTraverseLinks())
	{
		m_ctx->log(RC_LOG_ERROR, "buildStaticPathingData: Failed to build traverse links.");
	}

	if (!dtUpdateDisjointPolyGroups(m_navMesh, data))
	{
		m_ctx->log(RC_LOG_ERROR, "buildStaticPathingData: Failed to update disjoint poly groups.");
	}

	if (!dtCreateTraverseTableData(m_navMesh, data, NavMesh_GetTraverseTableCountForNavMeshType(m_selectedNavMeshType)))
	{
		m_ctx->log(RC_LOG_ERROR, "buildStaticPathingData: Failed to build traverse table data.");
	}
}

void Editor::updateToolStates(const float dt)
{
	for (int i = 0; i < MAX_TOOLS; i++)
	{
		if (m_toolStates[i])
			m_toolStates[i]->handleUpdate(dt);
	}
}

void Editor::initToolStates(Editor* editor)
{
	for (int i = 0; i < MAX_TOOLS; i++)
	{
		if (m_toolStates[i])
			m_toolStates[i]->init(editor);
	}
}

void Editor::resetToolStates()
{
	for (int i = 0; i < MAX_TOOLS; i++)
	{
		if (m_toolStates[i])
			m_toolStates[i]->reset();
	}
}

void Editor::renderToolStates()
{
	for (int i = 0; i < MAX_TOOLS; i++)
	{
		if (m_toolStates[i])
			m_toolStates[i]->handleRender();
	}
}

void Editor::renderOverlayToolStates(double* proj, double* model, int* view)
{
	for (int i = 0; i < MAX_TOOLS; i++)
	{
		if (m_toolStates[i])
			m_toolStates[i]->handleRenderOverlay(proj, model, view);
	}
}

void Editor::renderMeshOffsetOptions()
{
	ImGui::Text("Render Offsets");

	ImGui::PushItemWidth(230);

	ImGui::SliderFloat3("Recast##RenderOffset", m_recastDrawOffset, -500, 500);
	ImGui::SliderFloat3("Detour##RenderOffset", m_detourDrawOffset, -500, 500);

	ImGui::PopItemWidth();
}

void Editor::renderDetourDebugMenu()
{
	ImGui::Text("Detour Render Options");

	bool isEnabled = (getNavMeshDrawFlags() & DU_DRAWNAVMESH_OFFMESHCONS);

	if (ImGui::Checkbox("Off-Mesh Connections", &isEnabled))
		toggleNavMeshDrawFlag(DU_DRAWNAVMESH_OFFMESHCONS);

	isEnabled = (getNavMeshDrawFlags() & DU_DRAWNAVMESH_QUERY_NODES);

	if (ImGui::Checkbox("Query Nodes", &isEnabled))
		toggleNavMeshDrawFlag(DU_DRAWNAVMESH_QUERY_NODES);

	isEnabled = (getNavMeshDrawFlags() & DU_DRAWNAVMESH_BVTREE);

	if (ImGui::Checkbox("BVTree", &isEnabled))
		toggleNavMeshDrawFlag(DU_DRAWNAVMESH_BVTREE);

	isEnabled = (getNavMeshDrawFlags() & DU_DRAWNAVMESH_PORTALS);

	if (ImGui::Checkbox("Portals", &isEnabled))
		toggleNavMeshDrawFlag(DU_DRAWNAVMESH_PORTALS);

	isEnabled = (getNavMeshDrawFlags() & DU_DRAWNAVMESH_WITH_CLOSED_LIST);

	if (ImGui::Checkbox("Closed List", &isEnabled))
		toggleNavMeshDrawFlag(DU_DRAWNAVMESH_WITH_CLOSED_LIST);

	isEnabled = (getNavMeshDrawFlags() & DU_DRAWNAVMESH_TILE_COLORS);

	if (ImGui::Checkbox("Tile ID Colors", &isEnabled))
		toggleNavMeshDrawFlag(DU_DRAWNAVMESH_TILE_COLORS);

	isEnabled = (getNavMeshDrawFlags() & DU_DRAWNAVMESH_TILE_BOUNDS);

	if (ImGui::Checkbox("Tile Bounds", &isEnabled))
		toggleNavMeshDrawFlag(DU_DRAWNAVMESH_TILE_BOUNDS);

#if DT_NAVMESH_SET_VERSION >= 8
	isEnabled = (getNavMeshDrawFlags() & DU_DRAWNAVMESH_TILE_CELLS);

	if (ImGui::Checkbox("Tile Cells", &isEnabled))
		toggleNavMeshDrawFlag(DU_DRAWNAVMESH_TILE_CELLS);
#endif

	isEnabled = (getNavMeshDrawFlags() & DU_DRAWNAVMESH_POLY_VERTS);

	if (ImGui::Checkbox("Vertex Points", &isEnabled))
		toggleNavMeshDrawFlag(DU_DRAWNAVMESH_POLY_VERTS);

	isEnabled = (getNavMeshDrawFlags() & DU_DRAWNAVMESH_POLY_BOUNDS_INNER);

	if (ImGui::Checkbox("Inner Poly Boundaries", &isEnabled))
		toggleNavMeshDrawFlag(DU_DRAWNAVMESH_POLY_BOUNDS_INNER);

	isEnabled = (getNavMeshDrawFlags() & DU_DRAWNAVMESH_POLY_BOUNDS_OUTER);

	if (ImGui::Checkbox("Outer Poly Boundaries", &isEnabled))
		toggleNavMeshDrawFlag(DU_DRAWNAVMESH_POLY_BOUNDS_OUTER);

	isEnabled = (getNavMeshDrawFlags() & DU_DRAWNAVMESH_POLY_CENTERS);

	if (ImGui::Checkbox("Poly Centers", &isEnabled))
		toggleNavMeshDrawFlag(DU_DRAWNAVMESH_POLY_CENTERS);

	isEnabled = (getNavMeshDrawFlags() & DU_DRAWNAVMESH_POLY_GROUPS);

	if (ImGui::Checkbox("Poly Group Colors", &isEnabled))
		toggleNavMeshDrawFlag(DU_DRAWNAVMESH_POLY_GROUPS);

	isEnabled = (getNavMeshDrawFlags() & DU_DRAWNAVMESH_DEPTH_MASK);

	if (ImGui::Checkbox("Depth Mask", &isEnabled))
		toggleNavMeshDrawFlag(DU_DRAWNAVMESH_DEPTH_MASK);

	isEnabled = (getNavMeshDrawFlags() & DU_DRAWNAVMESH_ALPHA);

	if (ImGui::Checkbox("Transparency", &isEnabled))
		toggleNavMeshDrawFlag(DU_DRAWNAVMESH_ALPHA);

	isEnabled = (getNavMeshDrawFlags() & DU_DRAWNAVMESH_TRAVERSE_RAY_OFFSET);

	if (ImGui::Checkbox("Traverse Ray Offsets", &isEnabled))
		toggleNavMeshDrawFlag(DU_DRAWNAVMESH_TRAVERSE_RAY_OFFSET);

	isEnabled = (getNavMeshDrawFlags() & DU_DRAWNAVMESH_TRAVERSE_LINKS);

	if (ImGui::Checkbox("Traverse Links", &isEnabled))
		toggleNavMeshDrawFlag(DU_DRAWNAVMESH_TRAVERSE_LINKS);

	if (isEnabled && m_navMesh) // Supplemental options only available with a valid navmesh!
	{
		ImGui::PushItemWidth(190);
		ImGui::SliderInt("Traverse Type", &m_traverseLinkParams.traverseLinkType, -1, 31);
		ImGui::SliderInt("Traverse Dist", &m_traverseLinkParams.traverseLinkDistance, -1, 255);
		ImGui::SliderInt("Traverse Anim", &m_traverseLinkParams.traverseAnimType, -2, m_navMesh->getParams()->traverseTableCount-1);
		ImGui::PopItemWidth();
	}
}

// NOTE: the climb height should never equal or exceed the agent's height, see https://groups.google.com/g/recastnavigation/c/L5rBamxcOBk/m/5xGLj6YP25kJ
// Quote: "you will get into trouble in cases where there is an overhand which is low enough to step over and high enough for the agent to walk under."
const hulldef hulls[NAVMESH_COUNT] = {
	{ g_navMeshNames[NAVMESH_SMALL]      , NAI_Hull::Width(HULL_HUMAN)   * NAI_Hull::Scale(HULL_HUMAN)  , NAI_Hull::Height(HULL_HUMAN)  , NAI_Hull::Height(HULL_HUMAN)   * NAI_Hull::Scale(HULL_HUMAN)  , 32, 8 },
	{ g_navMeshNames[NAVMESH_MED_SHORT]  , NAI_Hull::Width(HULL_PROWLER) * NAI_Hull::Scale(HULL_PROWLER), NAI_Hull::Height(HULL_PROWLER), NAI_Hull::Height(HULL_PROWLER) * NAI_Hull::Scale(HULL_PROWLER), 32, 4 },
	{ g_navMeshNames[NAVMESH_MEDIUM]     , NAI_Hull::Width(HULL_MEDIUM)  * NAI_Hull::Scale(HULL_MEDIUM) , NAI_Hull::Height(HULL_MEDIUM) , NAI_Hull::Height(HULL_MEDIUM)  * NAI_Hull::Scale(HULL_MEDIUM) , 32, 4 },
	{ g_navMeshNames[NAVMESH_LARGE]      , NAI_Hull::Width(HULL_TITAN)   * NAI_Hull::Scale(HULL_TITAN)  , NAI_Hull::Height(HULL_TITAN)  , NAI_Hull::Height(HULL_TITAN)   * NAI_Hull::Scale(HULL_TITAN)  , 64, 2 },
	{ g_navMeshNames[NAVMESH_EXTRA_LARGE], NAI_Hull::Width(HULL_GOLIATH) * NAI_Hull::Scale(HULL_GOLIATH), NAI_Hull::Height(HULL_GOLIATH), NAI_Hull::Height(HULL_GOLIATH) * NAI_Hull::Scale(HULL_GOLIATH), 64, 2 },
};

void Editor::selectNavMeshType(const NavMeshType_e navMeshType)
{
	const hulldef& h = hulls[navMeshType];

	m_agentRadius = h.radius;
	m_agentMaxClimb = h.climbHeight;
	m_agentHeight = h.height;
	m_navmeshName = h.name;
	m_tileSize = h.tileSize;
	m_polyCellRes = h.cellResolution;

	m_selectedNavMeshType = navMeshType;
}

bool Editor::loadAll(std::string path, const bool fullPath)
{
	dtFreeNavMesh(m_navMesh);
	m_navMesh = nullptr;

	const char* navMeshPath = nullptr;
	char buffer[256];

	if (!fullPath) // Load from model name (e.g. "mp_rr_box").
	{
		fs::path p = "..\\maps\\navmesh\\";
		if (fs::is_directory(p))
		{
			path.insert(0, p.string());
		}

		sprintf(buffer, "%s_%s.nm", path.c_str(), m_navmeshName);
		navMeshPath = buffer;
	}
	else
		navMeshPath = path.c_str();

	FILE* fp = fopen(navMeshPath, "rb");
	if (!fp)
		return false;

	// Read header.
	dtNavMeshSetHeader header;
	size_t readLen = fread(&header, sizeof(dtNavMeshSetHeader), 1, fp);
	if (readLen != 1)
	{
		fclose(fp);
		return false;
	}
	if (header.magic != DT_NAVMESH_SET_MAGIC) // todo(amos) check for tool mode since tilecache uses different constants!
	{
		fclose(fp);
		return false;
	}
	if (header.version != DT_NAVMESH_SET_VERSION) // todo(amos) check for tool mode since tilecache uses different constants!
	{
		fclose(fp);
		return false;
	}

	dtNavMesh* mesh = dtAllocNavMesh();
	if (!mesh)
	{
		fclose(fp);
		return false;
	}


	dtStatus status = mesh->init(&header.params);
	if (dtStatusFailed(status))
	{
		fclose(fp);
		return false;
	}
	
	// Read tiles.
	for (int i = 0; i < header.numTiles; ++i)
	{
		dtNavMeshTileHeader tileHeader;
		readLen = fread(&tileHeader, sizeof(tileHeader), 1, fp);
		if (readLen != 1)
		{
			fclose(fp);
			return false;
		}

		if (!tileHeader.tileRef || !tileHeader.dataSize)
			break;

		unsigned char* data = (unsigned char*)rdAlloc(tileHeader.dataSize, RD_ALLOC_PERM);
		if (!data)
			break;

		memset(data, 0, tileHeader.dataSize);
		readLen = fread(data, tileHeader.dataSize, 1, fp);

		if (readLen != 1)
		{
			rdFree(data);
			fclose(fp);
			return false;
		}

		mesh->addTile(data, tileHeader.dataSize, DT_TILE_FREE_DATA, tileHeader.tileRef, NULL);
	}

	// Read read static pathing data.
	if (header.params.polyGroupCount >= DT_MIN_POLY_GROUP_COUNT)
	{
		for (int i = 0; i < header.params.traverseTableCount; i++)
		{
			int* traverseTable = (int*)rdAlloc(header.params.traverseTableSize, RD_ALLOC_PERM);
			if (!traverseTable)
				break;

			memset(traverseTable, 0, header.params.traverseTableSize);
			readLen = fread(traverseTable, header.params.traverseTableSize, 1, fp);

			if (readLen != 1)
			{
				rdFree(traverseTable);
				fclose(fp);
				return 0;
			}

			mesh->setTraverseTable(i, traverseTable);
		}
	}

	fclose(fp);
	m_navMesh = mesh;

	return true;
}

void Editor::saveAll(std::string path, const dtNavMesh* mesh)
{
	if (!mesh)
		return;

	fs::path p = "..\\maps\\navmesh\\";
	if (fs::is_directory(p))
	{
		path.insert(0, p.string());
	}

	char buffer[256];
	sprintf(buffer, "%s_%s.nm", path.c_str(), m_navmeshName);

	FILE* fp = fopen(buffer, "wb");
	if (!fp)
		return;

	// Store header.
	dtNavMeshSetHeader header;
	header.magic = DT_NAVMESH_SET_MAGIC;
	header.version = DT_NAVMESH_SET_VERSION;
	header.numTiles = 0;

	for (int i = 0; i < mesh->getMaxTiles(); ++i)
	{
		const dtMeshTile* tile = mesh->getTile(i);
		if (!tile || !tile->header || !tile->dataSize)
			continue;

		header.numTiles++;
	}

	const dtNavMeshParams* params = mesh->getParams();

	memcpy(&header.params, params, sizeof(dtNavMeshParams));
	fwrite(&header, sizeof(dtNavMeshSetHeader), 1, fp);

	// Store tiles.
	for (int i = 0; i < mesh->getMaxTiles(); ++i)
	{
		const dtMeshTile* tile = mesh->getTile(i);
		if (!tile || !tile->header || !tile->dataSize)
			continue;

		dtNavMeshTileHeader tileHeader;
		tileHeader.tileRef = mesh->getTileRef(tile);
		tileHeader.dataSize = tile->dataSize;

		fwrite(&tileHeader, sizeof(tileHeader), 1, fp);
		fwrite(tile->data, tile->dataSize, 1, fp);
	}

#if DT_NAVMESH_SET_VERSION == 5
	int mset5Unkown = 0;
	for (int i = 0; i < params->polyGroupCount; i++)
		fwrite(&mset5Unkown, sizeof(int), 1, fp);
#endif

	// Only store if we have 3 or more poly groups.
	if (params->polyGroupCount >= DT_MIN_POLY_GROUP_COUNT)
	{
		int** traverseTables = mesh->getTraverseTables();

		rdAssert(traverseTables);

		for (int i = 0; i < header.params.traverseTableCount; i++)
		{
			const int* const tableData = traverseTables[i];
			rdAssert(tableData);

			fwrite(tableData, sizeof(int), (header.params.traverseTableSize/4), fp);
		}
	}

	fclose(fp);
}

bool Editor::loadNavMesh(const char* path, const bool fullPath)
{
	const bool result = Editor::loadAll(path, fullPath);
	m_navQuery->init(m_navMesh, 2048);

	m_loadedNavMeshType = m_selectedNavMeshType;

	if (m_tool)
	{
		m_tool->reset();
		m_tool->init(this);
	}

	resetToolStates();
	initToolStates(this);

	return result;
}
