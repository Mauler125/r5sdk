#pragma once

// !TODO: BUILD AGNOSTIC! //
namespace
{
	ADDRESS CollisionBSPData_LinkPhysics = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x57\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\xF9\x33\xED", "xxxx?xxxx?xxxx????xxxxx"); // case 1: only gets called on changelevel, needs more research, function gets called by CModelLoader virtual function.
	// 0x140256480 // 48 89 5C 24 ? 48 89 6C 24 ? 57 48 81 EC ? ? ? ? 48 8B F9 33 ED //
}

enum class eBspRes_t
{
	RES_RPAK = 0,
	RES_VPK,
	RES_STBSP
};

extern int g_nLoadedPakFileId[256];

void MOD_LoadDependencies(eBspRes_t resourceType);
///////////////////////////////////////////////////////////////////////////////
class HModel_BSP : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: CollisionBSPData_LinkPhysics         : 0x" << std::hex << std::uppercase << CollisionBSPData_LinkPhysics.GetPtr()  << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HModel_BSP);
