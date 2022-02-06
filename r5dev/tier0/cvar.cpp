#include "core/stdafx.h"
#include "tier0/cvar.h"
#include "tier0/IConVar.h"
#include "engine/sys_dll2.h"

//-----------------------------------------------------------------------------
// ENGINE                                                                     |
ConVar* cm_debug_cmdquery                  = new ConVar();
ConVar* cm_return_false_cmdquery_all       = new ConVar();
ConVar* cm_return_false_cmdquery_cheats    = new ConVar();
ConVar* r_debug_overlay_nodecay            = new ConVar();

ConVar* rcon_address                       = new ConVar();
ConVar* rcon_password                      = new ConVar();
//-----------------------------------------------------------------------------
// SERVER                                                                     |
ConVar* sv_showconnecting                  = new ConVar();
ConVar* sv_pylonvisibility                 = new ConVar();

ConVar* sv_rcon_banpenalty                 = new ConVar(); // TODO
ConVar* sv_rcon_maxfailures                = new ConVar();
ConVar* sv_rcon_whitelist_address          = new ConVar(); // TODO
//-----------------------------------------------------------------------------
// CLIENT                                                                     |
ConVar* cl_drawconsoleoverlay              = new ConVar();
ConVar* cl_consoleoverlay_lines            = new ConVar();
ConVar* cl_consoleoverlay_offset_x         = new ConVar();
ConVar* cl_consoleoverlay_offset_y         = new ConVar();
ConVar* cl_consoleoverlay_native_clr       = new ConVar();
ConVar* cl_consoleoverlay_server_clr       = new ConVar();
ConVar* cl_consoleoverlay_client_clr       = new ConVar();
ConVar* cl_consoleoverlay_ui_clr           = new ConVar();

ConVar* cl_showsimstats                    = new ConVar();
ConVar* cl_simstats_offset_x               = new ConVar();
ConVar* cl_simstats_offset_y               = new ConVar();

ConVar* cl_showgpustats                    = new ConVar();
ConVar* cl_gpustats_offset_x               = new ConVar();
ConVar* cl_gpustats_offset_y               = new ConVar();

ConVar* con_max_size_logvector             = new ConVar();
ConVar* con_suggestion_limit               = new ConVar();
ConVar* con_suggestion_helptext            = new ConVar();
//-----------------------------------------------------------------------------
// FILESYSTEM                                                                 |
ConVar* fs_warning_level_sdk               = new ConVar();
ConVar* fs_show_warning_output             = new ConVar();
ConVar* fs_packedstore_entryblock_stats    = new ConVar();
//-----------------------------------------------------------------------------
// MATERIALSYSTEM                                                             |
ConVar* mat_showdxoutput                   = new ConVar();
//-----------------------------------------------------------------------------
// SQUIRREL                                                                   |
ConVar* sq_showrsonloading                 = new ConVar();
ConVar* sq_showscriptloading               = new ConVar();
ConVar* sq_showvmoutput                    = new ConVar();
ConVar* sq_showvmwarning                   = new ConVar();
//-----------------------------------------------------------------------------
// NETCHANNEL                                                                 |
ConVar* net_userandomkey                   = new ConVar();
ConVar* r5net_matchmaking_hostname         = new ConVar();
ConVar* r5net_show_debug                   = new ConVar();

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
CCVarIteratorInternal* CCVar::FactoryInternalIterator()
{
	static int index = 41;
	return CallVFunc<CCVarIteratorInternal*>(index, this);
}

//-----------------------------------------------------------------------------
// Purpose: returns all ConVars
//-----------------------------------------------------------------------------
std::unordered_map<std::string, ConCommandBase*> CCVar::DumpToMap()
{
	std::stringstream ss;
	CCVarIteratorInternal* itint = FactoryInternalIterator(); // Allocate new InternalIterator.

	std::unordered_map<std::string, ConCommandBase*> allConVars;

	for (itint->SetFirst(); itint->IsValid(); itint->Next()) // Loop through all instances.
	{
		ConCommandBase* pCommand = itint->Get();
		const char* pszCommandName = pCommand->m_pszName;
		allConVars[pszCommandName] = pCommand;
	}

	return allConVars;
}

///////////////////////////////////////////////////////////////////////////////
std::vector<std::string> g_vsvAllConVars;
CCVar* g_pCVar = reinterpret_cast<CCVar*>(p_CEngineAPI_Connect.FindPatternSelf("48 8D 0D", ADDRESS::Direction::DOWN, 40).ResolveRelativeAddressSelf(0x3, 0x7).GetPtr());
