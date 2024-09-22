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

#ifndef INPUTGEOM_H
#define INPUTGEOM_H

#include "NavEditor/Include/ChunkyTriMesh.h"
#include "NavEditor/Include/MeshLoaderObj.h"

enum VolumeType : unsigned char
{
	VOLUME_INVALID = 0xff,
	VOLUME_BOX = 0,
	VOLUME_CYLINDER,
	VOLUME_CONVEX
};

enum TraceMask : unsigned int
{
	TRACE_WORLD   = 1<<0, // The imported world geometry.
	TRACE_CLIP    = 1<<1, // Clip brushes.
	TRACE_TRIGGER = 1<<2, // Trigger brushes.
	TRACE_ALL = 0xffffffff
};

static const int MAX_SHAPEVOL_PTS = 12;
struct ShapeVolume
{
	ShapeVolume()
	{
		for (int i = 0; i < MAX_SHAPEVOL_PTS; i++)
		{
			rdVset(&verts[i*3], 0.f,0.f,0.f);
		}
		hmin = 0.f;
		hmax = 0.f;
		nverts = 0;
		flags = 0;
		area = 0;
		type = VOLUME_INVALID;
	}

	float verts[MAX_SHAPEVOL_PTS*3];
	float hmin, hmax;
	int nverts;
	unsigned short flags;
	unsigned char area;
	unsigned char type;
};

struct BuildSettings
{
	// Cell size in world units
	float cellSize;
	// Cell height in world units
	float cellHeight;
	// Agent height in world units
	float agentHeight;
	// Agent radius in world units
	float agentRadius;
	// Agent max climb in world units
	float agentMaxClimb;
	// Agent max slope in degrees
	float agentMaxSlope;
	// Region minimum size in voxels.
	// regionMinSize = sqrt(regionMinArea)
	int regionMinSize;
	// Region merge size in voxels.
	// regionMergeSize = sqrt(regionMergeArea)
	int regionMergeSize;
	// Edge max length in world units
	int edgeMaxLen;
	// Edge max error in voxels
	float edgeMaxError;
	int vertsPerPoly;
	// The polygon cell resolution.
	int polyCellRes;
	// Detail sample distance in voxels
	float detailSampleDist;
	// Detail sample max error in voxel heights.
	float detailSampleMaxError;
	// Partition type, see SamplePartitionType
	int partitionType;
	// Bounds of the area to mesh
	float navMeshBMin[3];
	float navMeshBMax[3];
	// Original bounds of the area to mesh.
	float origNavMeshBMin[3];
	float origNavMeshBMax[3];
	// Size of the tiles in voxels
	int tileSize;
};

class InputGeom
{
	rcChunkyTriMesh* m_chunkyMesh;
	IMeshLoader* m_mesh;
	float m_meshBMin[3], m_meshBMax[3];
	float m_navMeshBMin[3], m_navMeshBMax[3];
	BuildSettings m_buildSettings;
	bool m_hasBuildSettings;
	
	/// @name Off-Mesh connections.
	///@{
	static const int MAX_OFFMESH_CONNECTIONS = 1024;
	float m_offMeshConVerts[MAX_OFFMESH_CONNECTIONS*3*2];
	float m_offMeshConRefPos[MAX_OFFMESH_CONNECTIONS*3];
	float m_offMeshConRads[MAX_OFFMESH_CONNECTIONS];
	float m_offMeshConRefYaws[MAX_OFFMESH_CONNECTIONS];
	unsigned char m_offMeshConDirs[MAX_OFFMESH_CONNECTIONS];
	unsigned char m_offMeshConJumps[MAX_OFFMESH_CONNECTIONS];
	unsigned char m_offMeshConOrders[MAX_OFFMESH_CONNECTIONS];
	unsigned char m_offMeshConAreas[MAX_OFFMESH_CONNECTIONS];
	unsigned short m_offMeshConFlags[MAX_OFFMESH_CONNECTIONS];
	unsigned short m_offMeshConId[MAX_OFFMESH_CONNECTIONS];
	short m_offMeshConCount;
	///@}

	/// @name Convex Volumes.
	///@{
	static const int MAX_VOLUMES = 256;
	ShapeVolume m_volumes[MAX_VOLUMES];
	int m_volumeCount;
	///@}
	
	bool loadMesh(class rcContext* ctx, const std::string& filepath);
	bool loadPlyMesh(class rcContext* ctx, const std::string& filepath);
	bool loadGeomSet(class rcContext* ctx, const std::string& filepath);
public:
	InputGeom();
	~InputGeom();
	
	
	bool load(class rcContext* ctx, const std::string& filepath);
	bool saveGeomSet(const BuildSettings* settings);
	
	/// Method to return static mesh data.
	const IMeshLoader* getMesh() const { return m_mesh; }
	const float* getMeshBoundsMin() const { return m_meshBMin; }
	const float* getMeshBoundsMax() const { return m_meshBMax; }

	float* getNavMeshBoundsMin() { return m_hasBuildSettings ? m_buildSettings.navMeshBMin : m_navMeshBMin; }
	float* getNavMeshBoundsMax() { return m_hasBuildSettings ? m_buildSettings.navMeshBMax : m_navMeshBMax; }

	const float* getNavMeshBoundsMin() const { return m_hasBuildSettings ? m_buildSettings.navMeshBMin : m_navMeshBMin; }
	const float* getNavMeshBoundsMax() const { return m_hasBuildSettings ? m_buildSettings.navMeshBMax : m_navMeshBMax; }

	const float* getOriginalNavMeshBoundsMin() const { return m_hasBuildSettings ? m_buildSettings.origNavMeshBMin : m_meshBMin; }
	const float* getOriginalNavMeshBoundsMax() const { return m_hasBuildSettings ? m_buildSettings.origNavMeshBMax : m_meshBMax; }

	const rcChunkyTriMesh* getChunkyMesh() const { return m_chunkyMesh; }
	const BuildSettings* getBuildSettings() const { return m_hasBuildSettings ? &m_buildSettings : 0; }
	bool raycastMesh(const float* src, const float* dst, const unsigned int mask, int* vol = nullptr, float* tmin = nullptr) const;

	/// @name Off-Mesh connections.
	///@{
	int getOffMeshConnectionCount() const { return m_offMeshConCount; }
	const float* getOffMeshConnectionVerts() const { return m_offMeshConVerts; }
	const float* getOffMeshConnectionRads() const { return m_offMeshConRads; }
	const unsigned char* getOffMeshConnectionDirs() const { return m_offMeshConDirs; }
	const unsigned char* getOffMeshConnectionJumps() const { return m_offMeshConJumps; }
	const unsigned char* getOffMeshConnectionOrders() const { return m_offMeshConOrders; }
	const unsigned char* getOffMeshConnectionAreas() const { return m_offMeshConAreas; }
	const unsigned short* getOffMeshConnectionFlags() const { return m_offMeshConFlags; }
	const unsigned short* getOffMeshConnectionId() const { return m_offMeshConId; }
	const float* getOffMeshConnectionRefPos() const { return m_offMeshConRefPos; }
	const float* getOffMeshConnectionRefYaws() const { return m_offMeshConRefYaws; }
	void addOffMeshConnection(const float* spos, const float* epos, const float rad,
							  unsigned char bidir, unsigned char jump, unsigned char order, 
							  unsigned char area, unsigned short flags);
	void deleteOffMeshConnection(int i);
	void drawOffMeshConnections(struct duDebugDraw* dd, const float* offset, bool hilight = false);
	///@}

	/// @name Shape Volumes.
	///@{
	int getConvexVolumeCount() const { return m_volumeCount; } // todo(amos): rename to 'getShapeVolumeCount'
	ShapeVolume* getConvexVolumes() { return m_volumes; } // todo(amos): rename to 'getShapeVolumes'
	int addBoxVolume(const float* bmin, const float* bmax,
						 unsigned short flags, unsigned char area);
	int addCylinderVolume(const float* pos, const float radius,
						 const float height, unsigned short flags, unsigned char area);
	int addConvexVolume(const float* verts, const int nverts,
						 const float minh, const float maxh, unsigned short flags, unsigned char area);
	void deleteConvexVolume(int i); // todo(amos): rename to 'deleteShapeVolumes'
	void drawBoxVolumes(struct duDebugDraw* dd, const float* offset, const int hilightIdx = -1);
	void drawCylinderVolumes(struct duDebugDraw* dd, const float* offset, const int hilightIdx = -1);
	void drawConvexVolumes(struct duDebugDraw* dd, const float* offset, const int hilightIdx = -1);
	///@}
	
private:
	// Explicitly disabled copy constructor and copy assignment operator.
	InputGeom(const InputGeom&);
	InputGeom& operator=(const InputGeom&);
};

#endif // INPUTGEOM_H
