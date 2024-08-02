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

#ifndef RECASTEDITORTILEMESH_H
#define RECASTEDITORTILEMESH_H

#include "Recast/Include/Recast.h"
#include "Detour/Include/DetourNavMesh.h"
#include "NavEditor/Include/ChunkyTriMesh.h"
#include "NavEditor/Include/Editor.h"
#include "NavEditor/Include/Editor_Common.h"

class Editor_TileMesh : public Editor_StaticTileMeshCommon
{
protected:
	bool m_buildAll;
	
	int m_maxTiles;
	int m_maxPolysPerTile;
	
	float m_tileBuildTime;
	float m_tileMemUsage;
	int m_tileTriCount;

	unsigned char* buildTileMesh(const int tx, const int ty, const float* bmin, const float* bmax, int& dataSize);
	
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


#endif // RECASTEDITORTILEMESH_H
