#pragma once
#include "tier1/IConVar.h"

inline CMemory p_SetupGamemode;
inline auto SetupGamemode = p_SetupGamemode.RCast<bool(*)(const char* pszPlayList)>();

/* ==== CONCOMMANDCALLBACK ============================================================================================================================================== */
inline CMemory p_DownloadPlaylists_f;
inline auto _DownloadPlaylists_f = p_DownloadPlaylists_f.RCast<void(*)(void)>();

///////////////////////////////////////////////////////////////////////////////
void MP_GameMode_Changed_f(IConVar* pConVar, const char* pOldString, float flOldValue);
void MP_HostName_Changed_f(IConVar* pConVar, const char* pOldString, float flOldValue);
#ifndef DEDICATED
void ToggleConsole_f(const CCommand& args);
void ToggleBrowser_f(const CCommand& args);
#endif // !DEDICATED
#ifndef CLIENT_DLL
void Host_Kick_f(const CCommand& args);
void Host_KickID_f(const CCommand& args);
void Host_Ban_f(const CCommand& args);
void Host_BanID_f(const CCommand& args);
void Host_Unban_f(const CCommand& args);
void Host_ReloadBanList_f(const CCommand& args);
void Host_ReloadPlaylists_f(const CCommand& args);
void Host_Changelevel_f(const CCommand& args);
void Detour_HotSwap_f(const CCommand& args);
#endif // !CLIENT_DLL
void Pak_ListPaks_f(const CCommand& args);
void Pak_RequestUnload_f(const CCommand& args);
void Pak_RequestLoad_f(const CCommand& args);
void Pak_Swap_f(const CCommand& args);
void RTech_StringToGUID_f(const CCommand& args);
void RTech_Decompress_f(const CCommand& args);
void VPK_Pack_f(const CCommand& args);
void VPK_Unpack_f(const CCommand& args);
void VPK_Mount_f(const CCommand& args);
void VPK_Unmount_f(const CCommand& args);
void NET_SetKey_f(const CCommand& args);
void NET_GenerateKey_f(const CCommand& args);
void NET_UseRandomKeyChanged_f(IConVar* pConVar, const char* pOldString, float flOldValue);
void CON_Help_f(const CCommand& args);
#ifndef DEDICATED
void CON_LogHistory_f(const CCommand& args);
void CON_RemoveLine_f(const CCommand& args);
void CON_ClearLines_f(const CCommand& args);
void CON_ClearHistory_f(const CCommand& args);

void RCON_CmdQuery_f(const CCommand& args);
void RCON_Disconnect_f(const CCommand& args);
#endif // !DEDICATED
void RCON_PasswordChanged_f(IConVar* pConVar, const char* pOldString, float flOldValue);
#ifndef CLIENT_DLL
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
#if !defined (GAMEDLL_S0) && !defined (GAMEDLL_S1)
void BHit_f(const CCommand& args);
#endif // !GAMEDLL_S0 && !GAMEDLL_S1

void CVHelp_f(const CCommand& args);
void CVList_f(const CCommand& args);
void CVDiff_f(const CCommand& args);
void CVFlag_f(const CCommand& args);
///////////////////////////////////////////////////////////////////////////////
class VCallback : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: SetupGamemode                        : {:#18x} |\n", p_SetupGamemode.GetPtr());
		spdlog::debug("| FUN: DownloadPlaylist_f                   : {:#18x} |\n", p_DownloadPlaylists_f.GetPtr());
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
		p_SetupGamemode = g_GameDll.FindPatternSIMD("40 53 48 83 EC 20 48 8B D9 48 C7 C0 ?? ?? ?? ??");
		p_DownloadPlaylists_f = g_GameDll.FindPatternSIMD("33 C9 C6 05 ?? ?? ?? ?? ?? E9 ?? ?? ?? ??");

		SetupGamemode = p_SetupGamemode.RCast<bool(*)(const char*)>();       /*40 53 48 83 EC 20 48 8B D9 48 C7 C0 ?? ?? ?? ??*/
		_DownloadPlaylists_f = p_DownloadPlaylists_f.RCast<void(*)(void)>(); /*33 C9 C6 05 ?? ?? ?? ?? ?? E9 ?? ?? ?? ??*/
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VCallback);
