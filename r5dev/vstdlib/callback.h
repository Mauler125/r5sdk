#pragma once
#include "tier1/IConVar.h"

inline CMemory p_SetupGamemode;
inline auto SetupGamemode = p_SetupGamemode.RCast<bool(*)(const char* pszPlayList)>();

/* ==== CONCOMMANDCALLBACK ============================================================================================================================================== */
inline CMemory p_Host_Map_f;
inline auto _Host_Map_f = p_Host_Map_f.RCast<void (*)(CCommand* pCommand, char a2)>();

inline CMemory p_DownloadPlaylists_f;
inline auto _DownloadPlaylists_f = p_DownloadPlaylists_f.RCast<void(*)(void)>();

///////////////////////////////////////////////////////////////////////////////
void MP_GameMode_Changed_f(IConVar* pConVar, const char* pOldString, float flOldValue);
#ifndef DEDICATED
void GameConsole_Invoke_f(const CCommand& args);
void ServerBrowser_Invoke_f(const CCommand& args);
#endif // !DEDICATED
#ifndef CLIENT_DLL
void Host_Kick_f(const CCommand& args);
void Host_KickID_f(const CCommand& args);
void Host_Ban_f(const CCommand& args);
void Host_BanID_f(const CCommand& args);
void Host_Unban_f(const CCommand& args);
void Host_ReloadBanList_f(const CCommand& args);
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
void NET_SetKey_f(const CCommand& args);
void NET_GenerateKey_f(const CCommand& args);
#ifndef DEDICATED
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
void BHit_f(const CCommand& args);
///////////////////////////////////////////////////////////////////////////////
class VCallback : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: SetupGamemode                        : {:#18x} |\n", p_SetupGamemode.GetPtr());
		spdlog::debug("| FUN: Host_Map_f                           : {:#18x} |\n", p_Host_Map_f.GetPtr());
		spdlog::debug("| FUN: DownloadPlaylist_f                   : {:#18x} |\n", p_DownloadPlaylists_f.GetPtr());
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
		p_SetupGamemode = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x20\x48\x8B\xD9\x48\xC7\xC0\x00\x00\x00\x00"), "xxxxxxxxxxxx????");
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_Host_Map_f = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x18\x55\x41\x56\x41\x00\x00\x00\x00\x40\x02"), "xxxxxxxxx????xx");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_Host_Map_f = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x55\x41\x56\x41\x57\x48\x81\xEC\x00\x00\x00\x00\x83\x3D"), "xxxxxxxxx????xx");
#endif
		p_DownloadPlaylists_f = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x33\xC9\xC6\x05\x00\x00\x00\x00\x00\xE9\x00\x00\x00\x00"), "xxxx?????x????");

		SetupGamemode = p_SetupGamemode.RCast<bool(*)(const char*)>();
		_Host_Map_f = p_Host_Map_f.RCast<void (*)(CCommand* pCommand, char a2)>(); /*40 55 41 56 41 57 48 81 EC ?? ?? ?? ?? 83 3D*/
		_DownloadPlaylists_f = p_DownloadPlaylists_f.RCast<void(*)(void)>();       /*33 C9 C6 05 ?? ?? ?? ?? ?? E9 ?? ?? ?? ??*/
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VCallback);
