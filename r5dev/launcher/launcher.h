#ifndef LAUNCHER_H
#define LAUNCHER_H

inline CMemory p_WinMain;
inline auto v_WinMain = p_WinMain.RCast<int (*)(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)>();

inline CMemory p_LauncherMain;
inline auto v_LauncherMain = p_LauncherMain.RCast<int(*)(HINSTANCE hInstance)>();

inline CMemory p_TopLevelExceptionFilter;
inline auto v_TopLevelExceptionFilter = p_TopLevelExceptionFilter.RCast<LONG(*)(EXCEPTION_POINTERS* pExceptionPointer)>();

#if !defined (GAMEDLL_S0) && !defined (GAMEDLL_S1)
inline CMemory p_RemoveSpuriousGameParameters;
inline auto v_RemoveSpuriousGameParameters = p_RemoveSpuriousGameParameters.RCast<void* (*)(void)>();
#endif // !GAMEDLL_S0 || !GAMEDLL_S1

void AppendSDKParametersPreInit();
string LoadConfigFile(const char* svConfig);
void ParseAndApplyConfigFile(const string& svConfig);
const char* ExitCodeToString(int nCode);

///////////////////////////////////////////////////////////////////////////////
class VLauncher : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("WinMain", p_WinMain.GetPtr());
		LogFunAdr("LauncherMain", p_LauncherMain.GetPtr());
		LogFunAdr("TopLevelExceptionFilter", p_TopLevelExceptionFilter.GetPtr());
#if !defined (GAMEDLL_S0) && !defined (GAMEDLL_S1)
		LogFunAdr("RemoveSpuriousGameParameters", p_RemoveSpuriousGameParameters.GetPtr());
#endif // !GAMEDLL_S0 || !GAMEDLL_S1
	}
	virtual void GetFun(void) const
	{
		p_WinMain = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 41 8B D9 49 8B F8");
		v_WinMain = p_WinMain.RCast<int (*)(HINSTANCE, HINSTANCE, LPSTR, int)>();

		p_LauncherMain = g_GameDll.GetExportedFunction("LauncherMain");
		v_LauncherMain = p_LauncherMain.RCast<int(*)(HINSTANCE)>();

		p_TopLevelExceptionFilter = g_GameDll.FindPatternSIMD("40 53 48 83 EC 20 48 8B 05 ?? ?? ?? ?? 48 8B D9 48 85 C0 74 06");
		v_TopLevelExceptionFilter = p_TopLevelExceptionFilter.RCast<LONG(*)(EXCEPTION_POINTERS*)>();

#if !defined (GAMEDLL_S0) && !defined (GAMEDLL_S1)
		p_RemoveSpuriousGameParameters = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 81 EC ?? ?? ?? ?? 33 ED 48 8D 3D ?? ?? ?? ??");
		v_RemoveSpuriousGameParameters = p_RemoveSpuriousGameParameters.RCast<void* (*)(void)>();
#endif // !GAMEDLL_S0 || !GAMEDLL_S1
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const;
	virtual void Detach(void) const;
};
///////////////////////////////////////////////////////////////////////////////

#endif // LAUNCHER_H