#pragma once

#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
inline CMemory p_CollisionBSPData_LinkPhysics = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x20\x48\x8B\xD9\x48\x83\xC1\x08\xE8\x00\x00\x00\x00\x48\x8D\x4B\x68"), "xxxxxxxxxxxxxx????xxxx");
inline auto CollisionBSPData_LinkPhysics = p_CollisionBSPData_LinkPhysics.RCast<uint64_t(*)(void* thisptr)>(); /*40 53 48 83 EC 20 48 8B D9 48 83 C1 08 E8 ? ? ? ? 48 8D 4B 68*/
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
inline CMemory p_CollisionBSPData_LinkPhysics = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x57\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\xF9\x33\xED"), "xxxx?xxxx?xxxx????xxxxx");
inline auto CollisionBSPData_LinkPhysics = p_CollisionBSPData_LinkPhysics.RCast<uint64_t(*)(void* thisptr)>(); /*48 89 5C 24 ? 48 89 6C 24 ? 57 48 81 EC ? ? ? ? 48 8B F9 33 ED*/
#endif

void MOD_PreloadPak(const string& svSetFile);
///////////////////////////////////////////////////////////////////////////////
class HModel_BSP : public IDetour
{
	virtual void GetAdr(void) const
	{
		std::cout << "| FUN: CollisionBSPData_LinkPhysics         : 0x" << std::hex << std::uppercase << p_CollisionBSPData_LinkPhysics.GetPtr()  << std::setw(nPad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HModel_BSP);
