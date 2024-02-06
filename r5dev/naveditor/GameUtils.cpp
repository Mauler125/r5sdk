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

#include "pch.h"
#include "NavEditor/include/GameUtils.h"
#include "NavEditor/include/FileTypes.h"

void coordGameSwap(float* c)
{
	std::swap(c[1], c[2]);
	c[2] *= -1;
}
void coordGameUnswap(float* c)
{
	c[2] *= -1;
	std::swap(c[1], c[2]);
}
void coordShortGameSwap(unsigned short* c)
{
	std::swap(c[1], c[2]);
	c[2] = std::numeric_limits<unsigned short>::max() - c[2];
}
void coordShortGameUnswap(unsigned short* c)
{
	c[2] = std::numeric_limits<unsigned short>::max() - c[2];
	std::swap(c[1], c[2]);
}
void patchHeaderGame(NavMeshSetHeader& h)
{
	coordGameSwap(h.params.orig);
}
void unpatchHeaderGame(NavMeshSetHeader& h)
{
	coordGameUnswap(h.params.orig);
}

void patchTileGame(dtMeshTile* t)
{
	coordGameSwap(t->header->bmin);
	coordGameSwap(t->header->bmax);

	for (size_t i = 0; i < t->header->vertCount * 3; i += 3)
		coordGameSwap(t->verts + i);
	for (size_t i = 0; i < t->header->detailVertCount * 3; i += 3)
		coordGameSwap(t->detailVerts + i);
	for (size_t i = 0; i < t->header->polyCount; i++)
		coordGameSwap(t->polys[i].org);
	//might be wrong because of coord change might break tree layout
	for (size_t i = 0; i < t->header->bvNodeCount; i++)
	{
		coordShortGameSwap(t->bvTree[i].bmax);
		coordShortGameSwap(t->bvTree[i].bmin);
	}
	for (size_t i = 0; i < t->header->offMeshConCount; i++)
	{
		coordGameSwap(t->offMeshCons[i].pos);
		coordGameSwap(t->offMeshCons[i].pos + 3);
		coordGameSwap(t->offMeshCons[i].unk);
	}
}
void unpatchTileGame(dtMeshTile* t)
{
	coordGameUnswap(t->header->bmin);
	coordGameUnswap(t->header->bmax);

	for (size_t i = 0; i < t->header->vertCount * 3; i += 3)
		coordGameUnswap(t->verts + i);
	for (size_t i = 0; i < t->header->detailVertCount * 3; i += 3)
		coordGameUnswap(t->detailVerts + i);
	for (size_t i = 0; i < t->header->polyCount; i++)
		coordGameUnswap(t->polys[i].org);
	//might be wrong because of coord change might break tree layout
	for (size_t i = 0; i < t->header->bvNodeCount; i++)
	{
		coordShortGameUnswap(t->bvTree[i].bmax);
		coordShortGameUnswap(t->bvTree[i].bmin);
	}
	for (size_t i = 0; i < t->header->offMeshConCount; i++)
	{
		coordGameUnswap(t->offMeshCons[i].pos);
		coordGameUnswap(t->offMeshCons[i].pos + 3);
		coordGameUnswap(t->offMeshCons[i].unk);
	}
}

void buildLinkTable(dtNavMesh* mesh, LinkTableData& data)
{
	//clear all labels
	for (int i = 0; i < mesh->getMaxTiles(); ++i)
	{
		dtMeshTile* tile = mesh->getTile(i);
		if (!tile || !tile->header || !tile->dataSize) continue;
		int pcount = tile->header->polyCount;
		for (int j = 0; j < pcount; j++)
		{
			dtPoly& poly = tile->polys[j];
			poly.disjointSetId = (unsigned short)-1;
		}
	}
	//first pass
	std::set<int> nlabels;
	for (int i = 0; i < mesh->getMaxTiles(); ++i)
	{
		dtMeshTile* tile = mesh->getTile(i);
		if (!tile || !tile->header || !tile->dataSize) continue;
		int pcount = tile->header->polyCount;
		for (int j = 0; j < pcount; j++)
		{
			dtPoly& poly = tile->polys[j];
			unsigned int plink = poly.firstLink;
			while (plink != DT_NULL_LINK)
			{
				const dtLink l = tile->links[plink];
				const dtMeshTile* t;
				const dtPoly* p;
				mesh->getTileAndPolyByRefUnsafe(l.ref, &t, &p);

				if (p->disjointSetId != (unsigned short)-1)
					nlabels.insert(p->disjointSetId);
				plink = l.next;
			}
			if (nlabels.empty())
			{
				poly.disjointSetId = (unsigned short)data.insert_new();
			}
			else
			{
				auto l = *nlabels.begin();
				poly.disjointSetId = (unsigned short)l;
				for (const int nl : nlabels)
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
		int pcount = tile->header->polyCount;
		for (int j = 0; j < pcount; j++)
		{
			dtPoly& poly = tile->polys[j];
			int id = data.find(poly.disjointSetId);
			poly.disjointSetId = (unsigned short)id;
		}
	}
}
void setReachable(std::vector<int>& data, int count, int id1, int id2, bool value)
{
	int w = ((count + 31) / 32);
	auto& cell = data[id1 * w + id2 / 32];
	uint32_t value_mask = ~(1 << (id2 & 0x1f));
	if (!value)
		cell = (cell & value_mask);
	else
		cell = (cell & value_mask) | (1 << (id2 & 0x1f));
}