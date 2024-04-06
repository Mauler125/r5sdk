#ifndef RTECH_STRYDER_H
#define RTECH_STRYDER_H

/* ==== STRYDER ================================================================================================================================================ */
inline void*(*v_Stryder_StitchRequest)(void* a1);
inline bool(*v_Stryder_SendOfflineRequest)(void);

///////////////////////////////////////////////////////////////////////////////
class VStryder : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("Stryder_StitchRequest", v_Stryder_StitchRequest);
		LogFunAdr("Stryder_SendOfflineRequest", v_Stryder_SendOfflineRequest);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 20 48 8B F9 E8 B4").GetPtr(v_Stryder_StitchRequest);
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 55 57 41 56 48 8D AC 24 ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 48 8B 35 ?? ?? ?? ??").GetPtr(v_Stryder_SendOfflineRequest);
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const { }
};
///////////////////////////////////////////////////////////////////////////////

#endif // RTECH_STRYDER_H
