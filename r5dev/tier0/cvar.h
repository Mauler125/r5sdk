#pragma once
#include "tier0/IConVar.h"

namespace
{
	/* ==== CCVAR =========================================================================================================================================================== */
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	ADDRESS p_CCVar_Disconnect = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x40\x57\x41\x56\x48\x83\xEC\x38\x4C\x8B\x35", "xxxxxxxxxxx");
	void* (*CCVar_Disconnect)() = (void* (*)())p_CCVar_Disconnect.GetPtr(); /*40 57 41 56 48 83 EC 38 4C 8B 35 ? ? ? ?*/
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	ADDRESS p_CCVar_Disconnect = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x83\xEC\x28\x48\x8B\x0D\x00\x00\x00\x00\x48\x85\xC9\x74\x26\x80\x3D\x00\x00\x00\x00\x00\x74\x1D\x48\x8B\x01\x8B\x15\x00\x00\x00\x00\xFF\x50\x58\xC7\x05\x00\x00\x00\x00\x00\x00\x00\x00\xC6\x05\x00\x00\x00\x00\x00\x48\xC7\x05\x00\x00\x00", "xxxxxxx????xxxxxxx?????xxxxxxx????xxxxx????????xx");
	void* (*CCVar_Disconnect)() = (void* (*)())p_CCVar_Disconnect.GetPtr(); /*48 83 EC 28 48 8B 0D ? ? ? ? 48 85 C9 74 26 80 3D ? ? ? ? ? 74 1D 48 8B 01 8B 15 ? ? ? ? FF 50 58 C7 05 ? ? ? ? ? ? ? ? C6 05 ? ? ? ? ? 48 C7 05 ? ? ? ? ? ? ? ?*/
#endif
	ADDRESS p_CCVar_GetCommandLineValue = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x40\x55\x48\x83\xEC\x20\x48\x8D\x6C\x24\x00\x48\x89\x5D\x10\x49\xC7\xC0\x00\x00\x00\x00", "xxxxxxxxxx?xxxxxxx????");
	const char* (*CCVar_GetCommandLineValue)(void* thisptr, const char* pVariableName) = (const char* (*)(void*, const char*))p_CCVar_GetCommandLineValue.GetPtr(); /*40 55 48 83 EC 20 48 8D 6C 24 ? 48 89 5D 10 49 C7 C0 ? ? ? ?*/
}

//-------------------------------------------------------------------------
// ENGINE                                                                 |
extern ConVar* cm_debug_cmdquery;
extern ConVar* cm_return_false_cmdquery_all;
extern ConVar* cm_return_false_cmdquery_cheats;
extern ConVar* r_debug_overlay_nodecay;

extern ConVar* rcon_address;
extern ConVar* rcon_password;
//-------------------------------------------------------------------------
// SERVER                                                                 |
extern ConVar* sv_showconnecting;
extern ConVar* sv_pylonvisibility;
#ifdef DEDICATED
extern ConVar* sv_rcon_banpenalty;
extern ConVar* sv_rcon_maxfailures;
extern ConVar* sv_rcon_maxignores;
extern ConVar* sv_rcon_maxsockets;
extern ConVar* sv_rcon_whitelist_address;
#endif // DEDICATED
//-------------------------------------------------------------------------
// CLIENT                                                                 |
#ifndef DEDICATED
extern ConVar* cl_drawconsoleoverlay;
extern ConVar* cl_consoleoverlay_lines;
extern ConVar* cl_consoleoverlay_offset_x;
extern ConVar* cl_consoleoverlay_offset_y;
extern ConVar* cl_consoleoverlay_native_clr;
extern ConVar* cl_consoleoverlay_server_clr;
extern ConVar* cl_consoleoverlay_client_clr;
extern ConVar* cl_consoleoverlay_ui_clr;

extern ConVar* cl_showsimstats;
extern ConVar* cl_simstats_offset_x;
extern ConVar* cl_simstats_offset_y;

extern ConVar* cl_showgpustats;;
extern ConVar* cl_gpustats_offset_x;
extern ConVar* cl_gpustats_offset_y;

extern ConVar* con_max_size_logvector;
extern ConVar* con_suggestion_limit;
extern ConVar* con_suggestion_helptext;
#endif // !DEDICATED
//-------------------------------------------------------------------------
// FILESYSTEM                                                             |
extern ConVar* fs_warning_level_sdk;
extern ConVar* fs_show_warning_output;
extern ConVar* fs_packedstore_entryblock_stats;
//-------------------------------------------------------------------------
// MATERIALSYSTEM                                                         |
#ifndef DEDICATED
extern ConVar* mat_showdxoutput;
#endif // !DEDICATED
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

class CCVarIteratorInternal // Fully reversed table, just look at the virtual function table and rename the function.
{
public:
	virtual void            SetFirst(void) = 0; //0
	virtual void            Next(void)     = 0; //1
	virtual	bool            IsValid(void)  = 0; //2
	virtual ConCommandBase* Get(void)      = 0; //3
};

class CCVar
{
public:
	ConCommandBase* FindCommandBase(const char* pszCommandName); // @0x1405983A0 in R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM
	ConVar* FindVar(const char* pszVarName);                     // @0x1405983B0 in R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM
	ConCommand* FindCommand(const char* pszCommandName);
	CCVarIteratorInternal* FactoryInternalIterator();
	std::unordered_map<std::string, ConCommandBase*> DumpToMap();
};

///////////////////////////////////////////////////////////////////////////////
extern std::vector<std::string> g_vsvAllConVars;
extern CCVar* g_pCVar;


///////////////////////////////////////////////////////////////////////////////
class HCvar : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: CCVar::Disconnect                    : 0x" << std::hex << std::uppercase << p_CCVar_Disconnect.GetPtr()          << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: CCVar::GetCommandLineValue           : 0x" << std::hex << std::uppercase << p_CCVar_GetCommandLineValue.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "| VAR: g_pCvar                              : 0x" << std::hex << std::uppercase << g_pCVar                              << std::setw(0)    << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HCvar);
