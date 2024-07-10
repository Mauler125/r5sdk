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

#ifndef RECASTSAMPLETILEMESH_H
#define RECASTSAMPLETILEMESH_H

#include "Recast/Include/Recast.h"
#include "Detour/Include/DetourNavMesh.h"
#include "NavEditor/Include/ChunkyTriMesh.h"
#include "NavEditor/Include/Editor.h"

class Editor_TileMesh : public Editor
{
protected:
	bool m_keepInterResults;
	bool m_buildAll;
	float m_totalBuildTimeMs;

	unsigned char* m_triareas;
	rcHeightfield* m_solid;
	rcCompactHeightfield* m_chf;
	rcContourSet* m_cset;
	rcPolyMesh* m_pmesh;
	rcPolyMeshDetail* m_dmesh;
	rcConfig m_cfg;	
	
	enum DrawTileMeshFlags
	{
		TM_DRAWFLAGS_INPUT_MESH         = (1<<0),
		TM_DRAWFLAGS_NAVMESH            = (1<<1),
		TM_DRAWFLAGS_VOXELS             = (1<<2),
		TM_DRAWFLAGS_VOXELS_WALKABLE    = (1<<3),
		TM_DRAWFLAGS_COMPACT            = (1<<4),
		TM_DRAWFLAGS_COMPACT_DISTANCE   = (1<<5),
		TM_DRAWFLAGS_COMPACT_REGIONS    = (1<<6),
		TM_DRAWFLAGS_REGION_CONNECTIONS = (1<<7),
		TM_DRAWFLAGS_RAW_CONTOURS       = (1<<8),
		TM_DRAWFLAGS_CONTOURS           = (1<<9),
		TM_DRAWFLAGS_POLYMESH           = (1<<10),
		TM_DRAWFLAGS_POLYMESH_DETAIL    = (1<<11),
	};

	unsigned int m_tileMeshDrawFlags;
	
	int m_maxTiles;
	int m_maxPolysPerTile;
	int m_tileSize;
	
	unsigned int m_tileCol;
	float m_lastBuiltTileBmin[3];
	float m_lastBuiltTileBmax[3];
	float m_tileBuildTime;
	float m_tileMemUsage;
	int m_tileTriCount;

	unsigned char* buildTileMesh(const int tx, const int ty, const float* bmin, const float* bmax, int& dataSize);
	
	void cleanup();
	
	void saveAll(const char* path, const dtNavMesh* mesh);
	dtNavMesh* loadAll(const char* path);
	
public:
	Editor_TileMesh();
	virtual ~Editor_TileMesh();
	
	virtual void handleSettings();
	virtual void handleTools();
	virtual void handleDebugMode();
	virtual void handleRender();
	virtual void handleRenderOverlay(double* proj, double* model, int* view);
	virtual void handleMeshChanged(class InputGeom* geom);
	virtual bool handleBuild();
	virtual void collectSettings(struct BuildSettings& settings);

	void selectNavMeshType(const NavMeshType_e navMeshType);

	inline unsigned int getTileMeshDrawFlags() const { return m_tileMeshDrawFlags; }
	inline void setTileMeshDrawFlags(unsigned int flags) { m_tileMeshDrawFlags = flags; }

	inline void toggleTileMeshDrawFlag(unsigned int flag) { m_tileMeshDrawFlags ^= flag; }
	
	void getTilePos(const float* pos, int& tx, int& ty);
	void getTileExtents(int tx, int ty, float* bmin, float* bmax);

	void buildTile(const float* pos);
	void removeTile(const float* pos);
	void buildAllTiles();
	void removeAllTiles();

	void buildAllHulls();
private:
	// Explicitly disabled copy constructor and copy assignment operator.
	Editor_TileMesh(const Editor_TileMesh&);
	Editor_TileMesh& operator=(const Editor_TileMesh&);
};


#endif // RECASTSAMPLETILEMESH_H
