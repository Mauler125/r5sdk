#ifndef GAMEUTILS_H
#define GAMEUTILS_H
#include "Detour/Include/DetourNavMesh.h"

void coordGameSwap(float* c);
void coordGameUnswap(float* c);

void coordShortGameSwap(unsigned short* c);
void coordShortGameUnswap(unsigned short* c);

void patchHeaderGame(dtNavMeshSetHeader& h);
void unpatchHeaderGame(dtNavMeshSetHeader& h);

void patchTileGame(dtMeshTile* t);
void unpatchTileGame(dtMeshTile* t);

#endif // GAMEUTILS_H