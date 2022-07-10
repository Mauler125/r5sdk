#pragma once
#include "tier1/IConVar.h"

//-------------------------------------------------------------------------
// ENGINE                                                                 |
extern ConVar* single_frame_shutdown_for_reload;
extern ConVar* old_gather_props;
extern ConVar* enable_debug_overlays;

extern ConVar* staticProp_defaultBuildFrustum;
extern ConVar* staticProp_no_fade_scalar;
extern ConVar* staticProp_gather_size_weight;

extern ConVar* model_defaultFadeDistScale;
extern ConVar* model_defaultFadeDistMin;

extern ConVar* hostname;
extern ConVar* hostdesc;
extern ConVar* hostip;
extern ConVar* hostport;
extern ConVar* host_hasIrreversibleShutdown;

extern ConVar* mp_gamemode;

extern ConVar* cm_debug_cmdquery;
extern ConVar* cm_unset_all_cmdquery;
extern ConVar* cm_unset_dev_cmdquery;
extern ConVar* cm_unset_cheat_cmdquery;

extern ConVar* rcon_address;
extern ConVar* rcon_password;

extern ConVar* r_debug_overlay_nodecay;
extern ConVar* r_debug_overlay_invisible;
extern ConVar* r_debug_overlay_wireframe;
extern ConVar* r_drawWorldMeshes;
extern ConVar* r_drawWorldMeshesDepthOnly;
extern ConVar* r_drawWorldMeshesDepthAtTheEnd;
//-------------------------------------------------------------------------
// SERVER                                                                 |
extern ConVar* ai_ainDumpOnLoad;
extern ConVar* ai_ainDebugConnect;
extern ConVar* navmesh_always_reachable;
extern ConVar* sv_showconnecting;
extern ConVar* sv_pylonVisibility;
extern ConVar* sv_pylonRefreshInterval;
extern ConVar* sv_banlistRefreshInterval;
extern ConVar* sv_statusRefreshInterval;
#ifdef DEDICATED
extern ConVar* sv_rcon_debug;
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
extern ConVar* cl_consoleoverlay_invert_rect_x;
extern ConVar* cl_consoleoverlay_invert_rect_y;
extern ConVar* cl_consoleoverlay_offset_x;
extern ConVar* cl_consoleoverlay_offset_y;

extern ConVar* cl_conoverlay_script_server_clr;
extern ConVar* cl_conoverlay_script_client_clr;
extern ConVar* cl_conoverlay_script_ui_clr;
extern ConVar* cl_conoverlay_native_server_clr;
extern ConVar* cl_conoverlay_native_client_clr;
extern ConVar* cl_conoverlay_native_ui_clr;
extern ConVar* cl_conoverlay_native_engine_clr;
extern ConVar* cl_conoverlay_native_fs_clr;
extern ConVar* cl_conoverlay_native_rtech_clr;
extern ConVar* cl_conoverlay_native_ms_clr;
extern ConVar* cl_conoverlay_netcon_clr;
extern ConVar* cl_conoverlay_warning_clr;
extern ConVar* cl_conoverlay_error_clr;

extern ConVar* cl_showhoststats;
extern ConVar* cl_hoststats_invert_rect_x;
extern ConVar* cl_hoststats_invert_rect_y;
extern ConVar* cl_hoststats_offset_x;
extern ConVar* cl_hoststats_offset_y;

extern ConVar* cl_showsimstats;
extern ConVar* cl_simstats_invert_rect_x;
extern ConVar* cl_simstats_invert_rect_y;
extern ConVar* cl_simstats_offset_x;
extern ConVar* cl_simstats_offset_y;

extern ConVar* cl_showgpustats;
extern ConVar* cl_gpustats_invert_rect_x;
extern ConVar* cl_gpustats_invert_rect_y;
extern ConVar* cl_gpustats_offset_x;
extern ConVar* cl_gpustats_offset_y;

extern ConVar* cl_showmaterialinfo;
extern ConVar* cl_materialinfo_offset_x;
extern ConVar* cl_materialinfo_offset_y;

extern ConVar* con_max_size_logvector;
extern ConVar* con_suggestion_limit;
extern ConVar* con_suggestion_showhelptext;
extern ConVar* con_suggestion_showflags;
extern ConVar* con_suggestion_flags_realtime;
#endif // !DEDICATED
//-------------------------------------------------------------------------
// FILESYSTEM                                                             |
extern ConVar* fs_warning_level_sdk;
extern ConVar* fs_show_warning_output;
extern ConVar* fs_packedstore_entryblock_stats;
extern ConVar* fs_packedstore_workspace;
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
extern ConVar* net_tracePayload;
extern ConVar* net_encryptionEnable;
extern ConVar* net_useRandomKey;
extern ConVar* net_usesocketsforloopback;
extern ConVar* pylon_matchmaking_hostname;
extern ConVar* pylon_showdebug;
//-------------------------------------------------------------------------
// RTECH API                                                              |
//-------------------------------------------------------------------------
// RUI                                                                    |
#ifndef DEDICATED
extern ConVar* rui_drawEnable;
#endif // !DEDICATED

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
	ConCommandBase* RegisterConCommand(ConCommandBase* pCommandToAdd);
	ConCommandBase* UnregisterConCommand(ConCommandBase* pCommandToRemove);
	ConCommandBase* FindCommandBase(const char* pszCommandName); // @0x1405983A0 in R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM
	ConVar* FindVar(const char* pszVarName);                     // @0x1405983B0 in R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM
	ConCommand* FindCommand(const char* pszCommandName);
	CCVarIteratorInternal* FactoryInternalIterator(void);
	unordered_map<string, ConCommandBase*> DumpToMap(void);
};

///////////////////////////////////////////////////////////////////////////////
extern CCVar* g_pCVar;

/* ==== CCVAR =========================================================================================================================================================== */
inline CMemory p_CCVar_Disconnect;
inline auto CCVar_Disconnect = p_CCVar_Disconnect.RCast<void* (*)(void)>();

inline CMemory p_CCVar_GetCommandLineValue;
inline auto CCVar_GetCommandLineValue = p_CCVar_GetCommandLineValue.RCast<const char* (*)(CCVar* thisptr, const char* pVariableName)>();

///////////////////////////////////////////////////////////////////////////////
class VCVar : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: CCVar::Disconnect                    : {:#18x} |\n", p_CCVar_Disconnect.GetPtr());
		spdlog::debug("| FUN: CCVar::GetCommandLineValue           : {:#18x} |\n", p_CCVar_GetCommandLineValue.GetPtr());
		spdlog::debug("| VAR: g_pCVar                              : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_pCVar));
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_CCVar_Disconnect = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x57\x41\x56\x48\x83\xEC\x38\x4C\x8B\x35"), "xxxxxxxxxxx");
		CCVar_Disconnect = p_CCVar_Disconnect.RCast<void* (*)(void)>(); /*40 57 41 56 48 83 EC 38 4C 8B 35 ? ? ? ?*/
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_CCVar_Disconnect = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x83\xEC\x28\x48\x8B\x0D\x00\x00\x00\x00\x48\x85\xC9\x74\x26\x80\x3D\x00\x00\x00\x00\x00\x74\x1D\x48\x8B\x01\x8B\x15\x00\x00\x00\x00\xFF\x50\x58\xC7\x05\x00\x00\x00\x00\x00\x00\x00\x00\xC6\x05\x00\x00\x00\x00\x00\x48\xC7\x05\x00\x00\x00"), "xxxxxxx????xxxxxxx?????xxxxxxx????xxxxx????????xx");
		CCVar_Disconnect = p_CCVar_Disconnect.RCast<void* (*)(void)>(); /*48 83 EC 28 48 8B 0D ? ? ? ? 48 85 C9 74 26 80 3D ? ? ? ? ? 74 1D 48 8B 01 8B 15 ? ? ? ? FF 50 58 C7 05 ? ? ? ? ? ? ? ? C6 05 ? ? ? ? ? 48 C7 05 ? ? ? ? ? ? ? ?*/
#endif
		p_CCVar_GetCommandLineValue = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x55\x48\x83\xEC\x20\x48\x8D\x6C\x24\x00\x48\x89\x5D\x10\x49\xC7\xC0\x00\x00\x00\x00"), "xxxxxxxxxx?xxxxxxx????");
		CCVar_GetCommandLineValue = p_CCVar_GetCommandLineValue.RCast<const char* (*)(CCVar* thisptr, const char* pVariableName)>(); /*40 55 48 83 EC 20 48 8D 6C 24 ? 48 89 5D 10 49 C7 C0 ? ? ? ?*/
	}
	virtual void GetVar(void) const
	{
		g_pCVar = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>(
			"\x48\x83\xEC\x28\x48\x8B\x05\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00\x48\x85\xC0\x48\x89\x15"),
			"xxxxxxx????xxx????xxxxxx").FindPatternSelf("48 8D 0D", CMemory::Direction::DOWN, 40).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CCVar*>();
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VCVar);
