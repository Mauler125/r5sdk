#ifndef GAMEUTILS_H
#define GAMEUTILS_H
#include "NavEditor/Include/FileTypes.h"

void coordGameSwap(float* c);
void coordGameUnswap(float* c);

void coordShortGameSwap(unsigned short* c);
void coordShortGameUnswap(unsigned short* c);

void patchHeaderGame(NavMeshSetHeader& h);
void unpatchHeaderGame(NavMeshSetHeader& h);

void patchTileGame(dtMeshTile* t);
void unpatchTileGame(dtMeshTile* t);

void buildLinkTable(dtNavMesh* mesh, LinkTableData& data);
void setReachable(std::vector<int>& data, int count, int id1, int id2, bool value);

#endif // GAMEUTILS_H