#pragma once
#include "thirdparty/recast/detour/include/detourstatus.h"
#include "thirdparty/recast/detour/include/detournavmesh.h"
#include "thirdparty/recast/Detour/Include/DetourNavMeshQuery.h"

//-------------------------------------------------------------------------
// RUNTIME: DETOUR
//-------------------------------------------------------------------------
inline CMemory p_Detour_LevelInit;
inline auto v_Detour_LevelInit = p_Detour_LevelInit.RCast<void(*)(void)>();

inline CMemory p_Detour_FreeNavMesh;
inline auto v_Detour_FreeNavMesh = p_Detour_FreeNavMesh.RCast<void(*)(dtNavMesh* mesh)>();

inline CMemory p_dtNavMesh__Init;
inline auto v_dtNavMesh__Init = p_dtNavMesh__Init.RCast<dtStatus(*)(dtNavMesh* thisptr, unsigned char* data, int flags)>();

inline CMemory p_dtNavMesh__addTile;
inline auto v_dtNavMesh__addTile = p_dtNavMesh__addTile.RCast<dtStatus(*)(dtNavMesh* thisptr, unsigned char* data, dtMeshHeader* header, int dataSize, int flags, dtTileRef lastRef)>();

inline CMemory p_dtNavMesh__isPolyReachable;
inline auto v_dtNavMesh__isPolyReachable = p_dtNavMesh__isPolyReachable.RCast<bool(*)(dtNavMesh* thisptr, dtPolyRef poly_1, dtPolyRef poly_2, int hull_type)>();


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
		spdlog::debug("| FUN: Detour_LevelInit                     : {:#18x} |\n", p_Detour_LevelInit.GetPtr());
		spdlog::debug("| FUN: Detour_FreeNavMesh                   : {:#18x} |\n", p_Detour_FreeNavMesh.GetPtr());
		spdlog::debug("| FUN: dtNavMesh::Init                      : {:#18x} |\n", p_dtNavMesh__Init.GetPtr());
		spdlog::debug("| FUN: dtNavMesh::addTile                   : {:#18x} |\n", p_dtNavMesh__addTile.GetPtr());
		spdlog::debug("| FUN: dtNavMesh::isPolyReachable           : {:#18x} |\n", p_dtNavMesh__isPolyReachable.GetPtr());
		spdlog::debug("| VAR: g_pNavMesh[5]                        : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_pNavMesh));
		spdlog::debug("| VAR: g_pNavMeshQuery                      : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_pNavMeshQuery));
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{

#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_Detour_LevelInit           = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x81\xEC\x00\x00\x00\x00\x45\x33\xF6\x48\x8D\x3D\x00\x00\x00\x00"), "xxxx?xxxx?xxxx?xxxxxxxxxxxx????xxxxxx????");
		p_Detour_FreeNavMesh         = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x30\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00"), "xxxxxxxxxx?xxxx?");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_Detour_LevelInit           = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\xAC\x24\x00\x00\x00\x00\x48\x81\xEC\x00\x00\x00\x00\x45\x33\xE4"), "xxxx?xxxx?xxxx?xxxxxxxxxxxxx????xxx????xxx");
		p_Detour_FreeNavMesh         = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x30\x48\x89\x6C\x24\x00\x48\x8B\xD9"), "xxxxxxxxxx?xxx");
#endif
		p_dtNavMesh__Init            = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x4C\x89\x44\x24\x00\x53\x41\x56\x48\x81\xEC\x00\x00\x00\x00\x0F\x10\x11"), "xxxx?xxxxxx????xxx");
		p_dtNavMesh__addTile         = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x44\x89\x4C\x24\x00\x41\x55"), "xxxx?xx");
		p_dtNavMesh__isPolyReachable = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x41\x56\x49\x63\xF1"), "xxxx?xxxx?xxxx?xxxxx");

		v_Detour_LevelInit           = p_Detour_LevelInit.RCast<void(*)(void)>(); /*48 89 5C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 55 41 54 41 55 41 56 41 57 48 8D AC 24 ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 45 33 E4*/
		v_Detour_FreeNavMesh         = p_Detour_FreeNavMesh.RCast<void(*)(dtNavMesh*)>();
		v_dtNavMesh__Init            = p_dtNavMesh__Init.RCast<dtStatus(*)(dtNavMesh*, unsigned char*, int)>();                                   /*4C 89 44 24 ?? 53 41 56 48 81 EC ?? ?? ?? ?? 0F 10 11*/
		v_dtNavMesh__addTile         = p_dtNavMesh__addTile.RCast<dtStatus(*)(dtNavMesh*, unsigned char*, dtMeshHeader*, int, int, dtTileRef)>(); /*44 89 4C 24 ?? 41 55*/
		v_dtNavMesh__isPolyReachable = p_dtNavMesh__isPolyReachable.RCast<bool(*)(dtNavMesh*, dtPolyRef, dtPolyRef, int)>();                      /*48 89 6C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 41 56 49 63 F1*/
	}
	virtual void GetVar(void) const
	{
		g_pNavMesh = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x54\x24\x00\x48\x89\x4C\x24\x00\x55\x53\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\x6C\x24\x00\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\x02"), "xxxx?xxxx?xxxxxxxxxxxxxxxx?xxx????xxx")
			.FindPatternSelf("48 8D 3D").ResolveRelativeAddressSelf(0x3, 0x7).RCast<dtNavMesh**>();
		g_pNavMeshQuery = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x56\x57\x41\x56\x48\x81\xEC\x00\x00\x00\x00\x48\x63\xD9"), "xxxx?xxxx?xxxxxxx????xxx")
			.FindPatternSelf("48 89 0D").ResolveRelativeAddressSelf(0x3, 0x7).RCast<dtNavMeshQuery*>();
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VRecast);
