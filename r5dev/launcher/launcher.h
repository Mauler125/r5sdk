#ifndef LAUNCHER_H
#define LAUNCHER_H

inline CMemory p_WinMain;
inline auto v_WinMain = p_WinMain.RCast<int (*)(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)>();

inline CMemory p_LauncherMain;
inline auto v_LauncherMain = p_LauncherMain.RCast<int(*)(HINSTANCE hInstance)>();

#if !defined (GAMEDLL_S0) && !defined (GAMEDLL_S1)
inline CMemory p_RemoveSpuriousGameParameters;
inline auto v_RemoveSpuriousGameParameters = p_RemoveSpuriousGameParameters.RCast<void* (*)(void)>();
#endif // !GAMEDLL_S0 || !GAMEDLL_S1

void AppendSDKParametersPreInit();
string LoadConfigFile(const string& svConfig);
void ParseAndApplyConfigFile(const string& svConfig);
const char* ExitCodeToString(int nCode);

void Launcher_Attach();
void Launcher_Detach();

///////////////////////////////////////////////////////////////////////////////
class VLauncher : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: WinMain                              : {:#18x} |\n", p_WinMain.GetPtr());
		spdlog::debug("| FUN: LauncherMain                         : {:#18x} |\n", p_LauncherMain.GetPtr());
#if !defined (GAMEDLL_S0) && !defined (GAMEDLL_S1)
		spdlog::debug("| FUN: RemoveSpuriousGameParameters         : {:#18x} |\n", p_RemoveSpuriousGameParameters.GetPtr());
#endif // !GAMEDLL_S0 || !GAMEDLL_S1
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
		p_WinMain = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x20\x41\x8B\xD9\x49\x8B\xF8"), "xxxx?xxxx?xxxx?xxxxxxxxxxx");
		v_WinMain = p_WinMain.RCast<int (*)(HINSTANCE, HINSTANCE, LPSTR, int)>();

		p_LauncherMain = g_GameDll.GetExportedFunction("LauncherMain");
		v_LauncherMain = p_LauncherMain.RCast<int(*)(HINSTANCE)>();

#if !defined (GAMEDLL_S0) && !defined (GAMEDLL_S1)
		p_RemoveSpuriousGameParameters = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x81\xEC\x00\x00\x00\x00\x33\xED\x48\x8D\x3D\x00\x00\x00\x00"), "xxxx?xxxx?xxxx?xxxx????xxxxx????");
		v_RemoveSpuriousGameParameters = p_RemoveSpuriousGameParameters.RCast<void* (*)(void)>();
#endif // !GAMEDLL_S0 || !GAMEDLL_S1
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VLauncher);
#endif // LAUNCHER_H