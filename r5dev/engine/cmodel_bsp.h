#pragma once

inline CMemory p_CollisionBSPData_LinkPhysics;
inline auto CollisionBSPData_LinkPhysics = p_CollisionBSPData_LinkPhysics.RCast<uint64_t(*)(void* thisptr)>();

inline CMemory p_MOD_LoadPakForMap;
inline auto v_MOD_LoadPakForMap = p_MOD_LoadPakForMap.RCast<bool(*)(const char* szLevelName)>();

inline CMemory p_MOD_ProcessPakQueue;
inline auto v_MOD_ProcessPakQueue = p_MOD_ProcessPakQueue.RCast<void(*)(void)>();

inline float* dword_14B383420;
inline int32_t * dword_1634F445C;
inline void** qword_167ED7BB8;
inline void** qword_14180A098;
inline bool* byte_16709DDDF;
inline char** off_141874660;
inline void** unk_141874555;
inline void** unk_1418749B0;
inline void** unk_141874550;
inline int64_t* qword_167ED7BC0;
inline int64_t* qword_167ED7C68;
inline int64_t* qword_167ED7BE0;
inline int64_t* qword_14045C070;
inline DWORD* dword_1641E443C;
inline bool* byte_167208B0C;

inline auto sub_14045BAC0 = p_MOD_ProcessPakQueue.RCast<__int64(*)(__int64(__fastcall* a1)(__int64, _DWORD*, __int64, _QWORD*), __int64 a2, __int64 a3, __int64 a4)>();
inline auto sub_14045A1D0 = p_MOD_ProcessPakQueue.RCast<__int64(*)(unsigned __int8(__fastcall* a1)(_QWORD), __int64 a2, __int64 a3, __int64 a4, volatile signed __int64* a5, char a6)>();
inline auto sub_140441220 = p_MOD_ProcessPakQueue.RCast<void(*)(__int64 a1, __int64 a2)>();

extern bool s_bBasePaksInitialized;
extern string g_svLevelName;
extern vector<string> g_vAllMaps;

bool MOD_LevelHasChanged(const string& svLevelName);
void MOD_GetAllInstalledMaps();
void MOD_PreloadPakFile(const string& svLevelName);
void MOD_UnloadPakFile(void);

void CModelBsp_Attach();
void CModelBsp_Detach();
///////////////////////////////////////////////////////////////////////////////
class VModel_BSP : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: CollisionBSPData_LinkPhysics         : {:#18x} |\n", p_CollisionBSPData_LinkPhysics.GetPtr());
		spdlog::debug("| FUN: MOD_LoadPakForMap                    : {:#18x} |\n", p_MOD_LoadPakForMap.GetPtr());
		spdlog::debug("| FUN: MOD_ProcessPakQueue                  : {:#18x} |\n", p_MOD_ProcessPakQueue.GetPtr());
		spdlog::debug("+----------------------------------------------------------------+\n");
		spdlog::debug("| FUN: sub_14045BAC0                        : {:#18x} |\n", reinterpret_cast<uintptr_t>(sub_14045BAC0));
		spdlog::debug("| FUN: sub_14045A1D0                        : {:#18x} |\n", reinterpret_cast<uintptr_t>(sub_14045A1D0));
		spdlog::debug("| FUN: sub_140441220                        : {:#18x} |\n", reinterpret_cast<uintptr_t>(sub_140441220));
		spdlog::debug("| VAR: dword_14B383420                      : {:#18x} |\n", reinterpret_cast<uintptr_t>(dword_14B383420));
		spdlog::debug("| VAR: dword_1634F445C                      : {:#18x} |\n", reinterpret_cast<uintptr_t>(dword_1634F445C));
		spdlog::debug("| VAR: qword_167ED7BB8                      : {:#18x} |\n", reinterpret_cast<uintptr_t>(qword_167ED7BB8));
		spdlog::debug("| VAR: qword_14180A098                      : {:#18x} |\n", reinterpret_cast<uintptr_t>(qword_14180A098));
		spdlog::debug("| VAR: byte_16709DDDF                       : {:#18x} |\n", reinterpret_cast<uintptr_t>(byte_16709DDDF));
		spdlog::debug("| VAR: off_141874660                        : {:#18x} |\n", reinterpret_cast<uintptr_t>(off_141874660));
		spdlog::debug("| VAR: unk_141874555                        : {:#18x} |\n", reinterpret_cast<uintptr_t>(unk_141874555));
		spdlog::debug("| VAR: unk_1418749B0                        : {:#18x} |\n", reinterpret_cast<uintptr_t>(unk_1418749B0));
		spdlog::debug("| VAR: unk_141874550                        : {:#18x} |\n", reinterpret_cast<uintptr_t>(unk_141874550));
		spdlog::debug("| VAR: qword_167ED7BC0                      : {:#18x} |\n", reinterpret_cast<uintptr_t>(qword_167ED7BC0));
		spdlog::debug("| VAR: qword_167ED7C68                      : {:#18x} |\n", reinterpret_cast<uintptr_t>(qword_167ED7C68));
		spdlog::debug("| VAR: qword_167ED7BE0                      : {:#18x} |\n", reinterpret_cast<uintptr_t>(qword_167ED7BE0));
		spdlog::debug("| VAR: qword_14045C070                      : {:#18x} |\n", reinterpret_cast<uintptr_t>(qword_14045C070));
		spdlog::debug("| VAR: dword_1641E443C                      : {:#18x} |\n", reinterpret_cast<uintptr_t>(dword_1641E443C));
		spdlog::debug("| VAR: byte_167208B0C                       : {:#18x} |\n", reinterpret_cast<uintptr_t>(byte_167208B0C));
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_CollisionBSPData_LinkPhysics = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x20\x48\x8B\xD9\x48\x83\xC1\x08\xE8\x00\x00\x00\x00\x48\x8D\x4B\x68"), "xxxxxxxxxxxxxx????xxxx");
		CollisionBSPData_LinkPhysics = p_CollisionBSPData_LinkPhysics.RCast<uint64_t(*)(void*)>(); /*48 89 5C 24 ?? 48 89 6C 24 ?? 57 48 81 EC ?? ?? ?? ?? 48 8B F9 33 ED*/

		p_MOD_LoadPakForMap = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x81\xEC\x00\x00\x00\x00\x4C\x8B\xC1\x48\x8D\x15\x00\x00\x00\x00\x48\x8D\x4C\x24\x00\xE8\x00\x00\x00\x00\x4C\x8D\x0D\x00\x00\x00\x00"), "xxx????xxxxxx????xxxx?x????xxx????");
		v_MOD_LoadPakForMap = p_MOD_LoadPakForMap.RCast<bool(*)(const char*)>(); /*48 81 EC ? ? ? ? 4C 8B C1 48 8D 15 ? ? ? ? 48 8D 4C 24 ? E8 ? ? ? ? 4C 8D 0D ? ? ? ?*/
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_CollisionBSPData_LinkPhysics = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x57\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\xF9\x33\xED"), "xxxx?xxxx?xxxx????xxxxx");
		CollisionBSPData_LinkPhysics = p_CollisionBSPData_LinkPhysics.RCast<uint64_t(*)(void*)>(); /*48 89 5C 24 ?? 48 89 6C 24 ?? 57 48 81 EC ?? ?? ?? ?? 48 8B F9 33 ED*/

		p_MOD_LoadPakForMap = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x81\xEC\x00\x00\x00\x00\x0F\xB6\x05\x00\x00\x00\x00\x4C\x8D\x05\x00\x00\x00\x00\x84\xC0"), "xxx????xxx????xxx????xx");
		v_MOD_LoadPakForMap = p_MOD_LoadPakForMap.RCast<bool(*)(const char*)>(); /*48 81 EC ? ? ? ? 0F B6 05 ? ? ? ? 4C 8D 05 ? ? ? ? 84 C0*/
#endif
		p_MOD_ProcessPakQueue = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x00\xF3\x0F\x10\x05\x00\x00\x00\x00\x32\xDB"), "xxxxx?xxxx????xx");
		v_MOD_ProcessPakQueue = p_MOD_ProcessPakQueue.RCast<void(*)(void)>(); /*40 53 48 83 EC ?? F3 0F 10 05 ? ? ? ? 32 DB*/

		sub_14045BAC0 = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x4C\x89\x4C\x24\x00\x4C\x89\x44\x24\x00\x55\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x83\xEC\x60"), "xxxx?xxxx?xxxx?xxxxxxxxxxxxxxx").RCast<__int64(*)(__int64(__fastcall* a1)(__int64, _DWORD*, __int64, _QWORD*), __int64 a2, __int64 a3, __int64 a4)>();
		sub_14045A1D0 = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x4C\x89\x4C\x24\x00\x4C\x89\x44\x24\x00\x48\x89\x54\x24\x00\x48\x89\x4C\x24\x00\x55\x53\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\x6C\x24\x00"), "xxxx?xxxx?xxxx?xxxx?xxxxxxxxxxxxxxxx?").RCast<__int64(*)(unsigned __int8(__fastcall* a1)(_QWORD), __int64 a2, __int64 a3, __int64 a4, volatile signed __int64* a5, char a6)>();
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		sub_140441220 = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x55\x48\x83\xEC\x20\x33\xED\x48\x39\x2D\x00\x00\x00\x00"), "xxxx?xxxxxxxxxx????").RCast<void(*)(__int64 a1, __int64 a2)>();
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		sub_140441220 = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x20\x33\xED\x48\x8D\x35\x00\x00\x00\x00\x48\x39\x2D\x00\x00\x00\x00"), "xxxx?xxxx?xxxx?xxxxxxxxxx????xxx????").RCast<void(*)(__int64 a1, __int64 a2)>();
#endif
	}
	virtual void GetVar(void) const
	{
		dword_14B383420 = p_MOD_ProcessPakQueue.FindPattern("F3 0F 10").ResolveRelativeAddressSelf(0x4, 0x8).RCast<float*>();
		dword_1634F445C = p_MOD_ProcessPakQueue.FindPattern("8B 05").ResolveRelativeAddressSelf(0x2, 0x6).RCast<int32_t*>();
		qword_167ED7BB8 = p_MOD_ProcessPakQueue.Offset(0x10).FindPattern("48 83").ResolveRelativeAddressSelf(0x3, 0x8).RCast<void**>();
		qword_14180A098 = p_MOD_ProcessPakQueue.Offset(0x20).FindPattern("83 3D").ResolveRelativeAddressSelf(0x2, 0x7).RCast<void**>();
		byte_16709DDDF = p_MOD_ProcessPakQueue.Offset(0x20).FindPattern("88 1D").ResolveRelativeAddressSelf(0x2, 0x6).RCast<bool*>();
		off_141874660 = p_MOD_ProcessPakQueue.Offset(0x40).FindPattern("4C 8D 15").ResolveRelativeAddressSelf(0x3, 0x7).RCast<char**>();
		unk_141874555 = p_MOD_ProcessPakQueue.Offset(0x40).FindPattern("4C 8D 1D").ResolveRelativeAddressSelf(0x3, 0x7).RCast<void**>();
		unk_1418749B0 = p_MOD_ProcessPakQueue.Offset(0xA0).FindPattern("48 8D 1D").ResolveRelativeAddressSelf(0x3, 0x7).RCast<void**>();
		unk_141874550 = p_MOD_ProcessPakQueue.Offset(0x150).FindPattern("48 8D 2D").ResolveRelativeAddressSelf(0x3, 0x7).RCast<void**>();
		qword_167ED7BC0 = p_MOD_ProcessPakQueue.Offset(0x200).FindPattern("48 83 3D").ResolveRelativeAddressSelf(0x3, 0x8).RCast<int64_t*>();
		qword_167ED7C68 = p_MOD_ProcessPakQueue.Offset(0x200).FindPattern("0F B7 05").ResolveRelativeAddressSelf(0x3, 0x7).RCast<int64_t*>();
		qword_167ED7BE0 = p_MOD_ProcessPakQueue.Offset(0x250).FindPattern("48 8D 0D").ResolveRelativeAddressSelf(0x3, 0x7).RCast<int64_t*>();
		qword_14045C070 = p_MOD_ProcessPakQueue.Offset(0x2A0).FindPattern("48 8D 0D").ResolveRelativeAddressSelf(0x3, 0x7).RCast<int64_t*>();
		dword_1641E443C = p_MOD_ProcessPakQueue.Offset(0x2A0).FindPattern("3B 05").ResolveRelativeAddressSelf(0x2, 0x6).RCast<DWORD*>();
		byte_167208B0C = p_MOD_ProcessPakQueue.Offset(0x2A0).FindPattern("C6 05").ResolveRelativeAddressSelf(0x2, 0x7).RCast<bool*>();

		(*((char**)(&qword_167ED7C68))) -= 6;
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VModel_BSP);
