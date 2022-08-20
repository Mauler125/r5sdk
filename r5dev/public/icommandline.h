#ifndef ICOMMANDLINE_H
#define ICOMMANDLINE_H

abstract_class ICommandLine
{
public:
	virtual void CreateCmdLine(const char* pszCommandline) = 0;
	virtual void CreateCmdLine(int argc, char** argv) = 0;
	virtual void CreatePool(void* pMem) = 0;

	virtual const char* GetCmdLine(void) const = 0;

	virtual const char* CheckParm(const char* psz, const char** ppszValue = NULL) const = 0;
	virtual void RemoveParm(const char* pszParm) = 0;
	virtual void AppendParm(const char* pszParm, const char* pszValues) = 0;

	virtual const char* ParmValue(const char* psz, const char* pDefaultVal = NULL) const = 0;
	virtual int ParmValue(const char* psz, int nDefaultVal) const = 0;
	virtual float ParmValue(const char* psz, float flDefaultVal) const = 0;

	virtual int ParmCount(void) const = 0;
	virtual int FindParm(const char* psz) const = 0;
	virtual const char* GetParm(int nIndex) const = 0;
	virtual bool GuardLocked(void) const = 0; // True = mutex locked.
	virtual void SetParm(int nIndex, char const* pParm) = 0;

	virtual void CleanUpParms(void) = 0;
};

#endif // ICOMMANDLINE_H
