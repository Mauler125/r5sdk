#pragma once
#include "thirdparty/recast/Detour/Include/DetourStatus.h"
#include "thirdparty/recast/Detour/Include/DetourNavMesh.h"
#include "thirdparty/recast/Detour/Include/DetourNavMeshQuery.h"

//-------------------------------------------------------------------------
// RUNTIME: DETOUR
//-------------------------------------------------------------------------
inline void(*v_Detour_LevelInit)(void);
inline void(*v_Detour_FreeNavMesh)(dtNavMesh* mesh);
inline dtStatus(*dtNavMesh__Init)(dtNavMesh* thisptr, unsigned char* data, int flags);
inline dtStatus(*dtNavMesh__addTile)(dtNavMesh* thisptr, unsigned char* data, dtMeshHeader* header, int dataSize, int flags, dtTileRef lastRef);
inline uint8_t(*dtNavMesh__isPolyReachable)(dtNavMesh* thisptr, dtPolyRef poly_1, dtPolyRef poly_2, int hull_type);


constexpr const char* NAVMESH_PATH = "maps/navmesh/";
constexpr const char* NAVMESH_EXT = ".nm";

static const char* S_HULL_TYPE[5] =
{
	"small",
	"med_short",
	"medium",
	"large",
	"extra_large"
};

enum E_HULL_TYPE
{
	SMALL = 0,
	MED_SHORT,
	MEDIUM,
	LARGE,
	EXTRA_LARGE
};

inline dtNavMesh** g_pNavMesh = nullptr;
inline dtNavMeshQuery* g_pNavMeshQuery = nullptr;

dtNavMesh* GetNavMeshForHull(int hullSize);
uint32_t GetHullMaskById(int hullId);

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
		LogFunAdr("Detour_FreeNavMesh", v_Detour_FreeNavMesh);
		LogFunAdr("dtNavMesh::Init", dtNavMesh__Init);
		LogFunAdr("dtNavMesh::addTile", dtNavMesh__addTile);
		LogFunAdr("dtNavMesh::isPolyReachable", dtNavMesh__isPolyReachable);
		LogVarAdr("g_pNavMesh[ MAX_HULLS ]", g_pNavMesh);
		LogVarAdr("g_pNavMeshQuery", g_pNavMeshQuery);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 55 41 54 41 55 41 56 41 57 48 8D AC 24 ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 45 33 E4").GetPtr(v_Detour_LevelInit);
		g_GameDll.FindPatternSIMD("40 53 48 83 EC 30 48 89 6C 24 ?? 48 8B D9").GetPtr(v_Detour_FreeNavMesh);
		g_GameDll.FindPatternSIMD("4C 89 44 24 ?? 53 41 56 48 81 EC ?? ?? ?? ?? 0F 10 11").GetPtr(dtNavMesh__Init);
		g_GameDll.FindPatternSIMD("44 89 4C 24 ?? 41 55").GetPtr(dtNavMesh__addTile);
		g_GameDll.FindPatternSIMD("48 89 6C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 41 56 49 63 F1").GetPtr(dtNavMesh__isPolyReachable);
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
