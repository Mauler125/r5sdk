#pragma once
#include "thirdparty/recast/detour/include/detourstatus.h"
#include "thirdparty/recast/detour/include/detournavmesh.h"

//-------------------------------------------------------------------------
// RUNTIME: DETOUR
//-------------------------------------------------------------------------
inline CMemory p_dtNavMesh__Init;
inline auto v_dtNavMesh__Init = p_dtNavMesh__Init.RCast<dtStatus(*)(dtNavMesh* thisptr, unsigned char* data, int flags)>();

inline CMemory p_dtNavMesh__addTile;
inline auto v_dtNavMesh__addTile = p_dtNavMesh__addTile.RCast<dtStatus(*)(dtNavMesh* thisptr, unsigned char* data, dtMeshHeader* header, int dataSize, int flags, dtTileRef lastRef)>();

inline CMemory p_dtNavMesh__isPolyReachable;
inline auto v_dtNavMesh__isPolyReachable = p_dtNavMesh__isPolyReachable.RCast<bool(*)(dtNavMesh* thisptr, dtPolyRef poly_1, dtPolyRef poly_2, int hull_type)>();
///////////////////////////////////////////////////////////////////////////////
class VRecast : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: dtNavMesh::Init                      : {:#18x} |\n", p_dtNavMesh__Init.GetPtr());
		spdlog::debug("| FUN: dtNavMesh::addTile                   : {:#18x} |\n", p_dtNavMesh__addTile.GetPtr());
		spdlog::debug("| FUN: dtNavMesh::isPolyReachable           : {:#18x} |\n", p_dtNavMesh__isPolyReachable.GetPtr());
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
		p_dtNavMesh__Init            = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x4C\x89\x44\x24\x00\x53\x41\x56\x48\x81\xEC\x00\x00\x00\x00\x0F\x10\x11"), "xxxx?xxxxxx????xxx");
		p_dtNavMesh__addTile         = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x44\x89\x4C\x24\x00\x41\x55"), "xxxx?xx");
		p_dtNavMesh__isPolyReachable = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x41\x56\x49\x63\xF1"), "xxxx?xxxx?xxxx?xxxxx");

		v_dtNavMesh__Init            = p_dtNavMesh__Init.RCast<dtStatus(*)(dtNavMesh*, unsigned char*, int)>(); /*4C 89 44 24 ? 53 41 56 48 81 EC ? ? ? ? 0F 10 11*/
		v_dtNavMesh__addTile         = p_dtNavMesh__addTile.RCast<dtStatus(*)(dtNavMesh*, unsigned char*, dtMeshHeader*, int, int, dtTileRef)>(); /*44 89 4C 24 ? 41 55*/
		v_dtNavMesh__isPolyReachable = p_dtNavMesh__isPolyReachable.RCast<bool(*)(dtNavMesh*, dtPolyRef, dtPolyRef, int)>(); /*48 89 6C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 56 49 63 F1*/
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VRecast);
