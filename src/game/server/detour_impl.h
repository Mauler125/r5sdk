#pragma once
#include "thirdparty/recast/Detour/Include/DetourStatus.h"
#include "thirdparty/recast/Detour/Include/DetourNavMesh.h"
#include "thirdparty/recast/Detour/Include/DetourNavMeshQuery.h"

//-------------------------------------------------------------------------
// RUNTIME: DETOUR
//-------------------------------------------------------------------------
inline CMemory p_Detour_LevelInit;
inline void(*v_Detour_LevelInit)(void);

inline CMemory p_Detour_FreeNavMesh;
inline void(*v_Detour_FreeNavMesh)(dtNavMesh* mesh);

inline CMemory p_dtNavMesh__Init;
inline dtStatus(*v_dtNavMesh__Init)(dtNavMesh* thisptr, unsigned char* data, int flags);

inline CMemory p_dtNavMesh__addTile;
inline dtStatus(*v_dtNavMesh__addTile)(dtNavMesh* thisptr, unsigned char* data, dtMeshHeader* header, int dataSize, int flags, dtTileRef lastRef);

inline CMemory p_dtNavMesh__isPolyReachable;
inline uint8_t(*v_dtNavMesh__isPolyReachable)(dtNavMesh* thisptr, dtPolyRef poly_1, dtPolyRef poly_2, int hull_type);


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
		LogFunAdr("Detour_LevelInit", p_Detour_LevelInit.GetPtr());
		LogFunAdr("Detour_FreeNavMesh", p_Detour_FreeNavMesh.GetPtr());
		LogFunAdr("dtNavMesh::Init", p_dtNavMesh__Init.GetPtr());
		LogFunAdr("dtNavMesh::addTile", p_dtNavMesh__addTile.GetPtr());
		LogFunAdr("dtNavMesh::isPolyReachable", p_dtNavMesh__isPolyReachable.GetPtr());
		LogVarAdr("g_pNavMesh[ MAX_HULLS ]", reinterpret_cast<uintptr_t>(g_pNavMesh));
		LogVarAdr("g_pNavMeshQuery", reinterpret_cast<uintptr_t>(g_pNavMeshQuery));
	}
	virtual void GetFun(void) const
	{

#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_Detour_LevelInit           = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 41 54 41 55 41 56 41 57 48 81 EC ?? ?? ?? ?? 45 33 F6 48 8D 3D ?? ?? ?? ??");
		p_Detour_FreeNavMesh         = g_GameDll.FindPatternSIMD("40 53 48 83 EC 30 48 89 6C 24 ?? 48 89 74 24 ??");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_Detour_LevelInit           = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 55 41 54 41 55 41 56 41 57 48 8D AC 24 ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 45 33 E4");
		p_Detour_FreeNavMesh         = g_GameDll.FindPatternSIMD("40 53 48 83 EC 30 48 89 6C 24 ?? 48 8B D9");
#endif
		p_dtNavMesh__Init            = g_GameDll.FindPatternSIMD("4C 89 44 24 ?? 53 41 56 48 81 EC ?? ?? ?? ?? 0F 10 11");
		p_dtNavMesh__addTile         = g_GameDll.FindPatternSIMD("44 89 4C 24 ?? 41 55");
		p_dtNavMesh__isPolyReachable = g_GameDll.FindPatternSIMD("48 89 6C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 41 56 49 63 F1");

		v_Detour_LevelInit           = p_Detour_LevelInit.RCast<void(*)(void)>(); /*48 89 5C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 55 41 54 41 55 41 56 41 57 48 8D AC 24 ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 45 33 E4*/
		v_Detour_FreeNavMesh         = p_Detour_FreeNavMesh.RCast<void(*)(dtNavMesh*)>();
		v_dtNavMesh__Init            = p_dtNavMesh__Init.RCast<dtStatus(*)(dtNavMesh*, unsigned char*, int)>();                                   /*4C 89 44 24 ?? 53 41 56 48 81 EC ?? ?? ?? ?? 0F 10 11*/
		v_dtNavMesh__addTile         = p_dtNavMesh__addTile.RCast<dtStatus(*)(dtNavMesh*, unsigned char*, dtMeshHeader*, int, int, dtTileRef)>(); /*44 89 4C 24 ?? 41 55*/
		v_dtNavMesh__isPolyReachable = p_dtNavMesh__isPolyReachable.RCast<uint8_t(*)(dtNavMesh*, dtPolyRef, dtPolyRef, int)>();                      /*48 89 6C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 41 56 49 63 F1*/
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
