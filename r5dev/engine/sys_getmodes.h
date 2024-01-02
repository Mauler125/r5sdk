#pragma once

//-------------------------------------------------------------------------
// CGAME
//-------------------------------------------------------------------------
inline bool(*CVideoMode_Common__CreateGameWindow)(int* pnRect);
inline HWND(*CVideoMode_Common__CreateWindowClass)(vrect_t* pnRect);

///////////////////////////////////////////////////////////////////////////////
class HVideoMode_Common : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CVideoMode_Common::CreateGameWindow", CVideoMode_Common__CreateGameWindow);
		LogFunAdr("CVideoMode_Common::CreateWindowClass", CVideoMode_Common__CreateWindowClass);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("40 56 57 48 83 EC 28 48 8B F9 E8 ?? ?? ?? ?? 48 8B F0").GetPtr(CVideoMode_Common__CreateGameWindow);
		g_GameDll.FindPatternSIMD("40 55 53 57 48 8D AC 24 ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 48 8B F9 FF 15 ?? ?? ?? ??").GetPtr(CVideoMode_Common__CreateWindowClass);
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
