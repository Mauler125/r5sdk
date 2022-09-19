#ifndef FILETYPES_H
#define FILETYPES_H
#include "Detour/Include/DetourNavMesh.h"

static const int NAVMESHSET_MAGIC = 'M' << 24 | 'S' << 16 | 'E' << 8 | 'T'; //'MSET';
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

#endif // FILETYPES_H