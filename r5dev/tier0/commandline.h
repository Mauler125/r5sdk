#pragma once
#include "public/icommandline.h"

class CCommandLine : public ICommandLine // VTABLE @0x141369C78 in R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM
{
public:
	void AppendParametersFromFile(const char* const pszConfig);

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
//-----------------------------------------------------------------------------
// Instance singleton and expose interface to rest of code
//-----------------------------------------------------------------------------
inline CCommandLine* CommandLine(void)
{
	return g_pCmdLine;
}

///////////////////////////////////////////////////////////////////////////////
class VCommandLine : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogVarAdr("g_pCmdLine", reinterpret_cast<uintptr_t>(g_pCmdLine));
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const
	{
		g_pCmdLine = g_GameDll.FindPatternSIMD("40 55 48 83 EC 20 48 8D 6C 24 ?? 48 89 5D 10 49 C7 C0 ?? ?? ?? ??")
			.FindPatternSelf("48 8D 0D", CMemory::Direction::DOWN, 250).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CCommandLine*>();
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////
