#pragma once
#include "public/icommandline.h"

class CCommandLine : public ICommandLine // VTABLE @0x141369C78 in R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM
{
private:
	enum
	{
		MAX_PARAMETER_LEN = 128,
		MAX_PARAMETERS = 256,
	};

	char* m_pszCmdLine;
	char m_Pad[0x18]; // <-- thread/mutex stuff.
	int m_nParmCount;
	char* m_ppParms[MAX_PARAMETERS];
};

extern CCommandLine* g_pCmdLine;
CCommandLine* CommandLine(void);

///////////////////////////////////////////////////////////////////////////////
class VCommandLine : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| VAR: g_pCmdLine                           : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_pCmdLine));
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const
	{
		g_pCmdLine = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>(
			"\x40\x55\x48\x83\xEC\x20\x48\x8D\x6C\x24\x00\x48\x89\x5D\x10\x49\xC7\xC0\x00\x00\x00\x00"),
			"xxxxxxxxxx?xxxxxxx????").FindPatternSelf("48 8D 0D", CMemory::Direction::DOWN, 250).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CCommandLine*>();
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VCommandLine);
