#pragma once
#include "core/stdafx.h"
#include "inputsystem/ButtonCode.h"

class CInputSystem
{
public:
	void EnableInput(bool bEnabled);        // @0x14039F100 in R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM
	void EnableMessagePump(bool bEnabled);  // @0x14039F110 in R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM
	bool IsButtonDown(ButtonCode_t Button); // @0x1403A0140 in R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM

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
		spdlog::debug("| VAR: g_pInputSystem                       : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_pInputSystem));
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const
	{
		g_pInputSystem = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>(
			"\x48\x83\xEC\x28\x48\x8B\x0D\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x05\x00\x00\x00\x00\x48\x85\xC9\x74\x11"),
			"xxxxxxx????xxx????xxx????xxxxx").FindPatternSelf("48 89 05", CMemory::Direction::DOWN, 40).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CInputSystem*>();
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VInputSystem);
