#pragma once

/* ==== RUI ====================================================================================================================================================== */
inline CMemory p_Rui_Draw;
inline auto v_Rui_Draw = p_Rui_Draw.RCast<bool(*)(__int64* a1, __m128* a2, const __m128i* a3, __int64 a4, __m128* a5)>();

inline CMemory p_Rui_LoadAsset;
inline auto v_Rui_LoadAsset = p_Rui_LoadAsset.RCast<void* (*)(const char* szRuiAssetName)>();

inline CMemory p_Rui_GetFontFace;
inline auto v_Rui_GetFontFace = p_Rui_GetFontFace.RCast<int16_t (*)(void)>();

void Rui_Attach();
void Rui_Detach();

///////////////////////////////////////////////////////////////////////////////
class VRui : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: Rui_Draw                             : {:#18x} |\n", p_Rui_Draw.GetPtr());
		spdlog::debug("| FUN: Rui_LoadAsset                        : {:#18x} |\n", p_Rui_LoadAsset.GetPtr());
		spdlog::debug("| FUN: Rui_GetFontFace                      : {:#18x} |\n", p_Rui_GetFontFace.GetPtr());
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
		p_Rui_Draw = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x40\x4C\x8B\x5A\x18"), "xxxxxxxxxx");
		v_Rui_Draw = p_Rui_Draw.RCast<bool(*)(__int64*, __m128*, const __m128i*, __int64, __m128*)>(); /* 40 53 48 83 EC 40 4C 8B 5A 18 */

		p_Rui_LoadAsset = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\xE8\x00\x00\x00\x00\xEB\x03\x49\x8B\xC6\x48\x89\x86\x00\x00\x00\x00\x8B\x86\x00\x00\x00\x00"), "x????xxxxxxxx????xx????").FollowNearCallSelf();
		v_Rui_LoadAsset = p_Rui_LoadAsset.RCast<void* (*)(const char*)>(); /*E8 ?? ?? ?? ?? EB 03 49 8B C6 48 89 86 ?? ?? ?? ?? 8B 86 ?? ?? ?? ??*/

		p_Rui_GetFontFace = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\xF7\x05\x00\x00\x00\x00\x00\x00\x00\x00\x4C\x8D\x0D\x00\x00\x00\x00\x74\x05\x49\x8B\xD1\xEB\x19\x48\x8B\x05\x00\x00\x00\x00\x48\x8D\x15\x00\x00\x00\x00\x48\x8B\x48\x58\x48\x85\xC9\x48\x0F\x45\xD1\xF7\x05\x00\x00\x00\x00\x00\x00\x00\x00\x75\x19\x48\x8B\x05\x00\x00\x00\x00\x4C\x8D\x0D\x00\x00\x00\x00\x4C\x8B\x40\x58\x4D\x85\xC0\x4D\x0F\x45\xC8\x49\x8B\xC9\x48\xFF\x25\x00\x00\x00\x00"), "xx????????xxx????xxxxxxxxxx????xxx????xxxxxxxxxxxxx????????xxxxx????xxx????xxxxxxxxxxxxxxxxx????");;
		v_Rui_GetFontFace = p_Rui_GetFontFace.RCast<int16_t(*)(void)>();/*F7 05 ? ? ? ? ? ? ? ? 4C 8D 0D ? ? ? ? 74 05 49 8B D1 EB 19 48 8B 05 ? ? ? ? 48 8D 15 ? ? ? ? 48 8B 48 58 48 85 C9 48 0F 45 D1 F7 05 ? ? ? ? ? ? ? ? 75 19 48 8B 05 ? ? ? ? 4C 8D 0D ? ? ? ? 4C 8B 40 58 4D 85 C0 4D 0F 45 C8 49 8B C9 48 FF 25 ? ? ? ?*/
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VRui);
