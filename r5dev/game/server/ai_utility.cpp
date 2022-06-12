//=============================================================================//
//
// Purpose: AI system utility
//
//=============================================================================//

#include "core/stdafx.h"
#include "tier1/cvar.h"
#include "game/server/detour_impl.h"

//-----------------------------------------------------------------------------
// Purpose: determines whether target poly is reachable from current agent poly
// input  : *this - 
//			poly_1 - 
//			poly_2 - 
//			hull_type - 
// Output : true if reachable, false otherwise
//-----------------------------------------------------------------------------
bool dtNavMesh__isPolyReachable(dtNavMesh* thisptr, dtPolyRef poly_1, dtPolyRef poly_2, int hull_type)
{
	if (navmesh_always_reachable->GetBool())
	{
		return true;
	}
	return v_dtNavMesh__isPolyReachable(thisptr, poly_1, poly_2, hull_type);
}

///////////////////////////////////////////////////////////////////////////////
void CAI_Utility_Attach()
{
	DetourAttach((LPVOID*)&v_dtNavMesh__isPolyReachable, &dtNavMesh__isPolyReachable);
}

void CAI_Utility_Detach()
{
	DetourDetach((LPVOID*)&v_dtNavMesh__isPolyReachable, &dtNavMesh__isPolyReachable);
}