#pragma once

inline CMemory p_BinkOpen;
inline auto v_BinkOpen = p_BinkOpen.RCast<void*(*)(HANDLE hBinkFile, UINT32 nFlags)>();

inline CMemory p_BinkClose;
inline auto v_BinkClose = p_BinkClose.RCast<void(*)(HANDLE hBinkFile)>();

inline CMemory p_BinkGetError;
inline auto v_BinkGetError = p_BinkGetError.RCast<const char*(*)(void)>();

void BinkImpl_Attach();
void BinkImpl_Detach();

///////////////////////////////////////////////////////////////////////////////
class BinkCore : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: BinkOpen                             : {:#18x} |\n", p_BinkOpen.GetPtr());
		spdlog::debug("| FUN: BinkClose                            : {:#18x} |\n", p_BinkClose.GetPtr());
		spdlog::debug("| FUN: BinkGetError                         : {:#18x} |\n", p_BinkGetError.GetPtr());
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
		p_BinkOpen = g_RadVideoToolsDll.GetExportedFunction("BinkOpen");
		v_BinkOpen = p_BinkOpen.RCast<void*(*)(HANDLE, UINT32)>();
		p_BinkClose = g_RadVideoToolsDll.GetExportedFunction("BinkClose");
		v_BinkClose = p_BinkClose.RCast<void(*)(HANDLE)>();
		p_BinkGetError = g_RadVideoToolsDll.GetExportedFunction("BinkGetError");
		v_BinkGetError = p_BinkGetError.RCast<const char* (*)(void)>();
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(BinkCore);
