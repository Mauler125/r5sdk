#pragma once
#include "thirdparty/recast/Detour/Include/DetourStatus.h"
#include "thirdparty/recast/Detour/Include/DetourNavMesh.h"
#include "thirdparty/recast/Detour/Include/DetourNavMeshQuery.h"
#include "game/server/ai_navmesh.h"

//-------------------------------------------------------------------------
// RUNTIME: DETOUR
//-------------------------------------------------------------------------
inline void(*v_Detour_LevelInit)(void);
inline bool(*v_Detour_IsGoalPolyReachable)(dtNavMesh* const nav, const dtPolyRef fromPoly, const dtPolyRef goalPoly, const TraverseAnimType_e animType);
inline dtStatus(*dtNavMesh__Init)(dtNavMesh* thisptr, unsigned char* data, int flags);
inline dtStatus(*dtNavMesh__addTile)(dtNavMesh* thisptr, void* unused, unsigned char* data, int dataSize, int flags, dtTileRef lastRef);
inline dtStatus(*dtNavMeshQuery__findNearestPoly)(dtNavMeshQuery* query, const float* center, const float* halfExtents,
	const dtQueryFilter* filter,
	dtPolyRef* nearestRef, float* nearestPt);


constexpr const char* NAVMESH_PATH = "maps/navmesh/";
constexpr const char* NAVMESH_EXT = ".nm";

inline dtNavMesh** g_pNavMesh = nullptr;
inline dtNavMeshQuery* g_pNavMeshQuery = nullptr;

dtNavMesh* Detour_GetNavMeshByType(const NavMeshType_e navMeshType);

void Detour_LevelInit();
void Detour_LevelShutdown();
bool Detour_IsLoaded();
void Detour_HotSwap();
///////////////////////////////////////////////////////////////////////////////
class VRecast : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("Detour_LevelInit", v_Detour_LevelInit);
		LogFunAdr("Detour_IsGoalPolyReachable", v_Detour_IsGoalPolyReachable);
		LogFunAdr("dtNavMesh::Init", dtNavMesh__Init);
		LogFunAdr("dtNavMesh::addTile", dtNavMesh__addTile);
		LogFunAdr("dtNavMeshQuery::findNearestPoly", dtNavMeshQuery__findNearestPoly);
		LogVarAdr("g_pNavMesh[ NavMeshType_e::NAVMESH_COUNT ]", g_pNavMesh);
		LogVarAdr("g_pNavMeshQuery", g_pNavMeshQuery);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 55 41 54 41 55 41 56 41 57 48 8D AC 24 ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 45 33 E4").GetPtr(v_Detour_LevelInit);
		g_GameDll.FindPatternSIMD("48 89 6C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 41 56 49 63 F1").GetPtr(v_Detour_IsGoalPolyReachable);
		g_GameDll.FindPatternSIMD("4C 89 44 24 ?? 53 41 56 48 81 EC ?? ?? ?? ?? 0F 10 11").GetPtr(dtNavMesh__Init);
		g_GameDll.FindPatternSIMD("44 89 4C 24 ?? 41 55").GetPtr(dtNavMesh__addTile);
		g_GameDll.FindPatternSIMD("4C 8B DC 49 89 4B ?? 41 54 41 57").GetPtr(dtNavMeshQuery__findNearestPoly);
	}
	virtual void GetVar(void) const
	{
		g_pNavMesh = g_GameDll.FindPatternSIMD("48 89 54 24 ?? 48 89 4C 24 ?? 55 53 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 ?? 48 81 EC ?? ?? ?? ?? 48 8B 02")
			.FindPatternSelf("48 8D 3D").ResolveRelativeAddressSelf(0x3, 0x7).RCast<dtNavMesh**>();
		g_pNavMeshQuery = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 56 57 41 56 48 81 EC ?? ?? ?? ?? 48 63 D9")
			.FindPatternSelf("48 89 0D").ResolveRelativeAddressSelf(0x3, 0x7).RCast<dtNavMeshQuery*>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
