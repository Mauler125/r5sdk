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

enum DrawDetourMeshFlags
{
	DU_DRAW_DETOURMESH_OFFMESHCONS          = 1<<0,  // Render off-mesh connections.
	DU_DRAW_DETOURMESH_QUERY_NODES          = 1<<1,  // Render navmesh query nodes.
	DU_DRAW_DETOURMESH_BVTREE               = 1<<2,  // Render BVTree.
	DU_DRAW_DETOURMESH_PORTALS              = 1<<3,  // Render portals.
	DU_DRAW_DETOURMESH_WITH_CLOSED_LIST     = 1<<4,  // Render navmesh with closed list.
	DU_DRAW_DETOURMESH_TILE_COLORS          = 1<<5,  // Render tiles colored by their ID's.
	DU_DRAW_DETOURMESH_TILE_BOUNDS          = 1<<6,  // Render tile boundaries.
	DU_DRAW_DETOURMESH_TILE_CELLS           = 1<<7,  // Render tile cells.
	DU_DRAW_DETOURMESH_POLY_FACES           = 1<<8,  // Render poly faces.
	DU_DRAW_DETOURMESH_POLY_EDGES           = 1<<9,  // Render poly edges.
	DU_DRAW_DETOURMESH_POLY_VERTS           = 1<<10, // Render poly verts.
	DU_DRAW_DETOURMESH_POLY_BOUNDS_INNER    = 1<<11, // Render inner poly boundaries.
	DU_DRAW_DETOURMESH_POLY_BOUNDS_OUTER    = 1<<12, // Render outer poly boundaries.
	DU_DRAW_DETOURMESH_POLY_CENTERS         = 1<<13, // Render poly centers.
	DU_DRAW_DETOURMESH_POLY_GROUPS          = 1<<14, // Render poly group by color.
	DU_DRAW_DETOURMESH_LEDGE_SPANS          = 1<<15, // Render ledge spans.
	DU_DRAW_DETOURMESH_DEPTH_MASK           = 1<<16, // Use depth mask.
	DU_DRAW_DETOURMESH_ALPHA                = 1<<17, // Use transparency.
	DU_DRAW_DETOURMESH_TRAVERSE_LINKS       = 1<<18, // Render traverse links.
	DU_DRAW_DETOURMESH_TILE_CACHE_BOUNDS    = 1<<19,
	DU_DRAW_DETOURMESH_TILE_CACHE_OBSTACLES = 1<<20,
};

struct duDrawTraverseLinkParams
{
	duDrawTraverseLinkParams() :
		traverseLinkType(-1),
		traverseLinkDistance(-1),
		traverseAnimType(-2),
		cellHeight(0.0f),
		extraOffset(0.0f),
		dynamicOffset(false)
	{}

	int traverseLinkType;
	int traverseLinkDistance;

	// -2 means all, -1 means disjoint poly groups only, anything above
	// refers to an actual anim type and indexes into the traverse tables.
	int traverseAnimType;

	// Used to determine the max LOS angle, this information is lost after
	// the mesh tile has been build so we have to cache it from the editor.
	float cellHeight;

	float extraOffset;
	bool dynamicOffset;
};

void duDebugDrawMeshTile(struct duDebugDraw* dd, const dtNavMesh& mesh, const dtNavMeshQuery* query, const dtMeshTile* tile, const float* offset, unsigned int flags, const duDrawTraverseLinkParams& traverseLinkParams);
void duDebugDrawNavMesh(struct duDebugDraw* dd, const dtNavMesh& mesh, const float* offset, unsigned int flags, const duDrawTraverseLinkParams& traverseLinkParams);
void duDebugDrawNavMeshWithClosedList(struct duDebugDraw* dd, const dtNavMesh& mesh, const dtNavMeshQuery& query, const float* offset, unsigned int flags, const duDrawTraverseLinkParams& traverseLinkParams);
void duDebugDrawNavMeshNodes(struct duDebugDraw* dd, const dtNavMeshQuery& query, const float* offset);
void duDebugDrawNavMeshBVTree(struct duDebugDraw* dd, const dtNavMesh& mesh, const float* offset);
void duDebugDrawNavMeshPortals(struct duDebugDraw* dd, const dtNavMesh& mesh, const float* offset);
void duDebugDrawNavMeshPolysWithFlags(struct duDebugDraw* dd, const dtNavMesh& mesh, const unsigned short polyFlags, const float* offset,
									  const unsigned int drawFlags, const unsigned int col, const bool soften = true);
void duDebugDrawNavMeshPoly(struct duDebugDraw* dd, const dtNavMesh& mesh, dtPolyRef ref, const float* offset,
									  const unsigned int drawFlags, const unsigned int col, const bool soften = true);

void duDebugDrawTileCacheLayerAreas(struct duDebugDraw* dd, const dtTileCacheLayer& layer, const float cs, const float ch, const float* offset);
void duDebugDrawTileCacheLayerRegions(struct duDebugDraw* dd, const dtTileCacheLayer& layer, const float cs, const float ch, const float* offset);
void duDebugDrawTileCacheContours(struct duDebugDraw* dd, const struct dtTileCacheContourSet& lcset,
								  const float* orig, const float cs, const float ch, const float* offset);
void duDebugDrawTileCachePolyMesh(struct duDebugDraw* dd, const struct dtTileCachePolyMesh& lmesh,
								  const float* orig, const float cs, const float ch, const float* offset);

#endif // DETOURDEBUGDRAW_H
