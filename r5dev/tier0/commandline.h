#pragma once

class CCommandLine // VTABLE @0x141369C78 in R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM
{
public:
	void CreateCmdLine(const char* pszCommandline);
	void CreateCmdLine(int argc, char** argv);
	void CreatePool(void* pMem);
	const char* GetCmdLine(void);
	const char* CheckParm(const char* psz, const char** ppszValue = NULL);
	void RemoveParm(const char* pszParm);
	void AppendParm(const char* pszParm, const char* pszValues);
	const char* ParmValue(const char* psz, const char* pDefaultVal = NULL);
	int ParmValue(const char* psz, int nDefaultVal);
	float ParmValue(const char* psz, float flDefaultVal);
	int ParmCount(void);
	int FindParm(const char* psz);
	const char* GetParm(int nIndex);
	void SetParm(int nIndex, char const* pParm);
};
CCommandLine* CommandLine(void);

///////////////////////////////////////////////////////////////////////////////
class HCommandLine : public IDetour
{
	virtual void GetAdr(void) const
	{
		std::cout << "| VAR: g_pCmdLine                           : 0x" << std::hex << std::uppercase << CommandLine() << std::setw(0) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HCommandLine);
