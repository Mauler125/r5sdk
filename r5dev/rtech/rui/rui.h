#pragma once

#ifndef DEDICATED

/* ==== RUI ====================================================================================================================================================== */
inline ADDRESS p_RuiDraw = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x40\x4C\x8B\x5A\x18"), "xxxxxxxxxx"); /* 40 53 48 83 EC 40 4C 8B 5A 18 */
inline bool (__fastcall* RuiDraw)(__int64* a1, __m128* a2, const __m128i* a3, __int64 a4, __m128* a5) = (bool (__fastcall*)(__int64*, __m128*, const __m128i*, __int64, __m128*))p_RuiDraw.GetPtr();


void Rui_Attach();
void Rui_Detach();

///////////////////////////////////////////////////////////////////////////////
class HRui : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: RuiDraw                              : 0x" << std::hex << std::uppercase << p_RuiDraw.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HRui);

#endif // !DEDICATED