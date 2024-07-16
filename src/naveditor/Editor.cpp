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
#include "NavEditor/Include/FileTypes.h"
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
	// Ground (0) : light blue
	case EDITOR_POLYAREA_GROUND: return duRGBA(0, 192, 215, 255);
	// Jump : yellow
	case EDITOR_POLYAREA_JUMP: return duRGBA(255, 255, 0, 255);
	// Road : brown
	case EDITOR_POLYAREA_ROAD: return duRGBA(50, 20, 12, 255);
	// Door : cyan
	case EDITOR_POLYAREA_DOOR: return duRGBA(0, 255, 255, 255);
	// Grass : green
	case EDITOR_POLYAREA_GRASS: return duRGBA(0, 255, 0, 255);
	// Water : blue
	case EDITOR_POLYAREA_WATER: return duRGBA(0, 0, 255, 255);
	// Unexpected : orange
	default: return duRGBA(255, 100, 10, 255);
	}
}

Editor::Editor() :
	m_geom(0),
	m_navMesh(0),
	m_navQuery(0),
	m_crowd(0),
	m_navMeshDrawFlags(
		DU_DRAWNAVMESH_OFFMESHCONS|DU_DRAWNAVMESH_CLOSEDLIST|
		DU_DRAWNAVMESH_VERTS|DU_DRAWNAVMESH_INNERBOUND|
		DU_DRAWNAVMESH_OUTERBOUND|DU_DRAWNAVMESH_POLYCENTERS|
		DU_DRAWNAVMESH_ALPHA),
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

void Editor::buildStaticPathingData()
{
	if (!m_navMesh) return;

	dtDisjointSet data;

	if (!dtCreateDisjointPolyGroups(m_navMesh, data))
	{
		m_ctx->log(RC_LOG_ERROR, "buildStaticPathingData: Failed to build disjoint poly groups.");
	}

	if (!dtCreateTraversalTableData(m_navMesh, data, NavMesh_GetTraversalTableCountForNavMeshType(m_selectedNavMeshType)))
	{
		m_ctx->log(RC_LOG_ERROR, "buildStaticPathingData: Failed to build traversal table data.");
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

	isEnabled = (getNavMeshDrawFlags() & DU_DRAWNAVMESH_NODES);

	if (ImGui::Checkbox("Query Nodes", &isEnabled))
		toggleNavMeshDrawFlag(DU_DRAWNAVMESH_NODES);

	isEnabled = (getNavMeshDrawFlags() & DU_DRAWNAVMESH_BVTREE);

	if (ImGui::Checkbox("BVTree", &isEnabled))
		toggleNavMeshDrawFlag(DU_DRAWNAVMESH_BVTREE);

	isEnabled = (getNavMeshDrawFlags() & DU_DRAWNAVMESH_PORTALS);

	if (ImGui::Checkbox("Portals", &isEnabled))
		toggleNavMeshDrawFlag(DU_DRAWNAVMESH_PORTALS);

	isEnabled = (getNavMeshDrawFlags() & DU_DRAWNAVMESH_CLOSEDLIST);

	if (ImGui::Checkbox("Closed List", &isEnabled))
		toggleNavMeshDrawFlag(DU_DRAWNAVMESH_CLOSEDLIST);

	isEnabled = (getNavMeshDrawFlags() & DU_DRAWNAVMESH_COLOR_TILES);

	if (ImGui::Checkbox("Tile ID Colors", &isEnabled))
		toggleNavMeshDrawFlag(DU_DRAWNAVMESH_COLOR_TILES);

	isEnabled = (getNavMeshDrawFlags() & DU_DRAWNAVMESH_VERTS);

	if (ImGui::Checkbox("Vertex Points", &isEnabled))
		toggleNavMeshDrawFlag(DU_DRAWNAVMESH_VERTS);

	isEnabled = (getNavMeshDrawFlags() & DU_DRAWNAVMESH_INNERBOUND);

	if (ImGui::Checkbox("Inner Poly Boundaries", &isEnabled))
		toggleNavMeshDrawFlag(DU_DRAWNAVMESH_INNERBOUND);

	isEnabled = (getNavMeshDrawFlags() & DU_DRAWNAVMESH_OUTERBOUND);

	if (ImGui::Checkbox("Outer Poly Boundaries", &isEnabled))
		toggleNavMeshDrawFlag(DU_DRAWNAVMESH_OUTERBOUND);

	isEnabled = (getNavMeshDrawFlags() & DU_DRAWNAVMESH_POLYCENTERS);

	if (ImGui::Checkbox("Poly Centers", &isEnabled))
		toggleNavMeshDrawFlag(DU_DRAWNAVMESH_POLYCENTERS);

	isEnabled = (getNavMeshDrawFlags() & DU_DRAWNAVMESH_POLYGROUPS);

	if (ImGui::Checkbox("Poly Group Colors", &isEnabled))
		toggleNavMeshDrawFlag(DU_DRAWNAVMESH_POLYGROUPS);

	isEnabled = (getNavMeshDrawFlags() & DU_DRAWNAVMESH_DEPTH_MASK);

	if (ImGui::Checkbox("Depth Mask", &isEnabled))
		toggleNavMeshDrawFlag(DU_DRAWNAVMESH_DEPTH_MASK);

	isEnabled = (getNavMeshDrawFlags() & DU_DRAWNAVMESH_ALPHA);

	if (ImGui::Checkbox("Transparency", &isEnabled))
		toggleNavMeshDrawFlag(DU_DRAWNAVMESH_ALPHA);
}

// NOTE: the climb height should never equal or exceed the agent's height, see https://groups.google.com/g/recastnavigation/c/L5rBamxcOBk/m/5xGLj6YP25kJ
// Quote: "you will get into trouble in cases where there is an overhand which is low enough to step over and high enough for the agent to walk under."
const hulldef hulls[NAVMESH_COUNT] = {
	{ g_navMeshNames[NAVMESH_SMALL]      , NAI_Hull::Width(HULL_HUMAN)   * NAI_Hull::Scale(HULL_HUMAN)  , NAI_Hull::Height(HULL_HUMAN)  , NAI_Hull::Height(HULL_HUMAN)   * NAI_Hull::Scale(HULL_HUMAN)  , 32 },
	{ g_navMeshNames[NAVMESH_MED_SHORT]  , NAI_Hull::Width(HULL_PROWLER) * NAI_Hull::Scale(HULL_PROWLER), NAI_Hull::Height(HULL_PROWLER), NAI_Hull::Height(HULL_PROWLER) * NAI_Hull::Scale(HULL_PROWLER), 32 },
	{ g_navMeshNames[NAVMESH_MEDIUM]     , NAI_Hull::Width(HULL_MEDIUM)  * NAI_Hull::Scale(HULL_MEDIUM) , NAI_Hull::Height(HULL_MEDIUM) , NAI_Hull::Height(HULL_MEDIUM)  * NAI_Hull::Scale(HULL_MEDIUM) , 32 },
	{ g_navMeshNames[NAVMESH_LARGE]      , NAI_Hull::Width(HULL_TITAN)   * NAI_Hull::Scale(HULL_TITAN)  , NAI_Hull::Height(HULL_TITAN)  , NAI_Hull::Height(HULL_TITAN)   * NAI_Hull::Scale(HULL_TITAN)  , 64 },
	{ g_navMeshNames[NAVMESH_EXTRA_LARGE], NAI_Hull::Width(HULL_GOLIATH) * NAI_Hull::Scale(HULL_GOLIATH), NAI_Hull::Height(HULL_GOLIATH), NAI_Hull::Height(HULL_GOLIATH) * NAI_Hull::Scale(HULL_GOLIATH), 64 },
};

void Editor::selectNavMeshType(const NavMeshType_e navMeshType)
{
	const hulldef& h = hulls[navMeshType];

	m_agentRadius = h.radius;
	m_agentMaxClimb = h.climbHeight;
	m_agentHeight = h.height;
	m_navmeshName = h.name;
	m_tileSize = h.tileSize;

	m_selectedNavMeshType = navMeshType;
}

dtNavMesh* Editor::loadAll(std::string path)
{
	fs::path p = "..\\maps\\navmesh\\";
	if (fs::is_directory(p))
	{
		path.insert(0, p.string());
	}

	char buffer[256];
	sprintf(buffer, "%s_%s.nm", path.c_str(), m_navmeshName);

	FILE* fp = fopen(buffer, "rb");
	if (!fp)
		return 0;

	// Read header.
	NavMeshSetHeader header;
	size_t readLen = fread(&header, sizeof(NavMeshSetHeader), 1, fp);
	if (readLen != 1)
	{
		fclose(fp);
		return 0;
	}
	if (header.magic != NAVMESHSET_MAGIC) // todo(amos) check for tool mode since tilecache uses different constants!
	{
		fclose(fp);
		return 0;
	}
	if (header.version != NAVMESHSET_VERSION) // todo(amos) check for tool mode since tilecache uses different constants!
	{
		fclose(fp);
		return 0;
	}

	dtNavMesh* mesh = dtAllocNavMesh();
	if (!mesh)
	{
		fclose(fp);
		return 0;
	}


	dtStatus status = mesh->init(&header.params);
	if (dtStatusFailed(status))
	{
		fclose(fp);
		return 0;
	}
	
	// Read tiles.
	for (int i = 0; i < header.numTiles; ++i)
	{
		NavMeshTileHeader tileHeader;
		readLen = fread(&tileHeader, sizeof(tileHeader), 1, fp);
		if (readLen != 1)
		{
			fclose(fp);
			return 0;
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
			return 0;
		}

		mesh->addTile(data, tileHeader.dataSize, DT_TILE_FREE_DATA, tileHeader.tileRef, NULL);
	}

	// Read read static pathing data.
	if (header.params.polyGroupCount >= DT_MIN_POLY_GROUP_COUNT)
	{
		for (int i = 0; i < header.params.traversalTableCount; i++)
		{
			int* traversalTable = (int*)rdAlloc(header.params.traversalTableSize, RD_ALLOC_PERM);
			if (!traversalTable)
				break;

			memset(traversalTable, 0, header.params.traversalTableSize);
			readLen = fread(traversalTable, header.params.traversalTableSize, 1, fp);

			if (readLen != 1)
			{
				rdFree(traversalTable);
				fclose(fp);
				return 0;
			}

			mesh->m_traversalTables[i] = traversalTable;
		}
	}

	fclose(fp);
	return mesh;
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
	NavMeshSetHeader header;
	header.magic = NAVMESHSET_MAGIC;
	header.version = NAVMESHSET_VERSION;
	header.numTiles = 0;

	for (int i = 0; i < mesh->getMaxTiles(); ++i)
	{
		const dtMeshTile* tile = mesh->getTile(i);
		if (!tile || !tile->header || !tile->dataSize)
			continue;

		header.numTiles++;
	}

	memcpy(&header.params, mesh->getParams(), sizeof(dtNavMeshParams));
	fwrite(&header, sizeof(NavMeshSetHeader), 1, fp);

	// Store tiles.
	for (int i = 0; i < mesh->getMaxTiles(); ++i)
	{
		const dtMeshTile* tile = mesh->getTile(i);
		if (!tile || !tile->header || !tile->dataSize)
			continue;

		NavMeshTileHeader tileHeader;
		tileHeader.tileRef = mesh->getTileRef(tile);
		tileHeader.dataSize = tile->dataSize;

		fwrite(&tileHeader, sizeof(tileHeader), 1, fp);
		fwrite(tile->data, tile->dataSize, 1, fp);
	}

	// Only store if we have 3 or more poly groups.
	if (mesh->m_params.polyGroupCount >= DT_MIN_POLY_GROUP_COUNT)
	{
		rdAssert(mesh->m_traversalTables);

		for (int i = 0; i < header.params.traversalTableCount; i++)
		{
			const int* const tableData = mesh->m_traversalTables[i];
			rdAssert(tableData);

			fwrite(tableData, sizeof(int), (header.params.traversalTableSize/4), fp);
		}
	}

	fclose(fp);
}