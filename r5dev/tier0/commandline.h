#pragma once
#include "public/icommandline.h"

class CCommandLine : public ICommandLine // VTABLE @0x141369C78 in R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM
{
public:
	static void StaticCreateCmdLine(CCommandLine* thisptr, const char* pszCommandLine);
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

extern bool g_bCommandLineCreated;
extern CCommandLine* g_pCmdLine;
//-----------------------------------------------------------------------------
// Instance singleton and expose interface to rest of code
//-----------------------------------------------------------------------------
inline CCommandLine* CommandLine(void)
{
	return g_pCmdLine;
}

inline void(*CCommandLine__CreateCmdLine)(CCommandLine* thisptr, const char* pszCommandLine);

///////////////////////////////////////////////////////////////////////////////
class VCommandLine : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CCommandLine::CreateCmdLine", CCommandLine__CreateCmdLine);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 89 54 24 ?? 48 89 4C 24 ?? 53 41 55 B8 ?? ?? ?? ??").GetPtr(CCommandLine__CreateCmdLine);
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
