#pragma once

// !TODO: BUILD AGNOSTIC! //
namespace
{
	ADDRESS CollisionBSPData_LoadAllLumps = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x89\x54\x24\x00\x48\x89\x4C\x24\x00\x55\x53\x56\x57\x41\x54\x41\x55\x41\x57", "xxxx?xxxx?xxxxxxxxxx"); // BSP.
	// 0x1402546F0 // 48 89 54 24 ? 48 89 4C 24 ? 55 53 56 57 41 54 41 55 41 57 //

	ADDRESS CollisionBSPData_LinkPhysics = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x57\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\xF9\x33\xED", "xxxx?xxxx?xxxx????xxxxx"); // case 1: only gets called on changelevel, needs more research, function gets called by CModelLoader virtual function.
	// 0x140256480 // 48 89 5C 24 ? 48 89 6C 24 ? 57 48 81 EC ? ? ? ? 48 8B F9 33 ED //
}

///////////////////////////////////////////////////////////////////////////////
class HModel_BSP : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: CollisionBSPData_LoadAllLumps        : 0x" << std::hex << std::uppercase << CollisionBSPData_LoadAllLumps.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: CollisionBSPData_LinkPhysics         : 0x" << std::hex << std::uppercase << CollisionBSPData_LinkPhysics.GetPtr()  << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HModel_BSP);
