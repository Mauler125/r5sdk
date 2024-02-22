#pragma once

inline bool(*v_SetupGamemode)(const char* pszPlayList);

/* ==== CONCOMMANDCALLBACK ============================================================================================================================================== */
inline void(*v__Cmd_Exec_f)(const CCommand& args);

///////////////////////////////////////////////////////////////////////////////
void MP_GameMode_Changed_f(IConVar* pConVar, const char* pOldString, float flOldValue);
void MP_HostName_Changed_f(IConVar* pConVar, const char* pOldString, float flOldValue);
#ifndef CLIENT_DLL
void Host_Changelevel_f(const CCommand& args);
#endif // !CLIENT_DLL
void VPK_Pack_f(const CCommand& args);
void VPK_Unpack_f(const CCommand& args);
void VPK_Mount_f(const CCommand& args);
void VPK_Unmount_f(const CCommand& args);
void NET_SetKey_f(const CCommand& args);
void NET_GenerateKey_f(const CCommand& args);
void NET_UseRandomKeyChanged_f(IConVar* pConVar, const char* pOldString, float flOldValue);
void NET_UseSocketsForLoopbackChanged_f(IConVar* pConVar, const char* pOldString, float flOldValue);
#ifndef DEDICATED

void RCON_InputOnlyChanged_f(IConVar* pConVar, const char* pOldString, float flOldValue);

void GFX_NVN_Changed_f(IConVar* pConVar, const char* pOldString, float flOldValue);
#endif // !DEDICATED
void RCON_PasswordChanged_f(IConVar* pConVar, const char* pOldString, float flOldValue);
void LanguageChanged_f(IConVar* pConVar, const char* pOldString, float flOldValue);
#ifndef CLIENT_DLL
void RCON_WhiteListAddresChanged_f(IConVar* pConVar, const char* pOldString, float flOldValue);
void RCON_ConnectionCountChanged_f(IConVar* pConVar, const char* pOldString, float flOldValue);
void SQVM_ServerScript_f(const CCommand& args);
#endif // !CLIENT_DLL
#ifndef DEDICATED
void SQVM_ClientScript_f(const CCommand& args);
void SQVM_UIScript_f(const CCommand& args);
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
#ifndef CLIENT_DLL
void CC_CreateFakePlayer_f(const CCommand& args);
#endif // !CLIENT_DLL
///////////////////////////////////////////////////////////////////////////////
class VCallback : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("SetupGamemode", v_SetupGamemode);
		LogFunAdr("Cmd_Exec_f", v__Cmd_Exec_f);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("40 53 48 83 EC 20 48 8B D9 48 C7 C0 ?? ?? ?? ??").GetPtr(v_SetupGamemode);
		g_GameDll.FindPatternSIMD("40 55 53 48 8D AC 24 ?? ?? ?? ?? B8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 2B E0 48 8B D9").GetPtr(v__Cmd_Exec_f);
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
