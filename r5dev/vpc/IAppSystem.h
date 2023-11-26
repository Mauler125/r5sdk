#pragma once

/* ==== IAPPSYSTEM ============================================================================================================================================== */
//inline CMemory p_IAppSystem_LoadLibrary; // C initializers/terminators
//inline void*(*IAppSystem_LoadLibrary)(void);

///////////////////////////////////////////////////////////////////////////////
class VAppSystem : public IDetour
{
	virtual void GetAdr(void) const
	{
		//LogFunAdr("CAppSystem::LoadLibrary", p_IAppSystem_LoadLibrary.GetPtr());
	}
	virtual void GetFun(void) const
	{
		//p_IAppSystem_LoadLibrary = g_GameDll.FindPatternSIMD("48 83 EC 28 48 8B 0D ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ?? 48 89 05 ?? ?? ?? ?? 48 85 C9 74 11");
		//IAppSystem_LoadLibrary = p_IAppSystem_LoadLibrary.RCast<void* (*)(void)>(); /*48 83 EC 28 48 8B 0D ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ?? 48 89 05 ?? ?? ?? ?? 48 85 C9 74 11*/
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const { }
};
///////////////////////////////////////////////////////////////////////////////
