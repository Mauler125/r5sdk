#ifndef DETOUR_DUMP_H
#define DETOUR_DUMP_H

#include "FileIO.h"

bool duDumpTraverseLinkDetail(const dtNavMesh& mesh, const dtNavMeshQuery* query, const int traverseType, duFileIO* const io);

#endif // DETOUR_DUMP_H
