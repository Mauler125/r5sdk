//=============================================================================//
//
// Purpose: AI system utility
//
//=============================================================================//

#include "core/stdafx.h"
#include "tier1/cvar.h"
#include "game/server/detour_impl.h"
#include "game/server/ai_networkmanager.h"

inline uint32_t g_pHullMasks[10] = // Hull mask table [r5apex_ds.exe + 131a2f8].
{
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xfffffffb, 0xfffffffa, 0xfffffff9, 0xfffffff8, 0x00040200
};

//-----------------------------------------------------------------------------
// Purpose: gets the navmesh by hull from global array [small, med_short, medium, large, extra_large]
// input  : hull - 
// Output : pointer to navmesh
//-----------------------------------------------------------------------------
dtNavMesh* GetNavMeshForHull(int hullSize)
{
    assert(hullSize >= 0 && hullSize <= 4); // Programmer error.
    return g_pNavMesh[hullSize];
}

//-----------------------------------------------------------------------------
// Purpose: gets hull mask by id
// input  : hullId - 
// Output : hull mask
//-----------------------------------------------------------------------------
uint32_t GetHullMaskById(int hullId)
{
    assert(hullId >= 0 && hullId <= 9); // Programmer error.
    return (hullId + g_pHullMasks[hullId]);
}

//-----------------------------------------------------------------------------
// Purpose: determines whether goal poly is reachable from agent poly
// input  : *nav - 
//			fromRef - 
//			goalRef - 
//			hull_type - 
// Output : value if reachable, false otherwise
//-----------------------------------------------------------------------------
uint8_t IsGoalPolyReachable(dtNavMesh* nav, dtPolyRef fromRef, dtPolyRef goalRef, int hullId)
{
	if (navmesh_always_reachable->GetBool())
		return true;

    return v_dtNavMesh__isPolyReachable(nav, fromRef, goalRef, hullId);
}

///////////////////////////////////////////////////////////////////////////////
void CAI_Utility_Attach()
{
	DetourAttach((LPVOID*)&v_dtNavMesh__isPolyReachable, &IsGoalPolyReachable);
}

void CAI_Utility_Detach()
{
	DetourDetach((LPVOID*)&v_dtNavMesh__isPolyReachable, &IsGoalPolyReachable);
}