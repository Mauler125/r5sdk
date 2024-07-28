//=============================================================================//
//
// Purpose: AI system utilities
//
//=============================================================================//

#include "core/stdafx.h"
#include "tier0/fasttimer.h"
#include "tier1/cvar.h"
#include "mathlib/bitvec.h"
#include "engine/server/server.h"
#include "public/edict.h"
#include "game/server/detour_impl.h"
#include "game/server/ai_networkmanager.h"

#include "vscript/languages/squirrel_re/vsquirrel.h"

static ConVar navmesh_always_reachable("navmesh_always_reachable", "0", FCVAR_DEVELOPMENTONLY, "Marks goal poly from agent poly as reachable regardless of table data ( !slower! )");

//-----------------------------------------------------------------------------
// Purpose: gets the navmesh by type from global array [small, med_short, medium, large, extra_large]
// input  : navMeshType - 
// Output : pointer to navmesh
//-----------------------------------------------------------------------------
dtNavMesh* Detour_GetNavMeshByType(const NavMeshType_e navMeshType)
{
    Assert(navMeshType >= NULL && navMeshType < NAVMESH_COUNT); // Programmer error.
    return g_pNavMesh[navMeshType];
}

//-----------------------------------------------------------------------------
// Purpose: free's the navmesh by type from global array [small, med_short, medium, large, extra_large]
// input  : navMeshType - 
//-----------------------------------------------------------------------------
void Detour_FreeNavMeshByType(const NavMeshType_e navMeshType)
{
    Assert(navMeshType >= NULL && navMeshType < NAVMESH_COUNT); // Programmer error.
    dtNavMesh* const nav = g_pNavMesh[navMeshType];

    if (nav) // Only free if NavMesh for type is loaded.
    {
        // Frees tiles, polys, tris, anything dynamically
        // allocated for this navmesh, and the navmesh itself.
        v_Detour_FreeNavMesh(nav);
        free(nav);

        g_pNavMesh[navMeshType] = nullptr;
    }
}

//-----------------------------------------------------------------------------
// Purpose: determines whether goal poly is reachable from agent poly
//          (only checks static pathing)
// input  : *nav - 
//			fromRef - 
//			goalRef - 
//			animType - 
// Output : value if reachable, false otherwise
//-----------------------------------------------------------------------------
bool Detour_IsGoalPolyReachable(dtNavMesh* const nav, const dtPolyRef fromRef, 
    const dtPolyRef goalRef, const TraverseAnimType_e animType)
{
    if (navmesh_always_reachable.GetBool())
        return true;

    const bool hasAnimType = animType != ANIMTYPE_NONE;
    const int traversalTableIndex = hasAnimType
        ? NavMesh_GetTraversalTableIndexForAnimType(animType)
        : NULL;

    return nav->isGoalPolyReachable(fromRef, goalRef, !hasAnimType, traversalTableIndex);
}

//-----------------------------------------------------------------------------
// Purpose: initialize NavMesh and Detour query singleton for level
//-----------------------------------------------------------------------------
void Detour_LevelInit()
{
    v_Detour_LevelInit();
    Detour_IsLoaded(); // Inform user which NavMesh files had failed to load.
}

//-----------------------------------------------------------------------------
// Purpose: free's the memory used by all valid NavMesh slots
//-----------------------------------------------------------------------------
void Detour_LevelShutdown()
{
    for (int i = 0; i < NAVMESH_COUNT; i++)
    {
        Detour_FreeNavMeshByType(NavMeshType_e(i));
    }
}

//-----------------------------------------------------------------------------
// Purpose: checks if the NavMesh has failed to load
// Output : true if a NavMesh has successfully loaded, false otherwise
//-----------------------------------------------------------------------------
bool Detour_IsLoaded()
{
    int ret = 0;
    for (int i = 0; i < NAVMESH_COUNT; i++)
    {
        const dtNavMesh* nav = Detour_GetNavMeshByType(NavMeshType_e(i));
        if (!nav) // Failed to load...
        {
            Warning(eDLL_T::SERVER, "NavMesh '%s%s_%s%s' not loaded\n", 
                NAVMESH_PATH, g_ServerGlobalVariables->m_pszMapName, 
                NavMesh_GetNameForType(NavMeshType_e(i)), NAVMESH_EXT);

            ret++;
        }
    }

    Assert(ret <= NAVMESH_COUNT);
    return (ret != NAVMESH_COUNT);
}

//-----------------------------------------------------------------------------
// Purpose: hot swaps the NavMesh with the current files on the disk
// (All hulls will be reloaded! If NavMesh for hull no longer exist, it will be kept empty!!!)
// 
// TODO: Currently when hotswapping, the game crashes if there's AI in the world.
// Loop over all CAI_BaseNPC instances, and call m_pPathfinder->m_navQuery.init()
// to clear NavMesh cache !!!
//-----------------------------------------------------------------------------
void Detour_HotSwap()
{
    Assert(ThreadInMainOrServerFrameThread());
    g_pServerScript->ExecuteCodeCallback("CodeCallback_OnNavMeshHotSwapBegin");

    // Free and re-init NavMesh.
    Detour_LevelShutdown();
    v_Detour_LevelInit();

    if (!Detour_IsLoaded())
        Error(eDLL_T::SERVER, NOERROR, "%s - Failed to hot swap NavMesh\n", __FUNCTION__);

    g_pServerScript->ExecuteCodeCallback("CodeCallback_OnNavMeshHotSwapEnd");
}

/*
=====================
Detour_HotSwap_f

  Hot swaps the NavMesh
  while the game is running
=====================
*/
static void Detour_HotSwap_f()
{
    if (!g_pServer->IsActive())
        return; // Only execute if server is initialized and active.

    Msg(eDLL_T::SERVER, "Executing NavMesh hot swap for level '%s'\n",
        g_ServerGlobalVariables->m_pszMapName);

    CFastTimer timer;

    timer.Start();
    Detour_HotSwap();

    timer.End();
    Msg(eDLL_T::SERVER, "Hot swap took '%lf' seconds\n", timer.GetDuration().GetSeconds());
}

static ConCommand navmesh_hotswap("navmesh_hotswap", Detour_HotSwap_f, "Hot swap the NavMesh for all hulls", FCVAR_DEVELOPMENTONLY | FCVAR_SERVER_FRAME_THREAD);

///////////////////////////////////////////////////////////////////////////////
void VRecast::Detour(const bool bAttach) const
{
	DetourSetup(&v_Detour_IsGoalPolyReachable, &Detour_IsGoalPolyReachable, bAttach);
	DetourSetup(&v_Detour_LevelInit, &Detour_LevelInit, bAttach);
}
