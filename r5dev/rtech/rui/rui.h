#pragma once

/* ==== RUI ====================================================================================================================================================== */
inline CMemory p_RuiDraw;
inline auto v_RuiDraw = p_RuiDraw.RCast<bool(*)(__int64* a1, __m128* a2, const __m128i* a3, __int64 a4, __m128* a5)>();

inline CMemory p_RuiLoadAsset;
inline auto v_RuiLoadAsset = p_RuiLoadAsset.RCast<void* (*)(const char* szRuiAssetName)>();

void Rui_Attach();
void Rui_Detach();

///////////////////////////////////////////////////////////////////////////////
class VRui : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: RuiDraw                              : {:#18x} |\n", p_RuiDraw.GetPtr());
		spdlog::debug("| FUN: RuiLoadAsset                         : {:#18x} |\n", p_RuiLoadAsset.GetPtr());
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
		p_RuiDraw = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x40\x4C\x8B\x5A\x18"), "xxxxxxxxxx");
		v_RuiDraw = p_RuiDraw.RCast<bool(*)(__int64*, __m128*, const __m128i*, __int64, __m128*)>(); /* 40 53 48 83 EC 40 4C 8B 5A 18 */

		p_RuiLoadAsset = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\xE8\x00\x00\x00\x00\xEB\x03\x49\x8B\xC6\x48\x89\x86\x00\x00\x00\x00\x8B\x86\x00\x00\x00\x00"), "x????xxxxxxxx????xx????").FollowNearCallSelf();
		v_RuiLoadAsset = p_RuiLoadAsset.RCast<void* (*)(const char*)>(); /*E8 ?? ?? ?? ?? EB 03 49 8B C6 48 89 86 ?? ?? ?? ?? 8B 86 ?? ?? ?? ??*/
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VRui);
