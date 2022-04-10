#pragma once

namespace
{
	//static auto g_CModelLoader = CMemory(0x14173B210).RCast<void*>();

	//static auto dword_1696A9D20 = CMemory(0x14D40B328).RCast<std::uint32_t>();
	//static auto dword_141744EE8 = CMemory(0x141744EE8).RCast<std::uint32_t>();
	//static auto dword_141744EBC = CMemory(0x141744EBC).RCast<std::uint32_t>();

	//static auto qword_14D40B328 = CMemory(0x14D40B328).RCast<std::uint32_t>();
	//static auto qword_141744EA8 = CMemory(0x141744EA8).RCast<std::uint32_t>();
	//static auto qword_141744EA0 = CMemory(0x141744EA0).RCast<std::uint32_t>();
	//static auto qword_141744E88 = CMemory(0x141744E88).RCast<std::uint32_t>();

	//static auto off_141744E70 = CMemory(0x141744E70).RCast<void**>();
	//static auto off_141731448 = CMemory(0x141744EA8).RCast<void**>();

	//__m128 xmmword_1415BD270 = _mm_castsi128_ps(_mm_set_epi32(0x3B808081, 0x3B808081, 0x3B808081, 0x3B808081)); // xmmword_1415BD270

	//static auto sub_1404365A0 = CMemory(0x1404365A0).RCast<void** (*)(__m128*, long long, long long, double)>(); // Prototype is most likely incorrect: 'local variable allocation has failed, the output may be wrong!'
	//static auto sub_140270130 = CMemory(0x140270130).RCast<__m128 (*)(__m128*)>();
	//static auto sub_14028F170 = CMemory(0x14028F170).RCast<const __m128i* (*)(__int64, __int64, __m128*, const __m128i*, const __m128i*)>();

	CMemory p_CalcPropStaticFrustumCulling = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\xC4\x44\x89\x40\x18\x48\x89\x50\x10\x55"), "xxxxxxxxxxxx");
	__int64 (*CalcPropStaticFrustumCulling)(__int64 a1, __int64 a2, unsigned int a3, unsigned int a4, __int64 a5, __int64 a6, __int64 a7) = (__int64 (*)(__int64 a1, __int64 a2, unsigned int a3, unsigned int a4, __int64 a5, __int64 a6, __int64 a7))p_CalcPropStaticFrustumCulling.GetPtr(); /*48 8B C4 44 89 40 18 48 89 50 10 55*/
}

__int64 __fastcall HCalcPropStaticFrustumCulling(__int64 a1, __int64 a2, unsigned int a3, unsigned int a4, __int64 a5, __int64 a6, __int64 a7);

void BspLib_Attach();
void BspLib_Detach();

///////////////////////////////////////////////////////////////////////////////
class HBspLib : public IDetour
{
	virtual void GetAdr(void) const
	{
		std::cout << "| FUN: CalcPropStaticFrustumCulling         : 0x" << std::hex << std::uppercase << p_CalcPropStaticFrustumCulling.GetPtr() << std::setw(nPad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HBspLib);
