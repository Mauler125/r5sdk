#pragma once
#include "tier1/IConVar.h"

inline CMemory p_SetupGamemode = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x20\x48\x8B\xD9\x48\xC7\xC0\x00\x00\x00\x00"), "xxxxxxxxxxxx????");
inline auto SetupGamemode = p_SetupGamemode.RCast<bool(*)(const char* pszPlayList)>();

bool MP_GameMode_Changed_f(ConVar* pVTable);
///////////////////////////////////////////////////////////////////////////////
class HCallback : public IDetour
{
	virtual void GetAdr(void) const
	{
		std::cout << "| FUN: SetupGamemode                        : 0x" << std::hex << std::uppercase << p_SetupGamemode.GetPtr() << std::setw(nPad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HCallback);
