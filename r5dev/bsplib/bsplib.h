#pragma once


inline CMemory p_BuildPropStaticFrustumCullMap;
inline auto v_BuildPropStaticFrustumCullMap = p_BuildPropStaticFrustumCullMap.RCast<void*(*)(__int64 a1, __int64 a2, unsigned int a3, unsigned int a4, __int64 a5, __int64 a6, __int64 a7)>();

//inline void** (*sub_1404365A0)(__m128*, const __m128i*, __m128i*, double) = nullptr;
//inline __m128 (*sub_140270130)(__m128*) = nullptr;
//inline const __m128i* (*sub_14028F170)(__int64, __int64, __m128*, const __m128i*, const __m128i*) = nullptr;
//inline int64_t(*sub_140257F20)(void*, __int64, __m128i*, __int8*) = nullptr;
//
//inline int32_t* dword_1696A9D20 = nullptr;
//inline int32_t* dword_141744EBC = nullptr;
//inline int32_t* dword_141744EE8 = nullptr;
//
//inline int64_t* qword_141744EA8 = nullptr;
//inline int64_t* qword_141744EA0 = nullptr;
//inline int64_t* qword_141744E88 = nullptr;
//
//inline __m128* xmmword_1415BD270 = nullptr;
//
//inline void* off_141744E70 = nullptr;
//inline void* off_141731448 = nullptr;

void* __fastcall BuildPropStaticFrustumCullMap(int64_t a1, int64_t a2, unsigned int a3, unsigned int a4, int64_t a5, int64_t a6, int64_t a7);

void BspLib_Attach();
void BspLib_Detach();
///////////////////////////////////////////////////////////////////////////////
class VBspLib : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: BuildPropStaticFrustumCullMap        : {:#18x} |\n", p_BuildPropStaticFrustumCullMap.GetPtr());
		//spdlog::debug("| FUN: sub_1404365A0                        : {:#18x} |\n", reinterpret_cast<uintptr_t>(sub_1404365A0));
		//spdlog::debug("| FUN: sub_140270130                        : {:#18x} |\n", reinterpret_cast<uintptr_t>(sub_140270130));
		//spdlog::debug("| FUN: sub_14028F170                        : {:#18x} |\n", reinterpret_cast<uintptr_t>(sub_14028F170));
		//spdlog::debug("| FUN: sub_140257F20                        : {:#18x} |\n", reinterpret_cast<uintptr_t>(sub_140257F20));
		//spdlog::debug("| VAR: dword_1696A9D20                      : {:#18x} |\n", reinterpret_cast<uintptr_t>(dword_1696A9D20));
		//spdlog::debug("| VAR: dword_141744EBC                      : {:#18x} |\n", reinterpret_cast<uintptr_t>(dword_141744EBC));
		//spdlog::debug("| VAR: dword_141744EE8                      : {:#18x} |\n", reinterpret_cast<uintptr_t>(dword_141744EE8));
		//spdlog::debug("| VAR: qword_141744EA8                      : {:#18x} |\n", reinterpret_cast<uintptr_t>(qword_141744EA8));
		//spdlog::debug("| VAR: qword_141744EA0                      : {:#18x} |\n", reinterpret_cast<uintptr_t>(qword_141744EA0));
		//spdlog::debug("| VAR: qword_141744E88                      : {:#18x} |\n", reinterpret_cast<uintptr_t>(qword_141744E88));
		//spdlog::debug("| VAR: xmmword_1415BD270                    : {:#18x} |\n", reinterpret_cast<uintptr_t>(xmmword_1415BD270));
		//spdlog::debug("| VAR: off_141744E70                        : {:#18x} |\n", reinterpret_cast<uintptr_t>(off_141744E70));
		//spdlog::debug("| VAR: off_141731448                        : {:#18x} |\n", reinterpret_cast<uintptr_t>(off_141731448));
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
		p_BuildPropStaticFrustumCullMap = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\xC4\x44\x89\x40\x18\x48\x89\x50\x10\x55"), "xxxxxxxxxxxx"); /*48 8B C4 44 89 40 18 48 89 50 10 55*/
		v_BuildPropStaticFrustumCullMap = p_BuildPropStaticFrustumCullMap.RCast<void*(*)(__int64, __int64, unsigned int, unsigned int, __int64, __int64, __int64)>();

		//sub_1404365A0 = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\xC4\x48\x83\xEC\x78\xF3\x41\x0F\x10\x48\x00"), "xxxxxxxxxxxx?").RCast<void** (*)(__m128*, const __m128i*, __m128i*, double)>();
		//sub_140270130 = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x83\xEC\x28\x66\x0F\x6F\x15\x00\x00\x00\x00"), "xxxxxxxx????").RCast<__m128(*)(__m128*)>();
		//sub_14028F170 = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x83\xEC\x58\xF3\x41\x0F\x7E\x11"), "xxxxxxxxx").RCast<const __m128i* (*)(__int64, __int64, __m128*, const __m128i*, const __m128i*)>();
		//sub_140257F20 = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x57\x48\x83\xEC\x20\x49\x8B\xD9\x49\x8B\xF8\x48\x85\xD2"), "xxxx?xxxxxxxxxxxxxx").RCast<__int64(*)(void*, __int64, __m128i*, __int8*)>();
	}
	virtual void GetVar(void) const
	{
//		dword_1696A9D20 = p_BuildPropStaticFrustumCullMap.FindPattern("89 0D").ResolveRelativeAddressSelf(0x2, 0x6).RCast<int32_t*>();
//		dword_141744EBC = p_BuildPropStaticFrustumCullMap.Offset(0x200).FindPattern("44 8B").ResolveRelativeAddressSelf(0x3, 0x7).RCast<int32_t*>();
//		dword_141744EE8 = p_BuildPropStaticFrustumCullMap.Offset(0x550).FindPattern("8B 15").ResolveRelativeAddressSelf(0x2, 0x6).RCast<int32_t*>();
//
//		qword_141744EA8 = p_BuildPropStaticFrustumCullMap.Offset(0x150).FindPattern("48 8B").ResolveRelativeAddressSelf(0x3, 0x7).RCast<int64_t*>();
//		qword_141744EA0 = p_BuildPropStaticFrustumCullMap.Offset(0x220).FindPattern("48 8B").ResolveRelativeAddressSelf(0x3, 0x7).RCast<int64_t*>();
//		qword_141744E88 = p_BuildPropStaticFrustumCullMap.Offset(0x4E0).FindPattern("48 8B").ResolveRelativeAddressSelf(0x3, 0x7).RCast<int64_t*>();
//
//		xmmword_1415BD270 = p_BuildPropStaticFrustumCullMap.Offset(0x1A0).FindPattern("0F 59").ResolveRelativeAddressSelf(0x3, 0x7).RCast<__m128*>();
//
//		off_141744E70 = p_BuildPropStaticFrustumCullMap.Offset(0x550).FindPattern("4C 8D").ResolveRelativeAddressSelf(0x3, 0x7).RCast<void*>();
//#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
//		off_141731448 = p_CalcPropStaticFrustumCulling.Offset(0x1F0).FindPattern("48 ?? ?? ?? ?? ?? 01").ResolveRelativeAddressSelf(0x3, 0x7).RCast<void*>();
//#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
//		off_141731448 = p_BuildPropStaticFrustumCullMap.Offset(0x200).FindPattern("48 ?? ?? ?? ?? ?? 01").ResolveRelativeAddressSelf(0x3, 0x7).RCast<void*>();
//#endif
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VBspLib);
