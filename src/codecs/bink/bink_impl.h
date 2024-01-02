#pragma once

inline void*(*v_BinkOpen)(HANDLE hBinkFile, UINT32 nFlags);
inline void(*v_BinkClose)(HANDLE hBinkFile);
inline const char*(*v_BinkGetError)(void);

///////////////////////////////////////////////////////////////////////////////
class BinkCore : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("BinkOpen", v_BinkOpen);
		LogFunAdr("BinkClose", v_BinkClose);
		LogFunAdr("BinkGetError", v_BinkGetError);
	}
	virtual void GetFun(void) const
	{
		g_RadVideoToolsDll.GetExportedSymbol("BinkOpen").GetPtr(v_BinkOpen);
		g_RadVideoToolsDll.GetExportedSymbol("BinkClose").GetPtr(v_BinkClose);
		g_RadVideoToolsDll.GetExportedSymbol("BinkGetError").GetPtr(v_BinkGetError);
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
