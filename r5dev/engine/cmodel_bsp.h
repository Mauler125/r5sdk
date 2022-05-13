#pragma once

inline CMemory p_CollisionBSPData_LinkPhysics;
inline auto CollisionBSPData_LinkPhysics = p_CollisionBSPData_LinkPhysics.RCast<uint64_t(*)(void* thisptr)>();

inline CMemory p_MOD_LoadPakForMap;
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
inline auto v_MOD_LoadPakForMap = p_MOD_LoadPakForMap.RCast<bool(*)(void)>();
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
inline auto v_MOD_LoadPakForMap = p_MOD_LoadPakForMap.RCast<bool(*)(void* pBuffer)>();
#endif

void MOD_PreloadPak();

void CModelBsp_Attach();
void CModelBsp_Detach();
///////////////////////////////////////////////////////////////////////////////
class VModel_BSP : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: CollisionBSPData_LinkPhysics         : {:#18x} |\n", p_CollisionBSPData_LinkPhysics.GetPtr());
		spdlog::debug("| FUN: MOD_LoadPakForMap                    : {:#18x} |\n", p_MOD_LoadPakForMap.GetPtr());
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_CollisionBSPData_LinkPhysics = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x20\x48\x8B\xD9\x48\x83\xC1\x08\xE8\x00\x00\x00\x00\x48\x8D\x4B\x68"), "xxxxxxxxxxxxxx????xxxx");
		CollisionBSPData_LinkPhysics = p_CollisionBSPData_LinkPhysics.RCast<uint64_t(*)(void*)>(); /*48 89 5C 24 ?? 48 89 6C 24 ?? 57 48 81 EC ?? ?? ?? ?? 48 8B F9 33 ED*/

		p_MOD_LoadPakForMap = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x81\xEC\x00\x00\x00\x00\x4C\x8B\xC1\x48\x8D\x15\x00\x00\x00\x00\x48\x8D\x4C\x24\x00\xE8\x00\x00\x00\x00\x4C\x8D\x0D\x00\x00\x00\x00"), "xxx????xxxxxx????xxxx?x????xxx????");
		v_MOD_LoadPakForMap = p_MOD_LoadPakForMap.RCast<bool(*)(void)>(); /*48 81 EC ? ? ? ? 4C 8B C1 48 8D 15 ? ? ? ? 48 8D 4C 24 ? E8 ? ? ? ? 4C 8D 0D ? ? ? ?*/
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_CollisionBSPData_LinkPhysics = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x57\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\xF9\x33\xED"), "xxxx?xxxx?xxxx????xxxxx");
		CollisionBSPData_LinkPhysics = p_CollisionBSPData_LinkPhysics.RCast<uint64_t(*)(void*)>(); /*48 89 5C 24 ?? 48 89 6C 24 ?? 57 48 81 EC ?? ?? ?? ?? 48 8B F9 33 ED*/

		p_MOD_LoadPakForMap = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x81\xEC\x00\x00\x00\x00\x0F\xB6\x05\x00\x00\x00\x00\x4C\x8D\x05\x00\x00\x00\x00\x84\xC0"), "xxx????xxx????xxx????xx");
		v_MOD_LoadPakForMap = p_MOD_LoadPakForMap.RCast<bool(*)(void* pBuffer)>(); /*48 81 EC ? ? ? ? 0F B6 05 ? ? ? ? 4C 8D 05 ? ? ? ? 84 C0*/
#endif
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VModel_BSP);
