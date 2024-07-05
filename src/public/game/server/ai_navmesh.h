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
	Assert(navMeshType >= 0 && navMeshType < NAVMESH_COUNT);
	return g_navMeshNames[navMeshType];
}

inline const int g_traverseAnimTypeSetTableIndices[ANIMTYPE_COUNT] = {
	// NAVMESH_SMALL has 5 reachability tables, so each traverse anim type indexes
	// into its own.
	0, 0, 0, 0, 0,

	// All other navmeshes have 1 reachability table, so we need to subtract the
	// number from the enumerant to index into the first one.
	-5, -6, -7, -8
};

inline int NavMesh_GetReachabilityTableIndexForTraverseAnimType(const TraverseAnimType_e animType)
{
	Assert(animType >= 0 && animType < V_ARRAYSIZE(g_traverseAnimTypeSetTableIndices));
	return animType + g_traverseAnimTypeSetTableIndices[animType];
}

#endif // AI_NAVMESH_H
