#pragma once
#include "IConVar.h"

namespace
{
	/* ==== CONCOMMANDCALLBACK ============================================================================================================================================== */
#if defined (GAMEDLL_S1)
	ADDRESS p_Map_Callback = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x89\x5C\x24\x18\x55\x41\x56\x41\x00\x00\x00\x00\x40\x02", "xxxxxxxxx????xx");
	void (*Map_Callback)(CCommand* cmd, char a2) = (void (*)(CCommand*, char))p_Map_Callback.GetPtr(); /*48 89 5C 24 18 55 41 56 41 ?? ?? ?? ?? 40 02*/
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	ADDRESS p_Map_Callback = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x40\x55\x41\x56\x41\x57\x48\x81\xEC\x00\x00\x00\x00\x83\x3D", "xxxxxxxxx????xx");
	void (*Map_Callback)(CCommand* cmd, char a2) = (void (*)(CCommand*, char))p_Map_Callback.GetPtr(); /*40 55 41 56 41 57 48 81 EC ?? ?? ?? ?? 83 3D*/
#endif
	ADDRESS p_DownloadPlaylists_Callback = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x33\xC9\xC6\x05\x00\x00\x00\x00\x00\xE9\x00\x00\x00\x00", "xxxx?????x????");
	void (*DownloadPlaylists_Callback)() = (void(*)())p_DownloadPlaylists_Callback.GetPtr(); /*33 C9 C6 05 ?? ?? ?? ?? ?? E9 ?? ?? ?? ??*/
}

///////////////////////////////////////////////////////////////////////////////
#ifndef DEDICATED
void _CGameConsole_f_CompletionFunc(const CCommand& cmd);
void _CCompanion_f_CompletionFunc(const CCommand& cmd);
#endif // !DEDICATED
void _Kick_f_CompletionFunc(CCommand* cmd);
void _KickID_f_CompletionFunc(CCommand* cmd);
void _Ban_f_CompletionFunc(CCommand* cmd);
void _BanID_f_CompletionFunc(CCommand* cmd);
void _Unban_f_CompletionFunc(CCommand* cmd);
void _ReloadBanList_f_CompletionFunc(CCommand* cmd);
void _RTech_GenerateGUID_f_CompletionFunc(CCommand* cmd);
void _RTech_Decompress_f_CompletionFunc(CCommand* cmd);
void _VPK_Decompress_f_CompletionFunc(CCommand* cmd);
void _NET_TraceNetChan_f_CompletionFunc(CCommand* cmd);
void _NET_SetKey_f_CompletionFunc(CCommand* cmd);
void _NET_GenerateKey_f_CompletionFunc(CCommand* cmd);


///////////////////////////////////////////////////////////////////////////////
class HCompletion : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: Map_Callback                         : 0x" << std::hex << std::uppercase << p_Map_Callback.GetPtr()               << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: DownloadPlaylists_Callback           : 0x" << std::hex << std::uppercase << p_DownloadPlaylists_Callback.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HCompletion);
