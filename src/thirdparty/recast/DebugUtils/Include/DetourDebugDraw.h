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

#ifndef DETOURDEBUGDRAW_H
#define DETOURDEBUGDRAW_H

#include "Detour/Include/DetourNavMesh.h"
#include "Detour/Include/DetourNavMeshQuery.h"
#include "DetourTileCache/Include/DetourTileCacheBuilder.h"

enum DrawNavMeshFlags
{
	DU_DRAWNAVMESH_OFFMESHCONS = 1 << 0,  // Render off-mesh connections.
	DU_DRAWNAVMESH_NODES       = 1 << 1,  // Render navmesh query nodes.
	DU_DRAWNAVMESH_BVTREE      = 1 << 2,  // Render BVTree.
	DU_DRAWNAVMESH_PORTALS     = 1 << 3,  // Render portals.
	DU_DRAWNAVMESH_CLOSEDLIST  = 1 << 4,  // Render navmesh with closed list.
	DU_DRAWNAVMESH_COLOR_TILES = 1 << 5,  // Render tiles colored by their ID's.
	DU_DRAWNAVMESH_CELLS       = 1 << 6,  // Render tile cells.
	DU_DRAWNAVMESH_VERTS       = 1 << 7,  // Render vertex points.
	DU_DRAWNAVMESH_INNERBOUND  = 1 << 8,  // Render inner poly boundaries.
	DU_DRAWNAVMESH_OUTERBOUND  = 1 << 9,  // Render outer poly boundaries.
	DU_DRAWNAVMESH_POLYCENTERS = 1 << 10, // Render poly centers.
	DU_DRAWNAVMESH_POLYGROUPS  = 1 << 11, // Render poly group by color.
	DU_DRAWNAVMESH_DEPTH_MASK  = 1 << 12, // Use depth mask.
	DU_DRAWNAVMESH_ALPHA       = 1 << 13, // Use transparency.
	DU_DRAWNAVMESH_TRAVERSE_LINKS = 1 << 14, // Render traverse links.
};

void duDebugDrawNavMesh(struct duDebugDraw* dd, const dtNavMesh& mesh, const float* offset, unsigned int flags, const int linkTypes = -1);
void duDebugDrawNavMeshWithClosedList(struct duDebugDraw* dd, const dtNavMesh& mesh, const dtNavMeshQuery& query, const float* offset, unsigned int flags, const int linkTypes = -1);
void duDebugDrawNavMeshNodes(struct duDebugDraw* dd, const dtNavMeshQuery& query, const float* offset);
void duDebugDrawNavMeshBVTree(struct duDebugDraw* dd, const dtNavMesh& mesh, const float* offset);
void duDebugDrawNavMeshPortals(struct duDebugDraw* dd, const dtNavMesh& mesh, const float* offset);
void duDebugDrawNavMeshPolysWithFlags(struct duDebugDraw* dd, const dtNavMesh& mesh, const unsigned short polyFlags, const float* offset,
									  const unsigned int drawFlags, const unsigned int col, const bool soften = true);
void duDebugDrawNavMeshPoly(struct duDebugDraw* dd, const dtNavMesh& mesh, dtPolyRef ref, const float* offset,
									  const unsigned int drawFlags, const unsigned int col, const bool soften = true);

void duDebugDrawTileCacheLayerAreas(struct duDebugDraw* dd, const dtTileCacheLayer& layer, const float cs, const float ch, const float* offset);
void duDebugDrawTileCacheLayerRegions(struct duDebugDraw* dd, const dtTileCacheLayer& layer, const float cs, const float ch, const float* offset);
void duDebugDrawTileCacheContours(duDebugDraw* dd, const struct dtTileCacheContourSet& lcset,
								  const float* orig, const float cs, const float ch, const float* offset);
void duDebugDrawTileCachePolyMesh(duDebugDraw* dd, const struct dtTileCachePolyMesh& lmesh,
								  const float* orig, const float cs, const float ch, const float* offset);

#endif // DETOURDEBUGDRAW_H
