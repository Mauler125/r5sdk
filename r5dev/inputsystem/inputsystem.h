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
extern CInputSystem* g_pInputSystem
;
///////////////////////////////////////////////////////////////////////////////
class HInputSystem : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| VAR: g_pInputSystem                       : 0x" << std::hex << std::uppercase << g_pInputSystem << std::setw(0) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HInputSystem);
