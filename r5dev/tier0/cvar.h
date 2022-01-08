#pragma once
#include "tier0/basetypes.h"
#include "tier0/IConVar.h"

namespace
{
	/* ==== CCVAR =========================================================================================================================================================== */
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	ADDRESS p_CCvar_Disconnect = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x40\x57\x41\x56\x48\x83\xEC\x38\x4C\x8B\x35", "xxxxxxxxxxx");
	void* (*CCvar_Disconnect)() = (void* (*)())p_CCvar_Disconnect.GetPtr(); /*40 57 41 56 48 83 EC 38 4C 8B 35 ? ? ? ?*/
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	ADDRESS p_CCvar_Disconnect = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x83\xEC\x28\x48\x8B\x0D\x00\x00\x00\x00\x48\x85\xC9\x74\x26\x80\x3D\x00\x00\x00\x00\x00\x74\x1D\x48\x8B\x01\x8B\x15\x00\x00\x00\x00\xFF\x50\x58\xC7\x05\x00\x00\x00\x00\x00\x00\x00\x00\xC6\x05\x00\x00\x00\x00\x00\x48\xC7\x05\x00\x00\x00", "xxxxxxx????xxxxxxx?????xxxxxxx????xxxxx????????xx");
	void* (*CCvar_Disconnect)() = (void* (*)())p_CCvar_Disconnect.GetPtr(); /*48 83 EC 28 48 8B 0D ? ? ? ? 48 85 C9 74 26 80 3D ? ? ? ? ? 74 1D 48 8B 01 8B 15 ? ? ? ? FF 50 58 C7 05 ? ? ? ? ? ? ? ? C6 05 ? ? ? ? ? 48 C7 05 ? ? ? ? ? ? ? ?*/
#endif
}

//-------------------------------------------------------------------------
// ENGINE                                                                 |
extern ConVar* cm_debug_cmdquery;
extern ConVar* cm_return_false_cmdquery_all;
extern ConVar* cm_return_false_cmdquery_cheats;
extern ConVar* r_debug_overlay_nodecay;
//-------------------------------------------------------------------------
// SERVER                                                                 |
extern ConVar* sv_showconnecting;
extern ConVar* sv_pylonvisibility;
//-------------------------------------------------------------------------
// CLIENT                                                                 |
extern ConVar* cl_drawconsoleoverlay;
extern ConVar* cl_consoleoverlay_lines;
extern ConVar* cl_consoleoverlay_offset_x;
extern ConVar* cl_consoleoverlay_offset_y;

extern ConVar* cl_showsimstats;
extern ConVar* cl_simstats_offset_x;
extern ConVar* cl_simstats_offset_y;

extern ConVar* cl_showgpustats;;
extern ConVar* cl_gpustats_offset_x;
extern ConVar* cl_gpustats_offset_y;
//-------------------------------------------------------------------------
// FILESYSTEM                                                             |
extern ConVar* fs_warning_level_native;
extern ConVar* fs_packedstore_entryblock_stats;
//-------------------------------------------------------------------------
// MATERIALSYSTEM                                                         |
extern ConVar* mat_showdxoutput;
//-------------------------------------------------------------------------
// SQUIRREL                                                               |
extern ConVar* sq_showrsonloading;
extern ConVar* sq_showscriptloading;
extern ConVar* sq_showvmoutput;
extern ConVar* sq_showvmwarning;
//-------------------------------------------------------------------------
// NETCHANNEL                                                             |
extern ConVar* net_userandomkey;
extern ConVar* r5net_matchmaking_hostname;
extern ConVar* r5net_show_debug;

///////////////////////////////////////////////////////////////////////////////
extern CCVar* g_pCvar;


///////////////////////////////////////////////////////////////////////////////
class HCvar : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: CCvar::Disconnect                    : 0x" << std::hex << std::uppercase << p_CCvar_Disconnect.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "| VAR: g_pCvar                              : 0x" << std::hex << std::uppercase << g_pCvar                     << std::setw(0)    << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HCvar);
