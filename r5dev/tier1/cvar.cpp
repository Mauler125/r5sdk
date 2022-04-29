#include "core/stdafx.h"
#include "tier1/cvar.h"
#include "tier1/IConVar.h"
#include "engine/sys_dll2.h"

//-----------------------------------------------------------------------------
// ENGINE                                                                     |
ConVar* single_frame_shutdown_for_reload   = nullptr;

ConVar* hostname                           = nullptr;
ConVar* hostport                           = nullptr;
ConVar* host_hasIrreversibleShutdown       = nullptr;
ConVar* mp_gamemode                        = nullptr;

ConVar* cm_debug_cmdquery                  = nullptr;
ConVar* cm_unset_all_cmdquery              = nullptr;
ConVar* cm_unset_dev_cmdquery              = nullptr;
ConVar* cm_unset_cheat_cmdquery            = nullptr;

ConVar* rcon_address                       = nullptr;
ConVar* rcon_password                      = nullptr;

ConVar* r_debug_overlay_nodecay            = nullptr;
ConVar* r_drawWorldMeshes                  = nullptr;
ConVar* r_drawWorldMeshesDepthOnly         = nullptr;
ConVar* r_drawWorldMeshesDepthAtTheEnd     = nullptr;
//-----------------------------------------------------------------------------
// SERVER                                                                     |
ConVar* ai_ainDumpOnLoad                   = nullptr;
ConVar* ai_ainDebugConnect                 = nullptr;
ConVar* navmesh_always_reachable           = nullptr;

ConVar* sv_showconnecting                  = nullptr;
ConVar* sv_pylonvisibility                 = nullptr;
ConVar* sv_pylonRefreshInterval            = nullptr;
ConVar* sv_banlistRefreshInterval          = nullptr;
ConVar* sv_statusRefreshInterval           = nullptr;

#ifdef DEDICATED
ConVar* sv_rcon_debug                      = nullptr;
ConVar* sv_rcon_banpenalty                 = nullptr; // TODO
ConVar* sv_rcon_maxfailures                = nullptr;
ConVar* sv_rcon_maxignores                 = nullptr;
ConVar* sv_rcon_maxsockets                 = nullptr;
ConVar* sv_rcon_whitelist_address          = nullptr;
#endif // DEDICATED
//-----------------------------------------------------------------------------
// CLIENT                                                                     |
#ifndef DEDICATED
ConVar* cl_drawconsoleoverlay              = nullptr;
ConVar* cl_consoleoverlay_lines            = nullptr;
ConVar* cl_consoleoverlay_invert_rect_x    = nullptr;
ConVar* cl_consoleoverlay_invert_rect_y    = nullptr;
ConVar* cl_consoleoverlay_offset_x         = nullptr;
ConVar* cl_consoleoverlay_offset_y         = nullptr;

ConVar* cl_conoverlay_script_server_clr    = nullptr;
ConVar* cl_conoverlay_script_client_clr    = nullptr;
ConVar* cl_conoverlay_script_ui_clr        = nullptr;
ConVar* cl_conoverlay_native_server_clr    = nullptr;
ConVar* cl_conoverlay_native_client_clr    = nullptr;
ConVar* cl_conoverlay_native_ui_clr        = nullptr;
ConVar* cl_conoverlay_native_engine_clr    = nullptr;
ConVar* cl_conoverlay_native_fs_clr        = nullptr;
ConVar* cl_conoverlay_native_rtech_clr     = nullptr;
ConVar* cl_conoverlay_native_ms_clr        = nullptr;
ConVar* cl_conoverlay_netcon_clr           = nullptr;
ConVar* cl_conoverlay_warning_clr          = nullptr;
ConVar* cl_conoverlay_error_clr            = nullptr;

ConVar* cl_showhoststats                   = nullptr;
ConVar* cl_hoststats_invert_rect_x         = nullptr;
ConVar* cl_hoststats_invert_rect_y         = nullptr;
ConVar* cl_hoststats_offset_x              = nullptr;
ConVar* cl_hoststats_offset_y              = nullptr;

ConVar* cl_showsimstats                    = nullptr;
ConVar* cl_simstats_invert_rect_x          = nullptr;
ConVar* cl_simstats_invert_rect_y          = nullptr;
ConVar* cl_simstats_offset_x               = nullptr;
ConVar* cl_simstats_offset_y               = nullptr;

ConVar* cl_showgpustats                    = nullptr;
ConVar* cl_gpustats_invert_rect_x          = nullptr;
ConVar* cl_gpustats_invert_rect_y          = nullptr;
ConVar* cl_gpustats_offset_x               = nullptr;
ConVar* cl_gpustats_offset_y               = nullptr;

ConVar* con_max_size_logvector             = nullptr;
ConVar* con_suggestion_limit               = nullptr;
ConVar* con_suggestion_showhelptext        = nullptr;
ConVar* con_suggestion_showflags           = nullptr;
ConVar* con_suggestion_flags_realtime      = nullptr;
#endif // !DEDICATED
//-----------------------------------------------------------------------------
// FILESYSTEM                                                                 |
ConVar* fs_warning_level_sdk               = nullptr;
ConVar* fs_show_warning_output             = nullptr;
ConVar* fs_packedstore_entryblock_stats    = nullptr;
//-----------------------------------------------------------------------------
// MATERIALSYSTEM                                                             |
#ifndef DEDICATED
ConVar* mat_showdxoutput                   = nullptr;
#endif // !DEDICATED
//-----------------------------------------------------------------------------
// SQUIRREL                                                                   |
ConVar* sq_showrsonloading                 = nullptr;
ConVar* sq_showscriptloading               = nullptr;
ConVar* sq_showvmoutput                    = nullptr;
ConVar* sq_showvmwarning                   = nullptr;
//-----------------------------------------------------------------------------
// NETCHANNEL                                                                 |
ConVar* net_tracePayload                   = nullptr;
ConVar* net_encryptionEnable               = nullptr;
ConVar* net_useRandomKey                   = nullptr;
ConVar* net_usesocketsforloopback          = nullptr;
ConVar* r5net_matchmaking_hostname         = nullptr;
ConVar* r5net_show_debug                   = nullptr;
//-----------------------------------------------------------------------------
// RTECH API                                                                  |
//-----------------------------------------------------------------------------
// RUI                                                                        |
#ifndef DEDICATED
ConVar* rui_drawEnable                     = nullptr;
#endif // !DEDICATED

//-----------------------------------------------------------------------------
// Purpose: registers input commands.
// Input  : *pszCommandName - 
//-----------------------------------------------------------------------------
ConCommandBase* CCVar::RegisterConCommand(ConCommandBase* pCommandToRemove)
{
	static int index = 9;
	return CallVFunc<ConCommandBase*>(index, this, pCommandToRemove);
}

//-----------------------------------------------------------------------------
// Purpose: unregisters input commands.
// Input  : *pszCommandName - 
//-----------------------------------------------------------------------------
ConCommandBase* CCVar::UnregisterConCommand(ConCommandBase* pCommandToRemove)
{
	static int index = 10;
	return CallVFunc<ConCommandBase*>(index, this, pCommandToRemove);
}

//-----------------------------------------------------------------------------
// Purpose: finds base commands.
// Input  : *pszCommandName - 
//-----------------------------------------------------------------------------
ConCommandBase* CCVar::FindCommandBase(const char* pszCommandName)
{
	static int index = 14;
	return CallVFunc<ConCommandBase*>(index, this, pszCommandName);
}

//-----------------------------------------------------------------------------
// Purpose: finds ConVars.
// Input  : *pszVarName - 
//-----------------------------------------------------------------------------
ConVar* CCVar::FindVar(const char* pszVarName)
{
	static int index = 16;
	return CallVFunc<ConVar*>(index, this, pszVarName);
}

//-----------------------------------------------------------------------------
// Purpose: finds ConCommands.
// Input  : *pszCommandName - 
//-----------------------------------------------------------------------------
ConCommand* CCVar::FindCommand(const char* pszCommandName)
{
	static int index = 18;
	return CallVFunc<ConCommand*>(index, this, pszCommandName);
}

//-----------------------------------------------------------------------------
// Purpose: iterates over all ConVars
//-----------------------------------------------------------------------------
CCVarIteratorInternal* CCVar::FactoryInternalIterator(void)
{
	static int index = 41;
	return CallVFunc<CCVarIteratorInternal*>(index, this);
}

//-----------------------------------------------------------------------------
// Purpose: returns all ConVars
//-----------------------------------------------------------------------------
unordered_map<string, ConCommandBase*> CCVar::DumpToMap(void)
{
	stringstream ss;
	CCVarIteratorInternal* itint = FactoryInternalIterator(); // Allocate new InternalIterator.

	unordered_map<string, ConCommandBase*> allConVars;

	for (itint->SetFirst(); itint->IsValid(); itint->Next()) // Loop through all instances.
	{
		ConCommandBase* pCommand = itint->Get();
		const char* pszCommandName = pCommand->m_pszName;
		allConVars[pszCommandName] = pCommand;
	}

	return allConVars;
}

///////////////////////////////////////////////////////////////////////////////
CCVar* g_pCVar = nullptr;
