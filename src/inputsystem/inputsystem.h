#pragma once
#include "core/stdafx.h"
#include "inputsystem/ButtonCode.h"

class CInputSystem
{
public:
	void EnableInput(bool bEnabled);        // @0x14039F100 in R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM
	void EnableMessagePump(bool bEnabled);  // @0x14039F110 in R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM
	bool IsButtonDown(ButtonCode_t Button); // @0x1403A0140 in R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM
	bool ButtonCodeToString(ButtonCode_t Button);
	ButtonCode_t StringToButtonCode(const char* pString);

private:
	char pad_0000[16]; //0x0000
public:
	bool m_bEnabled;     //0x0010 IsInputEnabled variable.
	bool m_bPumpEnabled; //0x0011 EnabledMessagePump variable.
};

///////////////////////////////////////////////////////////////////////////////
extern CInputSystem* g_pInputSystem;

///////////////////////////////////////////////////////////////////////////////
class VInputSystem : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogVarAdr("g_pInputSystem", reinterpret_cast<uintptr_t>(g_pInputSystem));
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const
	{
		g_pInputSystem = g_GameDll.FindPatternSIMD("48 83 EC 28 48 8B 0D ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ?? 48 89 05 ?? ?? ?? ?? 48 85 C9 74 11")
			.FindPatternSelf("48 89 05", CMemory::Direction::DOWN, 40).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CInputSystem*>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const { }
};
///////////////////////////////////////////////////////////////////////////////
