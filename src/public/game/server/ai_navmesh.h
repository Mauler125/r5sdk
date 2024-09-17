//=============================================================================//
//
// Purpose: defines the NavMesh types for AI
//
//=============================================================================//
#ifndef AI_NAVMESH_H
#define AI_NAVMESH_H

#include "ai_agent.h"

enum NavMeshType_e
{
	NAVMESH_SMALL = 0,
	NAVMESH_MED_SHORT,
	NAVMESH_MEDIUM,
	NAVMESH_LARGE,
	NAVMESH_EXTRA_LARGE,

	// Not a NavMesh!
	NAVMESH_COUNT
};

inline const char* const g_navMeshNames[NAVMESH_COUNT] = {
	"small",
	"med_short",
	"medium",
	"large",
	"extra_large"
};

inline const char* NavMesh_GetNameForType(const NavMeshType_e navMeshType)
{
	Assert(navMeshType >= 0 && navMeshType < V_ARRAYSIZE(g_navMeshNames));
	return g_navMeshNames[navMeshType];
}

inline const int g_navMeshTraverseTableCountIndicesPerType[NAVMESH_COUNT] = {
	// NAVMESH_SMALL supports the first 5 enumerants in TraverseAnimType_e
	5,

	// All other navmeshes support one each, counting from after the last one
	// supported by NAVMESH_SMALL
	1, 1, 1, 1
};

inline int NavMesh_GetTraverseTableCountForNavMeshType(const NavMeshType_e navMeshType)
{
	Assert(navMeshType >= 0 && navMeshType < NAVMESH_COUNT);
	return g_navMeshTraverseTableCountIndicesPerType[navMeshType];
}

inline const int g_navMeshTraverseTableIndicesPerType[ANIMTYPE_COUNT] = {
	// NAVMESH_SMALL has 5 traversal tables, so each traverse anim type indexes
	// into its own.
	0, 0, 0, 0, 0,

	// All other navmeshes have 1 traversal table, so we need to subtract the
	// number from the enumerant to index into the first one.
	-5, -6, -7, -8
};

inline int NavMesh_GetTraverseTableIndexForAnimType(const TraverseAnimType_e animType)
{
	Assert(animType >= 0 && animType < ANIMTYPE_COUNT);
	return animType + g_navMeshTraverseTableIndicesPerType[animType];
}


inline const TraverseAnimType_e g_navMeshFirstTraverseAnimTypeForNavMeshType[NAVMESH_COUNT]
{
	ANIMTYPE_HUMAN, // NAVMESH_SMALL starts at traversal table 0, 
	ANIMTYPE_PROWLER,
	ANIMTYPE_SUPER_SPECTRE,
	ANIMTYPE_TITAN,
	ANIMTYPE_GOLIATH
};

inline TraverseAnimType_e NavMesh_GetFirstTraverseAnimTypeForType(const NavMeshType_e navMeshType)
{
	Assert(navMeshType >= 0 && navMeshType < NAVMESH_COUNT);
	return g_navMeshFirstTraverseAnimTypeForNavMeshType[navMeshType];
}

#endif // AI_NAVMESH_H
