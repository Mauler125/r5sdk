#pragma once

inline CMemory p_BinkOpen;
inline void*(*v_BinkOpen)(HANDLE hBinkFile, UINT32 nFlags);

inline CMemory p_BinkClose;
inline void(*v_BinkClose)(HANDLE hBinkFile);

inline CMemory p_BinkGetError;
inline const char*(*v_BinkGetError)(void);

///////////////////////////////////////////////////////////////////////////////
class BinkCore : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("BinkOpen", p_BinkOpen.GetPtr());
		LogFunAdr("BinkClose", p_BinkClose.GetPtr());
		LogFunAdr("BinkGetError", p_BinkGetError.GetPtr());
	}
	virtual void GetFun(void) const
	{
		p_BinkOpen = g_RadVideoToolsDll.GetExportedSymbol("BinkOpen");
		v_BinkOpen = p_BinkOpen.RCast<void*(*)(HANDLE, UINT32)>();
		p_BinkClose = g_RadVideoToolsDll.GetExportedSymbol("BinkClose");
		v_BinkClose = p_BinkClose.RCast<void(*)(HANDLE)>();
		p_BinkGetError = g_RadVideoToolsDll.GetExportedSymbol("BinkGetError");
		v_BinkGetError = p_BinkGetError.RCast<const char* (*)(void)>();
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////

