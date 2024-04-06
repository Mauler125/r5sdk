#ifndef LAUNCHER_H
#define LAUNCHER_H

// NOTE: cannot hook this! this function is hooked by loader.dll and is also
// used to gracefully shutdown the SDK.
inline int(*v_LauncherMain)(HINSTANCE hInstance);

inline LONG(*v_TopLevelExceptionFilter)(EXCEPTION_POINTERS* pExceptionPointer);
inline void(*v_RemoveSpuriousGameParameters)(void);

///////////////////////////////////////////////////////////////////////////////
class VLauncher : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("LauncherMain", v_LauncherMain);
		LogFunAdr("TopLevelExceptionFilter", v_TopLevelExceptionFilter);
		LogFunAdr("RemoveSpuriousGameParameters", v_RemoveSpuriousGameParameters);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 8B C8 E8 ?? ?? ?? ?? CC").FollowNearCallSelf().GetPtr(v_LauncherMain);
		g_GameDll.FindPatternSIMD("40 53 48 83 EC 20 48 8B 05 ?? ?? ?? ?? 48 8B D9 48 85 C0 74 06").GetPtr(v_TopLevelExceptionFilter);
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 81 EC ?? ?? ?? ?? 33 ED 48 8D 3D ?? ?? ?? ??").GetPtr(v_RemoveSpuriousGameParameters);
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////

#endif // LAUNCHER_H