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
#include "Recast/Include/Recast.h"
#include "Detour/Include/DetourNavMesh.h"
#include "Detour/Include/DetourNavMeshQuery.h"
#include "DetourCrowd/Include/DetourCrowd.h"
#include "DebugUtils/Include/RecastDebugDraw.h"
#include "DebugUtils/Include/DetourDebugDraw.h"
#include "NavEditor/Include/Sample.h"
#include "NavEditor/Include/InputGeom.h"

unsigned int SampleDebugDraw::areaToCol(unsigned int area)
{
	switch(area)
	{
	// Ground (0) : light blue
	case SAMPLE_POLYAREA_GROUND: return duRGBA(0, 120, 255, 255);
	// Water : blue
	case SAMPLE_POLYAREA_WATER: return duRGBA(0, 0, 255, 255);
	// Road : brown
	case SAMPLE_POLYAREA_ROAD: return duRGBA(50, 20, 12, 255);
	// Door : cyan
	case SAMPLE_POLYAREA_DOOR: return duRGBA(0, 255, 255, 255);
	// Grass : green
	case SAMPLE_POLYAREA_GRASS: return duRGBA(0, 255, 0, 255);
	// Jump : yellow
	case SAMPLE_POLYAREA_JUMP: return duRGBA(255, 255, 0, 255);
	// Unexpected : red
	default: return duRGBA(255, 0, 0, 255);
	}
}

Sample::Sample() :
	m_geom(0),
	m_navMesh(0),
	m_navQuery(0),
	m_crowd(0),
	m_navMeshDrawFlags(DU_DRAWNAVMESH_OFFMESHCONS|DU_DRAWNAVMESH_CLOSEDLIST),
	m_filterLowHangingObstacles(true),
	m_filterLedgeSpans(true),
	m_filterWalkableLowHeightSpans(true),
	m_tool(0),
	m_ctx(0)
{
	resetCommonSettings();
	m_navQuery = dtAllocNavMeshQuery();
	m_crowd = dtAllocCrowd();

	for (int i = 0; i < MAX_TOOLS; i++)
		m_toolStates[i] = 0;
}

Sample::~Sample()
{
	dtFreeNavMeshQuery(m_navQuery);
	dtFreeNavMesh(m_navMesh);
	dtFreeCrowd(m_crowd);
	delete m_tool;
	for (int i = 0; i < MAX_TOOLS; i++)
		delete m_toolStates[i];
}

void Sample::setTool(SampleTool* tool)
{
	delete m_tool;
	m_tool = tool;
	if (tool)
		m_tool->init(this);
}

void Sample::handleSettings()
{
}

void Sample::handleTools()
{
}

void Sample::handleDebugMode()
{
}

void Sample::handleRender()
{
	if (!m_geom)
		return;
	
	// Draw mesh
	duDebugDrawTriMesh(&m_dd, m_geom->getMesh()->getVerts(), m_geom->getMesh()->getVertCount(),
					   m_geom->getMesh()->getTris(), m_geom->getMesh()->getNormals(), m_geom->getMesh()->getTriCount(), 0, 1.0f);
	// Draw bounds
	const float* bmin = m_geom->getMeshBoundsMin();
	const float* bmax = m_geom->getMeshBoundsMax();
	duDebugDrawBoxWire(&m_dd, bmin[0],bmin[1],bmin[2], bmax[0],bmax[1],bmax[2], duRGBA(255,255,255,128), 1.0f);
}

void Sample::handleRenderOverlay(double* /*proj*/, double* /*model*/, int* /*view*/)
{
}

void Sample::handleMeshChanged(InputGeom* geom)
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

void Sample::collectSettings(BuildSettings& settings)
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


void Sample::resetCommonSettings()
{
	m_cellSize = 15.0f;
	m_cellHeight = 5.85f;
	m_agentHeight = 2.0f;
	m_agentRadius = 0.6f;
	m_agentMaxClimb = 0.9f;
	m_agentMaxSlope = 45.0f;
	m_regionMinSize = 8;
	m_regionMergeSize = 20;
	m_edgeMaxLen = 12.0f;
	m_edgeMaxError = 1.3f;
	m_vertsPerPoly = 6.0f;
	m_detailSampleDist = 6.0f;
	m_detailSampleMaxError = 1.0f;
	m_partitionType = SAMPLE_PARTITION_WATERSHED;
	m_count_reachability_tables = 1;
}
hulldef hulls[5] = {
	{"small",8,72*0.5,70,512.0f},
	{"med_short",20,72*0.5,75,512.0f},
	{"medium",48,150*0.5,77,512.0f},
	{"large",60,235*0.5,80,960.0f},
	{"extra_large",88,235*0.5,80,960.0f},
};
void Sample::handleCommonSettings()
{
	bool is_human = true;
	for (auto& h : hulls)
	{
		if (imguiButton(h.name))
		{
			m_agentRadius = h.radius;
			m_agentMaxClimb = h.climb_height;
			m_agentHeight = h.height;
			if (is_human)
				m_count_reachability_tables = 4;
			m_navmesh_name = h.name;
		}
		is_human = false;
	}
	imguiLabel("Rasterization");
	imguiSlider("Cell Size", &m_cellSize, 0.1f, 100.0f, 0.01f);
	imguiSlider("Cell Height", &m_cellHeight, 0.1f, 100.0f, 0.01f);
	
	if (m_geom)
	{
		const float* bmin = m_geom->getNavMeshBoundsMin();
		const float* bmax = m_geom->getNavMeshBoundsMax();
		int gw = 0, gh = 0;
		rcCalcGridSize(bmin, bmax, m_cellSize, &gw, &gh);
		char text[64];
		snprintf(text, 64, "Voxels  %d x %d", gw, gh);
		imguiValue(text);
	}
	
	imguiSeparator();
	imguiLabel("Agent");
	imguiSlider("Height", &m_agentHeight, 0.1f, 300.0f, 0.1f);
	imguiSlider("Radius", &m_agentRadius, 0.0f, 100.0f, 0.1f);
	imguiSlider("Max Climb", &m_agentMaxClimb, 0.1f, 120.0f, 0.1f);
	imguiSlider("Max Slope", &m_agentMaxSlope, 0.0f, 90.0f, 1.0f);
	
	imguiSeparator();
	imguiLabel("Region");
	imguiSlider("Min Region Size", &m_regionMinSize, 0.0f, 150.0f, 1.0f);
	imguiSlider("Merged Region Size", &m_regionMergeSize, 0.0f, 150.0f, 1.0f);

	imguiSeparator();
	imguiLabel("Partitioning");
	if (imguiCheck("Watershed", m_partitionType == SAMPLE_PARTITION_WATERSHED))
		m_partitionType = SAMPLE_PARTITION_WATERSHED;
	if (imguiCheck("Monotone", m_partitionType == SAMPLE_PARTITION_MONOTONE))
		m_partitionType = SAMPLE_PARTITION_MONOTONE;
	if (imguiCheck("Layers", m_partitionType == SAMPLE_PARTITION_LAYERS))
		m_partitionType = SAMPLE_PARTITION_LAYERS;
	
	imguiSeparator();
	imguiLabel("Filtering");
	if (imguiCheck("Low Hanging Obstacles", m_filterLowHangingObstacles))
		m_filterLowHangingObstacles = !m_filterLowHangingObstacles;
	if (imguiCheck("Ledge Spans", m_filterLedgeSpans))
		m_filterLedgeSpans= !m_filterLedgeSpans;
	if (imguiCheck("Walkable Low Height Spans", m_filterWalkableLowHeightSpans))
		m_filterWalkableLowHeightSpans = !m_filterWalkableLowHeightSpans;

	imguiSeparator();
	imguiLabel("Polygonization");
	imguiSlider("Max Edge Length", &m_edgeMaxLen, 0.0f, 50.0f, 1.0f);
	imguiSlider("Max Edge Error", &m_edgeMaxError, 0.1f, 3.0f, 0.1f);
	imguiSlider("Verts Per Poly", &m_vertsPerPoly, 3.0f, 12.0f, 1.0f,false);		

	imguiSeparator();
	imguiLabel("Detail Mesh");
	imguiSlider("Sample Distance", &m_detailSampleDist, 0.0f, 16.0f, 1.0f);
	imguiSlider("Max Sample Error", &m_detailSampleMaxError, 0.0f, 16.0f, 1.0f);
	
	imguiSeparator();
}

void Sample::handleClick(const float* s, const float* p, bool shift)
{
	if (m_tool)
		m_tool->handleClick(s, p, shift);
}

void Sample::handleToggle()
{
	if (m_tool)
		m_tool->handleToggle();
}

void Sample::handleStep()
{
	if (m_tool)
		m_tool->handleStep();
}

bool Sample::handleBuild()
{
	return true;
}

void Sample::handleUpdate(const float dt)
{
	if (m_tool)
		m_tool->handleUpdate(dt);
	updateToolStates(dt);
}


void Sample::updateToolStates(const float dt)
{
	for (int i = 0; i < MAX_TOOLS; i++)
	{
		if (m_toolStates[i])
			m_toolStates[i]->handleUpdate(dt);
	}
}

void Sample::initToolStates(Sample* sample)
{
	for (int i = 0; i < MAX_TOOLS; i++)
	{
		if (m_toolStates[i])
			m_toolStates[i]->init(sample);
	}
}

void Sample::resetToolStates()
{
	for (int i = 0; i < MAX_TOOLS; i++)
	{
		if (m_toolStates[i])
			m_toolStates[i]->reset();
	}
}

void Sample::renderToolStates()
{
	for (int i = 0; i < MAX_TOOLS; i++)
	{
		if (m_toolStates[i])
			m_toolStates[i]->handleRender();
	}
}

void Sample::renderOverlayToolStates(double* proj, double* model, int* view)
{
	for (int i = 0; i < MAX_TOOLS; i++)
	{
		if (m_toolStates[i])
			m_toolStates[i]->handleRenderOverlay(proj, model, view);
	}
}

static const int NAVMESHSET_MAGIC = 'M'<<24 | 'S'<<16 | 'E'<<8 | 'T'; //'MSET';
static const int NAVMESHSET_VERSION = 8;

struct NavMeshSetHeader
{
	int magic;
	int version;
	int numTiles;
	dtNavMeshParams params;
};

struct NavMeshTileHeader
{
	dtTileRef tileRef;
	int dataSize;
};

void coord_tf_fix(float* c)
{
	std::swap(c[1], c[2]);
	c[2] *= -1;
}
void coord_tf_unfix(float* c)
{
	c[2] *= -1;
	std::swap(c[1], c[2]);
}
void coord_short_tf_fix(unsigned short* c)
{
	std::swap(c[1], c[2]);
	c[2] = std::numeric_limits<unsigned short>::max() - c[2];
}
void coord_short_tf_unfix(unsigned short* c)
{
	c[2] = std::numeric_limits<unsigned short>::max() - c[2];
	std::swap(c[1], c[2]);
}
void patch_headertf2(NavMeshSetHeader& h)
{
	coord_tf_fix(h.params.orig);
}
void unpatch_headertf2(NavMeshSetHeader& h)
{
	coord_tf_unfix(h.params.orig);
}

void patch_tiletf2(dtMeshTile* t)
{
	coord_tf_fix(t->header->bmin);
	coord_tf_fix(t->header->bmax);

	for (size_t i = 0; i < t->header->vertCount * 3; i += 3)
		coord_tf_fix(t->verts + i);
	for (size_t i = 0; i < t->header->detailVertCount * 3; i += 3)
		coord_tf_fix(t->detailVerts + i);
	for (size_t i = 0; i < t->header->polyCount; i++)
		coord_tf_fix(t->polys[i].org);
	//might be wrong because of coord change might break tree layout
	for (size_t i = 0; i < t->header->bvNodeCount; i++)
	{
		coord_short_tf_fix(t->bvTree[i].bmax);
		coord_short_tf_fix(t->bvTree[i].bmin);
	}
	for (size_t i = 0; i < t->header->offMeshConCount; i++)
	{
		coord_tf_fix(t->offMeshCons[i].pos);
		coord_tf_fix(t->offMeshCons[i].pos + 3);
		coord_tf_fix(t->offMeshCons[i].unk);
	}
}
void unpatch_tiletf2(dtMeshTile* t)
{
	coord_tf_unfix(t->header->bmin);
	coord_tf_unfix(t->header->bmax);

	for (size_t i = 0; i < t->header->vertCount * 3; i += 3)
		coord_tf_unfix(t->verts + i);
	for (size_t i = 0; i < t->header->detailVertCount * 3; i += 3)
		coord_tf_unfix(t->detailVerts + i);
	for (size_t i = 0; i < t->header->polyCount; i++)
		coord_tf_unfix(t->polys[i].org);
	//might be wrong because of coord change might break tree layout
	for (size_t i = 0; i < t->header->bvNodeCount; i++)
	{
		coord_short_tf_unfix(t->bvTree[i].bmax);
		coord_short_tf_unfix(t->bvTree[i].bmin);
	}
	for (size_t i = 0; i < t->header->offMeshConCount; i++)
	{
		coord_tf_unfix(t->offMeshCons[i].pos);
		coord_tf_unfix(t->offMeshCons[i].pos+3);
		coord_tf_unfix(t->offMeshCons[i].unk);
	}
}
struct LinkTableData
{
	//disjoint set algo from some crappy site because i'm too lazy to think
	int setCount = 0;
	std::vector<int> rank;
	std::vector<int> parent;
	void init(int size)
	{
		rank.resize(size);
		parent.resize(size);

		for (int i = 0; i < parent.size(); i++)
			parent[i] = i;
	}
	int insert_new()
	{
		rank.push_back(0);
		parent.push_back(setCount);
		return setCount++;
	}
	int find(int id)
	{
		if (parent[id] != id)
			return find(parent[id]);
		return id;
	}
	void set_union(int x, int y)
	{
		int sx = find(x);
		int sy = find(y);
		if (sx == sy) //same set already
			return;

		if (rank[sx] < rank[sy])
			parent[sx] = sy;
		else if (rank[sx] > rank[sy])
			parent[sy] = sx;
		else
		{
			parent[sy] = sx;
			rank[sx] += 1;
		}
	}
};
void build_link_table(dtNavMesh* mesh, LinkTableData& data)
{
	//clear all labels
	for (int i = 0; i < mesh->getMaxTiles(); ++i)
	{
		dtMeshTile* tile = mesh->getTile(i);
		if (!tile || !tile->header || !tile->dataSize) continue;
		auto pcount = tile->header->polyCount;
		for (int j = 0; j < pcount; j++)
		{
			auto& poly = tile->polys[j];
			poly.link_table_idx = -1;
		}
	}
	//first pass
	std::set<int> nlabels;
	for (int i = 0; i < mesh->getMaxTiles(); ++i)
	{
		dtMeshTile* tile = mesh->getTile(i);
		if (!tile || !tile->header || !tile->dataSize) continue;
		auto pcount = tile->header->polyCount;
		for (int j = 0; j < pcount; j++)
		{
			auto& poly = tile->polys[j];
			auto plink = poly.firstLink;
			while (plink != DT_NULL_LINK)
			{
				auto l = tile->links[plink];
				const dtMeshTile* t;
				const dtPoly* p;
				mesh->getTileAndPolyByRefUnsafe(l.ref, &t, &p);

				if (p->link_table_idx != (unsigned short)-1)
					nlabels.insert(p->link_table_idx);
				plink = l.next;
			}
			if (nlabels.empty())
			{
				poly.link_table_idx = data.insert_new();
			}
			else
			{
				auto l = *nlabels.begin();
				poly.link_table_idx = l;
				for (auto nl : nlabels)
					data.set_union(l, nl);
			}
			nlabels.clear();
		}
	}
	//second pass
	for (int i = 0; i < mesh->getMaxTiles(); ++i)
	{
		dtMeshTile* tile = mesh->getTile(i);
		if (!tile || !tile->header || !tile->dataSize) continue;
		auto pcount = tile->header->polyCount;
		for (int j = 0; j < pcount; j++)
		{
			auto& poly = tile->polys[j];
			auto id = data.find(poly.link_table_idx);
			poly.link_table_idx = id;
		}
	}
}
void set_reachable(std::vector<int>& data, int count, int id1, int id2, bool value)
{
	int w = ((count + 31) / 32);
	auto& cell = data[id1 * w + id2 / 32];
	uint32_t value_mask = ~(1 << (id2 & 0x1f));
	if (!value)
		cell = (cell & value_mask);
	else
		cell = (cell & value_mask) | (1 << (id2 & 0x1f));
}

dtNavMesh* Sample::loadAll(const char* path)
{

	char buffer[256];
	sprintf(buffer, "%s_%s.nm", path, m_navmesh_name);

	FILE* fp = fopen(buffer, "rb");
	if (!fp) return 0;

	// Read header.
	NavMeshSetHeader header;
	size_t readLen = fread(&header, sizeof(NavMeshSetHeader), 1, fp);
	if (readLen != 1)
	{
		fclose(fp);
		return 0;
	}
	if (header.magic != NAVMESHSET_MAGIC)
	{
		fclose(fp);
		return 0;
	}
	if (header.version != NAVMESHSET_VERSION)
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
	if(*is_tf2) patch_headertf2(header);
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

		unsigned char* data = (unsigned char*)dtAlloc(tileHeader.dataSize, DT_ALLOC_PERM);
		if (!data) break;
		memset(data, 0, tileHeader.dataSize);
		readLen = fread(data, tileHeader.dataSize, 1, fp);
		if (readLen != 1)
		{
			dtFree(data);
			fclose(fp);
			return 0;
		}
		dtTileRef result;
		mesh->addTile(data, tileHeader.dataSize, DT_TILE_FREE_DATA, tileHeader.tileRef, &result);
		auto tile = const_cast<dtMeshTile*>(mesh->getTileByRef(result));
		if (*is_tf2) patch_tiletf2(tile);
	}

	fclose(fp);

	return mesh;
}

void Sample::saveAll(std::string path, dtNavMesh* mesh)
{
	if (!mesh) return;

	std::filesystem::path p = "..\\maps\\navmesh\\";

	if (std::filesystem::is_directory(p))
	{
		path.insert(0, p.string());
	}

	char buffer[256];
	sprintf(buffer, "%s_%s.nm", path.c_str(), m_navmesh_name);

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
		dtMeshTile* tile = mesh->getTile(i);
		if (!tile || !tile->header || !tile->dataSize) continue;
		header.numTiles++;
	}
	memcpy(&header.params, mesh->getParams(), sizeof(dtNavMeshParams));
	header.params.disjoint_poly_group_count = 3;
	header.params.reachability_table_size = ((header.params.disjoint_poly_group_count + 31) / 32) * header.params.disjoint_poly_group_count * 32;

	if (*is_tf2)unpatch_headertf2(header);
	fwrite(&header, sizeof(NavMeshSetHeader), 1, fp);

	// Store tiles.
	for (int i = 0; i < mesh->getMaxTiles(); ++i)
	{
		dtMeshTile* tile = mesh->getTile(i);
		if (!tile || !tile->header || !tile->dataSize) continue;

		NavMeshTileHeader tileHeader;
		tileHeader.tileRef = mesh->getTileRef(tile);
		tileHeader.dataSize = tile->dataSize;
		fwrite(&tileHeader, sizeof(tileHeader), 1, fp);

		if (*is_tf2)unpatch_tiletf2(const_cast<dtMeshTile*>(tile));
		fwrite(tile->data, tile->dataSize, 1, fp);
		if (*is_tf2)patch_tiletf2(const_cast<dtMeshTile*>(tile));
	}

	int header_sth[3] = { 0,0,0 };
	fwrite(header_sth, sizeof(int), 3, fp);
	unsigned int reachability[32 * 3];
	for (int i = 0; i < 32 * 3; i++)
		reachability[i] = 0xffffffff;

	for (int i = 0; i < header.params.reachability_table_count; i++)
		fwrite(reachability, sizeof(int), (header.params.reachability_table_size / 4), fp);
	fclose(fp);
}

//void Sample::saveAll(std::string path, dtNavMesh* mesh)
//{
//	if (!mesh) return;
//
//	std::filesystem::path p = "..\\maps\\navmesh\\";
//
//	if (std::filesystem::is_directory(p))
//	{
//		path.insert(0, p.string());
//	}
//
//	char buffer[256];
//	sprintf(buffer, "%s_%s.nm", path.c_str(), m_navmesh_name);
//
//	FILE* fp = fopen(buffer, "wb");
//	if (!fp)
//		return;
//
//	// Store header.
//	NavMeshSetHeader header;
//	header.magic = NAVMESHSET_MAGIC;
//	header.version = NAVMESHSET_VERSION;
//	header.numTiles = 0;
//
//	for (int i = 0; i < mesh->getMaxTiles(); ++i)
//	{
//		dtMeshTile* tile = mesh->getTile(i);
//		if (!tile || !tile->header || !tile->dataSize) continue;
//		header.numTiles++;
//	}
//	memcpy(&header.params, mesh->getParams(), sizeof(dtNavMeshParams));
//	header.params.disjoint_poly_group_count = 3;
//
//	LinkTableData link_data;
//	build_link_table(mesh, link_data);
//	int table_size = ((link_data.setCount + 31) / 32) * link_data.setCount * 32;
//	header.params.disjoint_poly_group_count = link_data.setCount;
//	header.params.reachability_table_count = m_count_reachability_tables;
//	header.params.reachability_table_size = table_size;
//
//	if (*is_tf2)unpatch_headertf2(header);
//	fwrite(&header, sizeof(NavMeshSetHeader), 1, fp);
//
//	// Store tiles.
//	for (int i = 0; i < mesh->getMaxTiles(); ++i)
//	{
//		dtMeshTile* tile = mesh->getTile(i);
//		if (!tile || !tile->header || !tile->dataSize) continue;
//
//		NavMeshTileHeader tileHeader;
//		tileHeader.tileRef = mesh->getTileRef(tile);
//		tileHeader.dataSize = tile->dataSize;
//		fwrite(&tileHeader, sizeof(tileHeader), 1, fp);
//
//		if (*is_tf2)unpatch_tiletf2(const_cast<dtMeshTile*>(tile));
//		fwrite(tile->data, tile->dataSize, 1, fp);
//		if (*is_tf2)patch_tiletf2(const_cast<dtMeshTile*>(tile));
//	}
//
//	//still dont know what this thing is...
//	int header_sth = 0;
//	for (int i = 0; i < link_data.setCount; i++)
//		fwrite(&header_sth, sizeof(int), 1, fp);
//
//	std::vector<int> reachability(table_size, 0);
//	for (int i = 0; i < link_data.setCount; i++)
//		set_reachable(reachability, link_data.setCount, i, i, true);
//	for (int i = 0; i < header.params.reachability_table_count; i++)
//		fwrite(reachability.data(), sizeof(int), (table_size / 4), fp);
//	fclose(fp);
//}