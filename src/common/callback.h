#pragma once

inline bool(*v_SetupGamemode)(const char* pszPlayList);

/* ==== CONCOMMANDCALLBACK ============================================================================================================================================== */
inline void(*v__Cmd_Exec_f)(const CCommand& args);
inline void(*v__setClassVarServer_f)(const CCommand& args);
inline void(*v__setClassVarClient_f)(const CCommand& args);
#ifndef DEDICATED
inline void(*v__UIScript_Reset_f)();
#endif // !DEDICATED

#ifndef CLIENT_DLL
inline int* g_nCommandClientIndex = nullptr;
#endif // !CLIENT_DLL

///////////////////////////////////////////////////////////////////////////////
void MP_GameMode_Changed_f(IConVar* pConVar, const char* pOldString, float flOldValue, ChangeUserData_t pUserData);
#ifndef CLIENT_DLL
void Host_Changelevel_f(const CCommand& args);
#endif // !CLIENT_DLL
void VPK_Pack_f(const CCommand& args);
void VPK_Unpack_f(const CCommand& args);
void VPK_Mount_f(const CCommand& args);
void VPK_Unmount_f(const CCommand& args);
#ifndef DEDICATED

void GFX_NVN_Changed_f(IConVar* pConVar, const char* pOldString, float flOldValue, ChangeUserData_t pUserData);
#endif // !DEDICATED
void LanguageChanged_f(IConVar* pConVar, const char* pOldString, float flOldValue, ChangeUserData_t pUserData);
#ifndef DEDICATED
void Set_f(const CCommand& args);
void Mat_CrossHair_f(const CCommand& args);
void Line_f(const CCommand& args);
void Sphere_f(const CCommand& args);
void Capsule_f(const CCommand& args);
#endif // !DEDICATED
void BHit_f(const CCommand& args);

void CVHelp_f(const CCommand& args);
void CVList_f(const CCommand& args);
void CVDiff_f(const CCommand& args);
void CVFlag_f(const CCommand& args);

///////////////////////////////////////////////////////////////////////////////
class VCallback : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("SetupGamemode", v_SetupGamemode);
		LogFunAdr("Cmd_Exec_f", v__Cmd_Exec_f);
		LogFunAdr("SetClassVarServer_f", v__setClassVarServer_f);
		LogFunAdr("SetClassVarClient_f", v__setClassVarClient_f);
#ifndef DEDICATED
		LogFunAdr("UIScript_Reset_f", v__UIScript_Reset_f);
#endif // !DEDICATED
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("40 53 48 83 EC 20 48 8B D9 48 C7 C0 ?? ?? ?? ??").GetPtr(v_SetupGamemode);
		g_GameDll.FindPatternSIMD("40 55 53 48 8D AC 24 ?? ?? ?? ?? B8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 2B E0 48 8B D9").GetPtr(v__Cmd_Exec_f);
		g_GameDll.FindPatternSIMD("41 56 48 83 EC ?? 8B 05 ?? ?? ?? ?? 4C 8B F1 FF C0").GetPtr(v__setClassVarServer_f);
		g_GameDll.FindPatternSIMD("4C 8B DC 57 48 81 EC ?? ?? ?? ?? 8B 05").GetPtr(v__setClassVarClient_f);
#ifndef DEDICATED
		g_GameDll.FindPatternSIMD("40 55 41 54 48 8D AC 24 ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 45 33 E4 48 8D 0D").GetPtr(v__UIScript_Reset_f);
#endif // !DEDICATED
	}
	virtual void GetVar(void) const
	{
#ifndef CLIENT_DLL
		CMemory(v__setClassVarServer_f).FindPatternSelf("8B 05").ResolveRelativeAddressSelf(2, 6).GetPtr(g_nCommandClientIndex);
#endif // !CLIENT_DLL
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
