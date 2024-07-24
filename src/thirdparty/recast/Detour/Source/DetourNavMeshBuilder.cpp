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

#include "Shared/Include/SharedMath.h"
#include "Shared/Include/SharedCommon.h"
#include "Shared/Include/SharedAlloc.h"
#include "Shared/Include/SharedAssert.h"
#include "Detour/Include/DetourNavMesh.h"
#include "Detour/Include/DetourNavMeshBuilder.h"

static unsigned short MESH_NULL_IDX = 0xffff;


struct BVItem
{
	unsigned short bmin[3];
	unsigned short bmax[3];
	int i;
};

static int compareItemX(const void* va, const void* vb)
{
	const BVItem* a = (const BVItem*)va;
	const BVItem* b = (const BVItem*)vb;
	if (a->bmin[0] < b->bmin[0])
		return -1;
	if (a->bmin[0] > b->bmin[0])
		return 1;
	return 0;
}

static int compareItemY(const void* va, const void* vb)
{
	const BVItem* a = (const BVItem*)va;
	const BVItem* b = (const BVItem*)vb;
	if (a->bmin[1] < b->bmin[1])
		return -1;
	if (a->bmin[1] > b->bmin[1])
		return 1;
	return 0;
}

static int compareItemZ(const void* va, const void* vb)
{
	const BVItem* a = (const BVItem*)va;
	const BVItem* b = (const BVItem*)vb;
	if (a->bmin[2] < b->bmin[2])
		return -1;
	if (a->bmin[2] > b->bmin[2])
		return 1;
	return 0;
}

static void calcExtends(BVItem* items, const int /*nitems*/, const int imin, const int imax,
						unsigned short* bmin, unsigned short* bmax)
{
	bmin[0] = items[imin].bmin[0];
	bmin[1] = items[imin].bmin[1];
	bmin[2] = items[imin].bmin[2];
	
	bmax[0] = items[imin].bmax[0];
	bmax[1] = items[imin].bmax[1];
	bmax[2] = items[imin].bmax[2];
	
	for (int i = imin+1; i < imax; ++i)
	{
		const BVItem& it = items[i];
		if (it.bmin[0] < bmin[0]) bmin[0] = it.bmin[0];
		if (it.bmin[1] < bmin[1]) bmin[1] = it.bmin[1];
		if (it.bmin[2] < bmin[2]) bmin[2] = it.bmin[2];
		
		if (it.bmax[0] > bmax[0]) bmax[0] = it.bmax[0];
		if (it.bmax[1] > bmax[1]) bmax[1] = it.bmax[1];
		if (it.bmax[2] > bmax[2]) bmax[2] = it.bmax[2];
	}
}

inline int longestAxis(unsigned short x, unsigned short y, unsigned short z)
{
	int	axis = 0;
	unsigned short maxVal = x;
	if (y > maxVal)
	{
		axis = 1;
		maxVal = y;
	}
	if (z > maxVal)
	{
		axis = 2;
	}
	return axis;
}

static void subdivide(BVItem* items, int nitems, int imin, int imax, int& curNode, dtBVNode* nodes)
{
	int inum = imax - imin;
	int icur = curNode;
	
	dtBVNode& node = nodes[curNode++];
	
	if (inum == 1)
	{
		// Leaf
		node.bmin[0] = items[imin].bmin[0];
		node.bmin[1] = items[imin].bmin[1];
		node.bmin[2] = items[imin].bmin[2];
		
		node.bmax[0] = items[imin].bmax[0];
		node.bmax[1] = items[imin].bmax[1];
		node.bmax[2] = items[imin].bmax[2];
		
		node.i = items[imin].i;
	}
	else
	{
		// Split
		calcExtends(items, nitems, imin, imax, node.bmin, node.bmax);
		
		int	axis = longestAxis(node.bmax[0] - node.bmin[0],
							   node.bmax[1] - node.bmin[1],
							   node.bmax[2] - node.bmin[2]);
		
		if (axis == 0)
		{
			// Sort along x-axis
			qsort(items+imin, inum, sizeof(BVItem), compareItemX);
		}
		else if (axis == 1)
		{
			// Sort along y-axis
			qsort(items+imin, inum, sizeof(BVItem), compareItemY);
		}
		else
		{
			// Sort along z-axis
			qsort(items+imin, inum, sizeof(BVItem), compareItemZ);
		}
		
		int isplit = imin+inum/2;
		
		// Left
		subdivide(items, nitems, imin, isplit, curNode, nodes);
		// Right
		subdivide(items, nitems, isplit, imax, curNode, nodes);
		
		int iescape = curNode - icur;
		// Negative index means escape.
		node.i = -iescape;
	}
}

static int createBVTree(dtNavMeshCreateParams* params, dtBVNode* nodes, int /*nnodes*/)
{
	// Build tree
	float quantFactor = 1 / params->cs;
	BVItem* items = (BVItem*)rdAlloc(sizeof(BVItem)*params->polyCount, RD_ALLOC_TEMP);
	for (int i = 0; i < params->polyCount; i++)
	{
		BVItem& it = items[i];
		it.i = i;
		// Calc polygon bounds. Use detail meshes if available.
		if (params->detailMeshes)
		{
			int vb = (int)params->detailMeshes[i*4+0];
			int ndv = (int)params->detailMeshes[i*4+1];
			float bmin[3];
			float bmax[3];

			const float* dv = &params->detailVerts[vb*3];
			rdVcopy(bmin, dv);
			rdVcopy(bmax, dv);

			for (int j = 1; j < ndv; j++)
			{
				rdVmin(bmin, &dv[j * 3]);
				rdVmax(bmax, &dv[j * 3]);
			}

			// BV-tree uses cs for all dimensions
			it.bmin[0] = (unsigned short)rdClamp((int)((bmin[0] - params->bmin[0])*quantFactor), 0, 0xffff);
			it.bmin[1] = (unsigned short)rdClamp((int)((bmin[1] - params->bmin[1])*quantFactor), 0, 0xffff);
			it.bmin[2] = (unsigned short)rdClamp((int)((bmin[2] - params->bmin[2])*quantFactor), 0, 0xffff);

			it.bmax[0] = (unsigned short)rdClamp((int)((bmax[0] - params->bmin[0])*quantFactor), 0, 0xffff);
			it.bmax[1] = (unsigned short)rdClamp((int)((bmax[1] - params->bmin[1])*quantFactor), 0, 0xffff);
			it.bmax[2] = (unsigned short)rdClamp((int)((bmax[2] - params->bmin[2])*quantFactor), 0, 0xffff);
		}
		else
		{
			const unsigned short* p = &params->polys[i*params->nvp * 2];
			it.bmin[0] = it.bmax[0] = params->verts[p[0] * 3 + 0];
			it.bmin[1] = it.bmax[1] = params->verts[p[0] * 3 + 1];
			it.bmin[2] = it.bmax[2] = params->verts[p[0] * 3 + 2];

			for (int j = 1; j < params->nvp; ++j)
			{
				if (p[j] == MESH_NULL_IDX) break;
				unsigned short x = params->verts[p[j] * 3 + 0];
				unsigned short y = params->verts[p[j] * 3 + 1];
				unsigned short z = params->verts[p[j] * 3 + 2];

				if (x < it.bmin[0]) it.bmin[0] = x;
				if (y < it.bmin[1]) it.bmin[1] = y;
				if (z < it.bmin[2]) it.bmin[2] = z;

				if (x > it.bmax[0]) it.bmax[0] = x;
				if (y > it.bmax[1]) it.bmax[1] = y;
				if (z > it.bmax[2]) it.bmax[2] = z;
			}
			// Remap z
			it.bmin[2] = (unsigned short)rdMathFloorf((float)it.bmin[2] * params->ch / params->cs);
			it.bmax[2] = (unsigned short)rdMathCeilf((float)it.bmax[2] * params->ch / params->cs);
		}
	}
	
	int curNode = 0;
	subdivide(items, params->polyCount, 0, params->polyCount, curNode, nodes);
	
	rdFree(items);
	
	return curNode;
}

static unsigned char classifyOffMeshPoint(const float* pt, const float* bmin, const float* bmax)
{
	static const unsigned char XP = 1<<0;
	static const unsigned char ZP = 1<<1;
	static const unsigned char XM = 1<<2;
	static const unsigned char ZM = 1<<3;	

	unsigned char outcode = 0; 
	outcode |= (pt[0] >= bmax[0]) ? XP : 0;
	outcode |= (pt[1] >= bmax[1]) ? ZP : 0;
	outcode |= (pt[0] < bmin[0])  ? XM : 0;
	outcode |= (pt[1] < bmin[1])  ? ZM : 0;

	switch (outcode)
	{
	case XP: return 0;
	case XP|ZP: return 1;
	case ZP: return 2;
	case XM|ZP: return 3;
	case XM: return 4;
	case XM|ZM: return 5;
	case ZM: return 6;
	case XP|ZM: return 7;
	};

	return 0xff;	
}

static void setPolyGroupsTraversalReachability(int* const tableData, const int numPolyGroups,
	const unsigned short polyGroup1, const unsigned short polyGroup2, const bool isReachable)
{
	const int index = dtCalcTraversalTableCellIndex(numPolyGroups, polyGroup1, polyGroup2);
	const int value = 1<<(polyGroup2 & 31);

	if (isReachable)
		tableData[index] |= value;
	else
		tableData[index] &= ~value;
}

bool dtCreateDisjointPolyGroups(dtNavMesh* nav, dtDisjointSet& disjoint)
{
	rdAssert(nav);

	// Reserve the first poly groups
	// 0 = DT_NULL_POLY_GROUP.
	// 1 = DT_STRAY_POLY_GROUP.
	disjoint.init(DT_FIRST_USABLE_POLY_GROUP);

	// Clear all labels.
	for (int i = 0; i < nav->getMaxTiles(); ++i)
	{
		dtMeshTile* tile = nav->getTile(i);
		if (!tile || !tile->header || !tile->dataSize) continue;
		int pcount = tile->header->polyCount;
		for (int j = 0; j < pcount; j++)
		{
			dtPoly& poly = tile->polys[j];
			poly.groupId = DT_NULL_POLY_GROUP;

			// NOTE: these fields are unknown and need to be reversed.
			// It is possible these are used internally only.
			poly.unk1 = (unsigned short)-1;
			poly.unk2 = (unsigned short)-1;
		}
	}

	// First pass to group linked and unlinked poly islands.
	std::set<unsigned short> linkedGroups;
	for (int i = 0; i < nav->getMaxTiles(); ++i)
	{
		dtMeshTile* tile = nav->getTile(i);
		if (!tile || !tile->header || !tile->dataSize) continue;
		const int pcount = tile->header->polyCount;
		for (int j = 0; j < pcount; j++)
		{
			dtPoly& poly = tile->polys[j];
			unsigned int plink = poly.firstLink;

			// Off-mesh connections need their own ID's, skip the assignment
			// here since else we will be marking 2 (or more) poly islands 
			// under the same group id.
			// NOTE: when we implement jump links, we will have to check on
			// these here as well! They also shouldn't merge 2 islands together.
			// Ultimately, the jump links should only be used during traversal
			// table building to mark linked islands as reachable.
			if (poly.getType() != DT_POLYTYPE_OFFMESH_CONNECTION)
			{
				while (plink != DT_NULL_LINK)
				{
					const dtLink l = tile->links[plink];
					const dtMeshTile* t;
					const dtPoly* p;
					nav->getTileAndPolyByRefUnsafe(l.ref, &t, &p);

					if (p->groupId != DT_NULL_POLY_GROUP)
						linkedGroups.insert(p->groupId);

					plink = l.next;
				}
			}

			const bool noLinkedGroups = linkedGroups.empty();

			if (noLinkedGroups)
			{
				// This poly isn't connected to anything, mark it so the game
				// won't consider this poly in path generation.
				if (poly.firstLink == DT_NULL_LINK)
					poly.groupId = DT_STRAY_POLY_GROUP;
				else
					poly.groupId = (unsigned short)disjoint.insertNew();
			}
			else
			{
				const unsigned short rootGroup = *linkedGroups.begin();
				poly.groupId = rootGroup;

				for (const int linkedGroup : linkedGroups)
					disjoint.setUnion(rootGroup, linkedGroup);
			}

			if (!noLinkedGroups)
				linkedGroups.clear();
		}
	}

	// Second pass to ensure all poly's have their root disjoint set ID.
	for (int i = 0; i < nav->getMaxTiles(); ++i)
	{
		dtMeshTile* tile = nav->getTile(i);
		if (!tile || !tile->header || !tile->dataSize) continue;
		const int pcount = tile->header->polyCount;
		for (int j = 0; j < pcount; j++)
		{
			dtPoly& poly = tile->polys[j];
			if (poly.groupId != DT_STRAY_POLY_GROUP)
			{
				int id = disjoint.find(poly.groupId);
				poly.groupId = (unsigned short)id;
			}
		}
	}

	// Gather all unique polygroups and map them to a contiguous range.
	std::map<unsigned short, unsigned short> groupMap;
	disjoint.init(DT_FIRST_USABLE_POLY_GROUP);

	for (int i = 0; i < nav->getMaxTiles(); ++i)
	{
		dtMeshTile* tile = nav->getTile(i);
		if (!tile || !tile->header || !tile->dataSize) continue;
		const int pcount = tile->header->polyCount;
		for (int j = 0; j < pcount; j++)
		{
			dtPoly& poly = tile->polys[j];
			unsigned short oldId = poly.groupId;
			if (oldId != DT_STRAY_POLY_GROUP && groupMap.find(oldId) == groupMap.end())
				groupMap[oldId] = (unsigned short)disjoint.insertNew();
		}
	}

	// Fourth pass to apply the new mapping to all polys.
	for (int i = 0; i < nav->getMaxTiles(); ++i)
	{
		dtMeshTile* tile = nav->getTile(i);
		if (!tile || !tile->header || !tile->dataSize) continue;
		const int pcount = tile->header->polyCount;
		for (int j = 0; j < pcount; j++)
		{
			dtPoly& poly = tile->polys[j];
			if (poly.groupId != DT_STRAY_POLY_GROUP)
				poly.groupId = groupMap[poly.groupId];
		}
	}

	// Third pass to handle off-mesh connections.
	// note(amos): this has to happen after the first and second pass as these
	// are for grouping directly connected polygons together, else groups linked
	// through off-mesh connections will be merged into a single group!
	// 
	// todo(amos): should off-mesh links be marked reachable for all traverse
	// anim types? Research needed on Titanfall 2. For now, mark connected
	// poly groups with off-mesh connections reachable.
	for (int i = 0; i < nav->getMaxTiles(); ++i)
	{
		dtMeshTile* tile = nav->getTile(i);
		if (!tile || !tile->header || !tile->dataSize) continue;
		const int pcount = tile->header->polyCount;
		for (int j = 0; j < pcount; j++)
		{
			dtPoly& poly = tile->polys[j];

			if (poly.getType() != DT_POLYTYPE_OFFMESH_CONNECTION)
				continue;

			unsigned int plink = poly.firstLink;
			unsigned short firstGroupId = DT_NULL_POLY_GROUP;

			while (plink != DT_NULL_LINK)
			{
				const dtLink l = tile->links[plink];
				const dtMeshTile* t;
				const dtPoly* p;
				nav->getTileAndPolyByRefUnsafe(l.ref, &t, &p);

				if (p->groupId != DT_NULL_POLY_GROUP)
				{
					if (firstGroupId == DT_NULL_POLY_GROUP)
						firstGroupId = p->groupId;
					else
						disjoint.setUnion(firstGroupId, p->groupId);
				}

				plink = l.next;
			}
		}
	}

	nav->setPolyGroupcount(disjoint.getSetCount());
	return true;
}

// todo(amos): remove param 'tableCount' and make struct 'dtTraversalTableCreateParams'
bool dtCreateTraversalTableData(dtNavMesh* nav, const dtDisjointSet& disjoint, const int tableCount)
{
	const int polyGroupCount = nav->getPolyGroupCount();
	const int tableSize = dtCalcTraversalTableSize(polyGroupCount);

	// TODO: currently we allocate 5 buffers and just copy the same traversal
	// tables in, this works fine since we don't generate jump links and
	// therefore all poly islands should be marked unreachable from each other.
	// But when we generate jump links, we need to take into consideration that
	// the '_small' navmesh supports 5 animation types (dictated by the field
	// "TraverseAnimType" in the NPC's settings file, and each of them have
	// different properties in regards to how far they can jump, or the angle,
	// ect... For example the "frag_drone" anim type can take far further jumps
	// than the "human" one. The human indexes into the first table while 
	// frag_drone indexes into the fourth. We have to set different links per
	// table. The 'dtLink::jumpType' field probably determines what belongs to
	// what TraverseAnimType, which we could use to set the traversability. 
	// More reasearch is needed for the jump links and flags... For other
	// navmeshes, e.g. the '_large' one, they all contain only 1 traversal
	// table as they only support one TraverseAnimType each. but also here
	// we have to "reverse" the properties from existing Titanfall 2 single
	// player navmeshes and determine the traversability in this loop below.
	for (int i = 0; i < tableCount; i++)
	{
		int* const traversalTable = (int*)rdAlloc(sizeof(int)*tableSize, RD_ALLOC_PERM);

		if (!traversalTable)
			return false;

		nav->setTraverseTable(i, traversalTable);
		memset(traversalTable, 0, sizeof(int)*tableSize);

		for (unsigned short j = 0; j < polyGroupCount; j++)
		{
			for (unsigned short k = 0; k < polyGroupCount; k++)
			{
				// Only reachable if its the same polygroup or if they are linked!
				const bool isReachable = j == k || disjoint.find(j) == disjoint.find(k);
				setPolyGroupsTraversalReachability(traversalTable, polyGroupCount, j, k, isReachable);
			}
		}
	}

	nav->setTraverseTableSize(tableSize);

	return true;
}

static const unsigned short DT_MESH_NULL_IDX = 0xffff;
static int countPolyVerts(const unsigned short* p, const int nvp) // todo(amos): deduplicate
{
	for (int i = 0; i < nvp; ++i)
		if (p[i] == DT_MESH_NULL_IDX)
			return i;
	return nvp;
}

struct CellItem
{
	float pos[3];
	int polyIndex;
};

bool createPolyMeshCells(const dtNavMeshCreateParams* params, rdTempVector<CellItem>& cellItems)
{
	const int nvp = params->nvp;
	const int resolution = params->cellResolution;
	const float stepX = (params->bmax[0]-params->bmin[0]) / resolution;
	const float stepY = (params->bmax[1]-params->bmin[1]) / resolution;

	for (int i = 0; i < params->polyCount; ++i)
	{
		const unsigned short* p = &params->polys[i*2*nvp];
		const int nv = countPolyVerts(p, nvp);

		if (nv < 3) // Don't generate cells for off-mesh connections.
			continue;

		const unsigned int vb = params->detailMeshes[i*4+0];
		const unsigned int ndv = params->detailMeshes[i*4+1];
		const unsigned int tb = params->detailMeshes[i*4+2];

		float polyVerts[DT_VERTS_PER_POLYGON*3];

		for (int j = 0; j < nv; ++j)
		{
			const unsigned short* polyVert = &params->verts[p[j]*3];
			float* flPolyVert = &polyVerts[j*3];

			flPolyVert[0] = params->bmin[0]+polyVert[0]*params->cs;
			flPolyVert[1] = params->bmin[1]+polyVert[1]*params->cs;
			flPolyVert[2] = params->bmin[2]+polyVert[2]*params->ch;
		}

		for (int j = 0; j <= resolution; j++)
		{
			for (int k = 0; k <= resolution; k++)
			{
				const float offsetX = (k % 2 == 0) ? 0.0f : stepX / 2.0f;

				float targetCellPos[3];
				targetCellPos[0] = params->bmin[0]+j*stepX+offsetX;
				targetCellPos[1] = params->bmin[1]+k*stepY;
				targetCellPos[2] = 0; // todo(amos): might need a proper fallback, but so far this never failed.

				if (!rdPointInPolygon(targetCellPos, polyVerts, nv))
					continue;

				for (int l = 0; l < params->detailTriCount; ++l)
				{
					const unsigned char* t = &params->detailTris[(tb+l)*4];
					float storage[3][3];
					const float* v[3];

					for (int m = 0; m < 3; ++m)
					{
						if (t[m] < nv)
						{
							for (int n = 0; n < 3; ++n)
							{
								storage[m][n] = params->bmin[n] + params->verts[p[t[m]]*3+n] * (n == 2 ? params->ch : params->cs);
							}
							v[m] = storage[m];
						}
						else
						{
							v[m] = &params->detailVerts[(vb+t[m])*3];
						}
					}

					if (rdClosestHeightPointTriangle(targetCellPos, v[0],v[1],v[2], targetCellPos[2]))
						break;
				}

				cellItems.push_back({ targetCellPos[0],targetCellPos[1],targetCellPos[2], i });
			}
		}
	}

	return true;
}

// TODO: Better error handling.

/// @par
/// 
/// The output data array is allocated using the detour allocator (rdAlloc()).  The method
/// used to free the memory will be determined by how the tile is added to the navigation
/// mesh.
///
/// @see dtNavMesh, dtNavMesh::addTile()
bool dtCreateNavMeshData(dtNavMeshCreateParams* params, unsigned char** outData, int* outDataSize)
{
	if (params->nvp > DT_VERTS_PER_POLYGON)
		return false;
	if (params->vertCount >= 0xffff)
		return false;
	if (!params->vertCount || !params->verts)
		return false;
	if (!params->polyCount || !params->polys)
		return false;

	const int nvp = params->nvp;
	
	// Classify off-mesh connection points. We store only the connections
	// whose start point is inside the tile.
	unsigned char* offMeshConClass = 0;
	int storedOffMeshConCount = 0;
	int offMeshConLinkCount = 0;
	
	if (params->offMeshConCount > 0)
	{
		offMeshConClass = (unsigned char*)rdAlloc(sizeof(unsigned char)*params->offMeshConCount*2, RD_ALLOC_TEMP);
		if (!offMeshConClass)
			return false;

		// Find tight height bounds, used for culling out off-mesh start locations.
		float hmin = FLT_MAX;
		float hmax = -FLT_MAX;
		
		if (params->detailVerts && params->detailVertsCount)
		{
			for (int i = 0; i < params->detailVertsCount; ++i)
			{
				const float h = params->detailVerts[i*3+2];
				hmin = rdMin(hmin,h);
				hmax = rdMax(hmax,h);
			}
		}
		else
		{
			for (int i = 0; i < params->vertCount; ++i)
			{
				const unsigned short* iv = &params->verts[i*3];
				const float h = params->bmin[2] + iv[2] * params->ch;
				hmin = rdMin(hmin,h);
				hmax = rdMax(hmax,h);
			}
		}
		hmin -= params->walkableClimb;
		hmax += params->walkableClimb;
		float bmin[3], bmax[3];
		rdVcopy(bmin, params->bmin);
		rdVcopy(bmax, params->bmax);
		bmin[2] = hmin;
		bmax[2] = hmax;

		for (int i = 0; i < params->offMeshConCount; ++i)
		{
			const float* p0 = &params->offMeshConVerts[(i*2+0)*3];
			const float* p1 = &params->offMeshConVerts[(i*2+1)*3];
			offMeshConClass[i*2+0] = classifyOffMeshPoint(p0, bmin, bmax);
			offMeshConClass[i*2+1] = classifyOffMeshPoint(p1, bmin, bmax);

			// Zero out off-mesh start positions which are not even potentially touching the mesh.
			if (offMeshConClass[i*2+0] == 0xff)
			{
				if (p0[2] < bmin[2] || p0[2] > bmax[2])
					offMeshConClass[i*2+0] = 0;
			}

			// Count how many links should be allocated for off-mesh connections.
			if (offMeshConClass[i*2+0] == 0xff)
				offMeshConLinkCount++;
			if (offMeshConClass[i*2+1] == 0xff)
				offMeshConLinkCount++;

			if (offMeshConClass[i*2+0] == 0xff)
				storedOffMeshConCount++;
		}
	}
	
	// Off-mesh connections are stored as polygons, adjust values.
	const int totPolyCount = params->polyCount + storedOffMeshConCount;
	const int totVertCount = params->vertCount + storedOffMeshConCount*2;
	
	// Find portal edges which are at tile borders.
	int edgeCount = 0;
	int portalCount = 0;
	for (int i = 0; i < params->polyCount; ++i)
	{
		const unsigned short* p = &params->polys[i*2*nvp];
		for (int j = 0; j < nvp; ++j)
		{
			if (p[j] == MESH_NULL_IDX) break;
			edgeCount++;
			
			if (p[nvp+j] & 0x8000)
			{
				unsigned short dir = p[nvp+j] & 0xf;
				if (dir != 0xf)
					portalCount++;
			}
		}
	}

	const int maxLinkCount = edgeCount + portalCount*2 + offMeshConLinkCount*2;
	
	// Find unique detail vertices.
	int uniqueDetailVertCount = 0;
	int detailTriCount = 0;
	if (params->detailMeshes)
	{
		// Has detail mesh, count unique detail vertex count and use input detail tri count.
		detailTriCount = params->detailTriCount;
		for (int i = 0; i < params->polyCount; ++i)
		{
			const unsigned short* p = &params->polys[i*nvp*2];
			int ndv = params->detailMeshes[i*4+1];
			int nv = 0;
			for (int j = 0; j < nvp; ++j)
			{
				if (p[j] == MESH_NULL_IDX) break;
				nv++;
			}
			ndv -= nv;
			uniqueDetailVertCount += ndv;
		}
	}
	else
	{
		// No input detail mesh, build detail mesh from nav polys.
		uniqueDetailVertCount = 0; // No extra detail verts.
		detailTriCount = 0;
		for (int i = 0; i < params->polyCount; ++i)
		{
			const unsigned short* p = &params->polys[i*nvp*2];
			int nv = 0;
			for (int j = 0; j < nvp; ++j)
			{
				if (p[j] == MESH_NULL_IDX) break;
				nv++;
			}
			detailTriCount += nv-2;
		}
	}

#if DT_NAVMESH_SET_VERSION >= 8
	rdTempVector<CellItem> cellItems;
	createPolyMeshCells(params, cellItems);
#endif

	// Calculate data size
	const int headerSize = rdAlign4(sizeof(dtMeshHeader));
	const int vertsSize = rdAlign4(sizeof(float)*3*totVertCount);
	const int polysSize = rdAlign4(sizeof(dtPoly)*totPolyCount);
	const int linksSize = rdAlign4(sizeof(dtLink)*maxLinkCount);
	const int detailMeshesSize = rdAlign4(sizeof(dtPolyDetail)*params->polyCount);
	const int detailVertsSize = rdAlign4(sizeof(float)*3*uniqueDetailVertCount);
	const int detailTrisSize = rdAlign4(sizeof(unsigned char)*4*detailTriCount);
	const int bvTreeSize = params->buildBvTree ? rdAlign4(sizeof(dtBVNode)*params->polyCount*2) : 0;
	const int offMeshConsSize = rdAlign4(sizeof(dtOffMeshConnection)*storedOffMeshConCount);
#if DT_NAVMESH_SET_VERSION >= 8
	const int cellsSize = rdAlign4(sizeof(dtCell)*(int)cellItems.size());
#endif
	
	int polyMapCount = 0; // TODO: this data has to be reversed still from the NavMesh!
	const int polyMapSize = polyMapCount * totPolyCount;

	const int dataSize = headerSize + vertsSize + polysSize + polyMapSize + linksSize +
		detailMeshesSize + detailVertsSize + detailTrisSize +
		bvTreeSize + offMeshConsSize
#if DT_NAVMESH_SET_VERSION >= 8
		+ cellsSize;
#else
		;
#endif

	//printf("%i %i %i %i(%i links) %i %i %i %i %i\n", headerSize, vertsSize, polysSize, linksSize, maxLinkCount, detailMeshesSize, detailVertsSize, detailTrisSize, bvTreeSize, offMeshConsSize);
	//printf("%i\n", dataSize);

	unsigned char* data = (unsigned char*)rdAlloc(sizeof(unsigned char)*dataSize, RD_ALLOC_PERM);
	if (!data)
	{
		rdFree(offMeshConClass);
		return false;
	}
	memset(data, 0, dataSize);
	
	unsigned char* d = data;
	
	dtMeshHeader* header = rdGetThenAdvanceBufferPointer<dtMeshHeader>(d, headerSize);
	float* navVerts = rdGetThenAdvanceBufferPointer<float>(d, vertsSize);
	dtPoly* navPolys = rdGetThenAdvanceBufferPointer<dtPoly>(d, polysSize);
	unsigned int* polyMap = rdGetThenAdvanceBufferPointer<unsigned int>(d, polyMapSize);
	d += linksSize; // Ignore links; just leave enough space for them. They'll be created on load.
	//dtLink* links = rdGetThenAdvanceBufferPointer<dtLink>(d, linksSize);
	dtPolyDetail* navDMeshes = rdGetThenAdvanceBufferPointer<dtPolyDetail>(d, detailMeshesSize);
	float* navDVerts = rdGetThenAdvanceBufferPointer<float>(d, detailVertsSize);
	unsigned char* navDTris = rdGetThenAdvanceBufferPointer<unsigned char>(d, detailTrisSize);
	dtBVNode* navBvtree = rdGetThenAdvanceBufferPointer<dtBVNode>(d, bvTreeSize);
	dtOffMeshConnection* offMeshCons = rdGetThenAdvanceBufferPointer<dtOffMeshConnection>(d, offMeshConsSize);
#if DT_NAVMESH_SET_VERSION >= 8
	dtCell* navCells = rdGetThenAdvanceBufferPointer<dtCell>(d, cellsSize);
#endif

	rdIgnoreUnused(polyMap);
	//for(int i=0;i<unkPerPoly*totPolyCount;i++)
	//	unknownArray[i] = rand();

	// Store header
	header->magic = DT_NAVMESH_MAGIC;
	header->version = DT_NAVMESH_VERSION;
	header->x = params->tileX;
	header->y = params->tileY;
	header->layer = params->tileLayer;
	header->userId = params->userId;
	header->polyCount = totPolyCount;
	header->polyMapCount = polyMapCount;
	header->vertCount = totVertCount;
	header->maxLinkCount = maxLinkCount;
	rdVcopy(header->bmin, params->bmin);
	rdVcopy(header->bmax, params->bmax);
	header->detailMeshCount = params->polyCount;
	header->detailVertCount = uniqueDetailVertCount;
	header->detailTriCount = detailTriCount;
	header->bvQuantFactor = 1.0f / params->cs;
	header->offMeshBase = params->polyCount;
#if DT_NAVMESH_SET_VERSION >= 8
	header->maxCellCount = (int)cellItems.size();
#endif
	header->walkableHeight = params->walkableHeight;
	header->walkableRadius = params->walkableRadius;
	header->walkableClimb = params->walkableClimb;
	header->offMeshConCount = storedOffMeshConCount;
	header->bvNodeCount = params->buildBvTree ? params->polyCount*2 : 0;

	const int offMeshVertsBase = params->vertCount;
	const int offMeshPolyBase = params->polyCount;
	
	// Store vertices
	// Mesh vertices
	for (int i = 0; i < params->vertCount; ++i)
	{
		const unsigned short* iv = &params->verts[i*3];
		float* v = &navVerts[i*3];
		v[0] = params->bmin[0] + iv[0] * params->cs;
		v[1] = params->bmin[1] + iv[1] * params->cs;
		v[2] = params->bmin[2] + iv[2] * params->ch;
	}
	// Off-mesh link vertices.
	int n = 0;
	for (int i = 0; i < params->offMeshConCount; ++i)
	{
		// Only store connections which start from this tile.
		if (offMeshConClass[i*2+0] == 0xff)
		{
			const float* linkv = &params->offMeshConVerts[i*2*3];
			float* v = &navVerts[(offMeshVertsBase + n*2)*3];
			rdVcopy(&v[0], &linkv[0]);
			rdVcopy(&v[3], &linkv[3]);
			n++;
		}
	}

#if DT_NAVMESH_SET_VERSION >= 8
	// Polygon cells.
	for (int i = 0; i < (int)cellItems.size(); i++)
	{
		const CellItem& cellItem = cellItems[i];
		dtCell& cell = navCells[i];

		rdVcopy(cell.pos, cellItem.pos);
		cell.polyIndex = cellItem.polyIndex;
	}
#endif
	
	// Store polygons
	// Mesh polys
	const unsigned short* src = params->polys;
	for (int i = 0; i < params->polyCount; ++i)
	{
		dtPoly* p = &navPolys[i];
		p->vertCount = 0;
		p->flags = params->polyFlags[i];
		p->setArea(params->polyAreas[i]);
		p->setType(DT_POLYTYPE_GROUND);
		for (int j = 0; j < nvp; ++j)
		{
			if (src[j] == MESH_NULL_IDX) break;
			p->verts[j] = src[j];
			if (src[nvp+j] & 0x8000)
			{
				// Border or portal edge.
				unsigned short dir = src[nvp+j] & 0xf;
				if (dir == 0xf) // Border
					p->neis[j] = 0;
				else if (dir == 0) // Portal x-
					p->neis[j] = DT_EXT_LINK | 4;
				else if (dir == 1) // Portal y+
					p->neis[j] = DT_EXT_LINK | 2;
				else if (dir == 2) // Portal x+
					p->neis[j] = DT_EXT_LINK | 0;
				else if (dir == 3) // Portal y-
					p->neis[j] = DT_EXT_LINK | 6;
			}
			else
			{
				// Normal connection
				p->neis[j] = src[nvp+j]+1;
			}
			rdVadd(p->center, p->center, &navVerts[p->verts[j] * 3]);
			p->vertCount++;
		}
		rdVscale(p->center, p->center, 1 / (float)(p->vertCount));
		p->surfaceArea = (unsigned short)rdMathFloorf(dtCalcPolySurfaceArea(p,navVerts) / DT_POLY_AREA_QUANT_FACTOR);

		src += nvp*2;
	}

	// Off-mesh connection vertices.
	n = 0;
	for (int i = 0; i < params->offMeshConCount; ++i)
	{
		// Only store connections which start from this tile.
		if (offMeshConClass[i*2+0] == 0xff)
		{
			dtPoly* p = &navPolys[offMeshPolyBase+n];
			p->vertCount = 2;
			p->verts[0] = (unsigned short)(offMeshVertsBase + n*2+0);
			p->verts[1] = (unsigned short)(offMeshVertsBase + n*2+1);
			p->flags = params->offMeshConFlags[i];
			p->setArea(params->offMeshConAreas[i]);
			p->setType(DT_POLYTYPE_OFFMESH_CONNECTION);
			n++;
		}
	}

	// Store detail meshes and vertices.
	// The nav polygon vertices are stored as the first vertices on each mesh.
	// We compress the mesh data by skipping them and using the navmesh coordinates.
	if (params->detailMeshes)
	{
		unsigned short vbase = 0;
		for (int i = 0; i < params->polyCount; ++i)
		{
			dtPolyDetail& dtl = navDMeshes[i];
			const int vb = (int)params->detailMeshes[i*4+0];
			const int ndv = (int)params->detailMeshes[i*4+1];
			const int nv = navPolys[i].vertCount;
			dtl.vertBase = (unsigned int)vbase;
			dtl.vertCount = (unsigned char)(ndv-nv);
			dtl.triBase = (unsigned int)params->detailMeshes[i*4+2];
			dtl.triCount = (unsigned char)params->detailMeshes[i*4+3];
			// Copy vertices except the first 'nv' verts which are equal to nav poly verts.
			if (ndv-nv)
			{
				memcpy(&navDVerts[vbase*3], &params->detailVerts[(vb+nv)*3], sizeof(float)*3*(ndv-nv));
				vbase += (unsigned short)(ndv-nv);
			}
		}
		// Store triangles.
		memcpy(navDTris, params->detailTris, sizeof(unsigned char)*4*params->detailTriCount);
	}
	else
	{
		// Create dummy detail mesh by triangulating polys.
		int tbase = 0;
		for (int i = 0; i < params->polyCount; ++i)
		{
			dtPolyDetail& dtl = navDMeshes[i];
			const int nv = navPolys[i].vertCount;
			dtl.vertBase = 0;
			dtl.vertCount = 0;
			dtl.triBase = (unsigned int)tbase;
			dtl.triCount = (unsigned char)(nv-2);
			// Triangulate polygon (local indices).
			for (int j = 2; j < nv; ++j)
			{
				unsigned char* t = &navDTris[tbase*4];
				t[0] = 0;
				t[1] = (unsigned char)(j-1);
				t[2] = (unsigned char)(j);
				// Bit for each edge that belongs to poly boundary.
				t[3] = (1<<2);
				if (j == 2) t[3] |= (1<<0);
				if (j == nv-1) t[3] |= (1<<4);
				tbase++;
			}
		}
	}

	// Store and create BVtree.
	if (params->buildBvTree)
	{
		createBVTree(params, navBvtree, 2*params->polyCount);
	}
	
	// Store Off-Mesh connections.
	n = 0;
	for (int i = 0; i < params->offMeshConCount; ++i)
	{
		// Only store connections which start from this tile.
		if (offMeshConClass[i*2+0] == 0xff)
		{
			dtOffMeshConnection* con = &offMeshCons[n];
			con->poly = (unsigned short)(offMeshPolyBase + n);
			// Copy connection end-points.
			const float* endPts = &params->offMeshConVerts[i*2*3];
			const float* refPos = &params->offMeshConRefPos[i*3];
			rdVcopy(&con->pos[0], &endPts[0]);
			rdVcopy(&con->pos[3], &endPts[3]);
			rdVcopy(&con->refPos[0], &refPos[0]);
			con->rad = params->offMeshConRad[i];
			con->refYaw = params->offMeshConRefYaw[i];
			con->flags = params->offMeshConDir[i] ? DT_OFFMESH_CON_BIDIR : 0;
			con->side = offMeshConClass[i*2+1];
#if DT_NAVMESH_SET_VERSION == 5
			con->jumpType = params->offMeshConJumps[i];
			con->unk1 = 1;
#endif
			con->userId = params->offMeshConUserID[i];
#if DT_NAVMESH_SET_VERSION >= 7
			con->hintIdx = DT_NULL_HINT; // todo(amos): hints are currently not supported.
#endif
			n++;
		}
	}
		
	rdFree(offMeshConClass);
	
	*outData = data;
	*outDataSize = dataSize;
	
	return true;
}

bool dtNavMeshHeaderSwapEndian(unsigned char* data, const int /*dataSize*/)
{
	dtMeshHeader* header = (dtMeshHeader*)data;
	
	int swappedMagic = DT_NAVMESH_MAGIC;
	int swappedVersion = DT_NAVMESH_VERSION;
	rdSwapEndian(&swappedMagic);
	rdSwapEndian(&swappedVersion);
	
	if ((header->magic != DT_NAVMESH_MAGIC || header->version != DT_NAVMESH_VERSION) &&
		(header->magic != swappedMagic || header->version != swappedVersion))
	{
		return false;
	}
		
	rdSwapEndian(&header->magic);
	rdSwapEndian(&header->version);
	rdSwapEndian(&header->x);
	rdSwapEndian(&header->y);
	rdSwapEndian(&header->layer);
	rdSwapEndian(&header->userId);
	rdSwapEndian(&header->polyCount);
	rdSwapEndian(&header->vertCount);
	rdSwapEndian(&header->maxLinkCount);
	rdSwapEndian(&header->detailMeshCount);
	rdSwapEndian(&header->detailVertCount);
	rdSwapEndian(&header->detailTriCount);
	rdSwapEndian(&header->bvNodeCount);
	rdSwapEndian(&header->offMeshConCount);
	rdSwapEndian(&header->offMeshBase);
#if DT_NAVMESH_SET_VERSION >= 8
	rdSwapEndian(&header->maxCellCount);
#endif
	rdSwapEndian(&header->walkableHeight);
	rdSwapEndian(&header->walkableRadius);
	rdSwapEndian(&header->walkableClimb);
	rdSwapEndian(&header->bmin[0]);
	rdSwapEndian(&header->bmin[1]);
	rdSwapEndian(&header->bmin[2]);
	rdSwapEndian(&header->bmax[0]);
	rdSwapEndian(&header->bmax[1]);
	rdSwapEndian(&header->bmax[2]);
	rdSwapEndian(&header->bvQuantFactor);

	// Freelist index and pointers are updated when tile is added, no need to swap.

	return true;
}

/// @par
///
/// @warning This function assumes that the header is in the correct endianess already. 
/// Call #dtNavMeshHeaderSwapEndian() first on the data if the data is expected to be in wrong endianess 
/// to start with. Call #dtNavMeshHeaderSwapEndian() after the data has been swapped if converting from 
/// native to foreign endianess.
bool dtNavMeshDataSwapEndian(unsigned char* data, const int /*dataSize*/)
{
	// Make sure the data is in right format.
	dtMeshHeader* header = (dtMeshHeader*)data;
	if (header->magic != DT_NAVMESH_MAGIC)
		return false;
	if (header->version != DT_NAVMESH_VERSION)
		return false;
	
	// Patch header pointers.
	const int headerSize = rdAlign4(sizeof(dtMeshHeader));
	const int vertsSize = rdAlign4(sizeof(float)*3*header->vertCount);
	const int polysSize = rdAlign4(sizeof(dtPoly)*header->polyCount);
	const int linksSize = rdAlign4(sizeof(dtLink)*(header->maxLinkCount));
	const int detailMeshesSize = rdAlign4(sizeof(dtPolyDetail)*header->detailMeshCount);
	const int detailVertsSize = rdAlign4(sizeof(float)*3*header->detailVertCount);
	const int detailTrisSize = rdAlign4(sizeof(unsigned char)*4*header->detailTriCount);
	const int bvtreeSize = rdAlign4(sizeof(dtBVNode)*header->bvNodeCount);
	const int offMeshLinksSize = rdAlign4(sizeof(dtOffMeshConnection)*header->offMeshConCount);
	
	unsigned char* d = data + headerSize;
	float* verts = rdGetThenAdvanceBufferPointer<float>(d, vertsSize);
	dtPoly* polys = rdGetThenAdvanceBufferPointer<dtPoly>(d, polysSize);
	d += linksSize; // Ignore links; they technically should be endian-swapped but all their data is overwritten on load anyway.
	//dtLink* links = rdGetThenAdvanceBufferPointer<dtLink>(d, linksSize);
	dtPolyDetail* detailMeshes = rdGetThenAdvanceBufferPointer<dtPolyDetail>(d, detailMeshesSize);
	float* detailVerts = rdGetThenAdvanceBufferPointer<float>(d, detailVertsSize);
	d += detailTrisSize; // Ignore detail tris; single bytes can't be endian-swapped.
	//unsigned char* detailTris = rdGetThenAdvanceBufferPointer<unsigned char>(d, detailTrisSize);
	dtBVNode* bvTree = rdGetThenAdvanceBufferPointer<dtBVNode>(d, bvtreeSize);
	dtOffMeshConnection* offMeshCons = rdGetThenAdvanceBufferPointer<dtOffMeshConnection>(d, offMeshLinksSize);
	
	// Vertices
	for (int i = 0; i < header->vertCount*3; ++i)
	{
		rdSwapEndian(&verts[i]);
	}

	// Polys
	for (int i = 0; i < header->polyCount; ++i)
	{
		dtPoly* p = &polys[i];
		// poly->firstLink is update when tile is added, no need to swap.
		for (int j = 0; j < DT_VERTS_PER_POLYGON; ++j)
		{
			rdSwapEndian(&p->verts[j]);
			rdSwapEndian(&p->neis[j]);
		}
		rdSwapEndian(&p->flags);
	}

	// Links are rebuild when tile is added, no need to swap.

	// Detail meshes
	for (int i = 0; i < header->detailMeshCount; ++i)
	{
		dtPolyDetail* pd = &detailMeshes[i];
		rdSwapEndian(&pd->vertBase);
		rdSwapEndian(&pd->triBase);
	}
	
	// Detail verts
	for (int i = 0; i < header->detailVertCount*3; ++i)
	{
		rdSwapEndian(&detailVerts[i]);
	}

	// BV-tree
	for (int i = 0; i < header->bvNodeCount; ++i)
	{
		dtBVNode* node = &bvTree[i];
		for (int j = 0; j < 3; ++j)
		{
			rdSwapEndian(&node->bmin[j]);
			rdSwapEndian(&node->bmax[j]);
		}
		rdSwapEndian(&node->i);
	}

	// Off-mesh Connections.
	for (int i = 0; i < header->offMeshConCount; ++i)
	{
		dtOffMeshConnection* con = &offMeshCons[i];
		for (int j = 0; j < 6; ++j)
			rdSwapEndian(&con->pos[j]);
		rdSwapEndian(&con->rad);
		rdSwapEndian(&con->poly);
	}
	
	return true;
}
