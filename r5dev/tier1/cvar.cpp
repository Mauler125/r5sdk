#include "core/stdafx.h"
#include "tier1/utlrbtree.h"
#include "tier1/cvar.h"
#include "tier1/IConVar.h"
#include "engine/sys_dll2.h"
#include "filesystem/filesystem.h"
#include "vstdlib/concommandhash.h"

//-----------------------------------------------------------------------------
// ENGINE                                                                     |
ConVar* sdk_fixedframe_tickinterval        = nullptr;
ConVar* single_frame_shutdown_for_reload   = nullptr;
ConVar* old_gather_props                   = nullptr;
ConVar* enable_debug_overlays              = nullptr;
ConVar* cm_unset_all_cmdquery              = nullptr;

ConVar* staticProp_defaultBuildFrustum     = nullptr;
ConVar* staticProp_no_fade_scalar          = nullptr;
ConVar* staticProp_gather_size_weight      = nullptr;

ConVar* model_defaultFadeDistScale         = nullptr;
ConVar* model_defaultFadeDistMin           = nullptr;

ConVar* hostname                           = nullptr;
ConVar* hostdesc                           = nullptr;
ConVar* hostip                             = nullptr;
ConVar* hostport                           = nullptr;
ConVar* host_hasIrreversibleShutdown       = nullptr;
ConVar* mp_gamemode                        = nullptr;

ConVar* rcon_address                       = nullptr;
ConVar* rcon_password                      = nullptr;

ConVar* r_debug_overlay_nodecay            = nullptr;
ConVar* r_debug_overlay_invisible          = nullptr;
ConVar* r_debug_overlay_wireframe          = nullptr;
ConVar* r_debug_overlay_zbuffer            = nullptr;
ConVar* r_drawWorldMeshes                  = nullptr;
ConVar* r_drawWorldMeshesDepthOnly         = nullptr;
ConVar* r_drawWorldMeshesDepthAtTheEnd     = nullptr;

ConVar* stream_overlay                     = nullptr;
ConVar* stream_overlay_mode                = nullptr;
//-----------------------------------------------------------------------------
// SERVER                                                                     |
#ifndef CLIENT_DLL
ConVar* ai_ainDumpOnLoad                   = nullptr;
ConVar* ai_ainDebugConnect                 = nullptr;
ConVar* ai_script_nodes_draw               = nullptr;
ConVar* ai_script_nodes_draw_range         = nullptr;

ConVar* navmesh_always_reachable           = nullptr;
ConVar* navmesh_debug_type                 = nullptr;
ConVar* navmesh_debug_tile_range           = nullptr;
ConVar* navmesh_debug_camera_range         = nullptr;
#ifndef DEDICATED
ConVar* navmesh_draw_bvtree                = nullptr;
ConVar* navmesh_draw_portal                = nullptr;
ConVar* navmesh_draw_polys                 = nullptr;
ConVar* navmesh_draw_poly_bounds           = nullptr;
ConVar* navmesh_draw_poly_bounds_inner     = nullptr;
#endif // !DEDICATED

ConVar* sv_showconnecting                  = nullptr;
ConVar* sv_pylonVisibility                 = nullptr;
ConVar* sv_pylonRefreshInterval            = nullptr;
ConVar* sv_banlistRefreshInterval          = nullptr;
ConVar* sv_statusRefreshInterval           = nullptr;

#ifdef DEDICATED
ConVar* sv_rcon_debug                      = nullptr;
ConVar* sv_rcon_sendlogs                   = nullptr;
ConVar* sv_rcon_banpenalty                 = nullptr; // TODO
ConVar* sv_rcon_maxfailures                = nullptr;
ConVar* sv_rcon_maxignores                 = nullptr;
ConVar* sv_rcon_maxsockets                 = nullptr;
ConVar* sv_rcon_whitelist_address          = nullptr;
#endif // DEDICATED
#endif // !CLIENT_DLL

ConVar* sv_visualizetraces = nullptr;
#if !defined (GAMEDLL_S0) && !defined (GAMEDLL_S1)
ConVar* bhit_enable = nullptr;
ConVar* bhit_abs_origin = nullptr;
#endif // !GAMEDLL_S0 && !GAMEDLL_S1
//-----------------------------------------------------------------------------
// CLIENT                                                                     |
#ifndef DEDICATED
ConVar* cl_rcon_request_sendlogs           = nullptr;

ConVar* cl_showhoststats                   = nullptr;
ConVar* cl_hoststats_invert_x              = nullptr;
ConVar* cl_hoststats_invert_y              = nullptr;
ConVar* cl_hoststats_offset_x              = nullptr;
ConVar* cl_hoststats_offset_y              = nullptr;

ConVar* cl_showsimstats                    = nullptr;
ConVar* cl_simstats_invert_x               = nullptr;
ConVar* cl_simstats_invert_y               = nullptr;
ConVar* cl_simstats_offset_x               = nullptr;
ConVar* cl_simstats_offset_y               = nullptr;

ConVar* cl_showgpustats                    = nullptr;
ConVar* cl_gpustats_invert_x               = nullptr;
ConVar* cl_gpustats_invert_y               = nullptr;
ConVar* cl_gpustats_offset_x               = nullptr;
ConVar* cl_gpustats_offset_y               = nullptr;

ConVar* cl_showmaterialinfo                = nullptr;
ConVar* cl_materialinfo_offset_x           = nullptr;
ConVar* cl_materialinfo_offset_y           = nullptr;

ConVar* cl_threaded_bone_setup             = nullptr;

ConVar* con_drawnotify                     = nullptr;
ConVar* con_notifylines                    = nullptr;
ConVar* con_notifytime                     = nullptr;

ConVar* con_notify_invert_x                = nullptr;
ConVar* con_notify_invert_y                = nullptr;
ConVar* con_notify_offset_x                = nullptr;
ConVar* con_notify_offset_y                = nullptr;

ConVar* con_notify_script_server_clr       = nullptr;
ConVar* con_notify_script_client_clr       = nullptr;
ConVar* con_notify_script_ui_clr           = nullptr;
ConVar* con_notify_native_server_clr       = nullptr;
ConVar* con_notify_native_client_clr       = nullptr;
ConVar* con_notify_native_ui_clr           = nullptr;
ConVar* con_notify_native_engine_clr       = nullptr;
ConVar* con_notify_native_fs_clr           = nullptr;
ConVar* con_notify_native_rtech_clr        = nullptr;
ConVar* con_notify_native_ms_clr           = nullptr;
ConVar* con_notify_netcon_clr              = nullptr;
ConVar* con_notify_common_clr              = nullptr;
ConVar* con_notify_warning_clr             = nullptr;
ConVar* con_notify_error_clr               = nullptr;

ConVar* con_max_size_logvector             = nullptr;
ConVar* con_max_size_history               = nullptr;
ConVar* con_suggestion_limit               = nullptr;
ConVar* con_suggestion_showhelptext        = nullptr;
ConVar* con_suggestion_showflags           = nullptr;
ConVar* con_suggestion_flags_realtime      = nullptr;

ConVar* origin_disconnectWhenOffline       = nullptr;
#endif // !DEDICATED
//-----------------------------------------------------------------------------
// FILESYSTEM                                                                 |
ConVar* fs_warning_level_sdk               = nullptr;
ConVar* fs_show_warning_output             = nullptr;
ConVar* fs_packedstore_entryblock_stats    = nullptr;
ConVar* fs_packedstore_workspace           = nullptr;
ConVar* fs_packedstore_compression_level   = nullptr;
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
ConVar* pylon_matchmaking_hostname         = nullptr;
ConVar* pylon_host_update_interval         = nullptr;
ConVar* pylon_showdebuginfo                = nullptr;
//-----------------------------------------------------------------------------
// RTECH API                                                                  |
ConVar* rtech_debug                        = nullptr;
//-----------------------------------------------------------------------------
// RUI                                                                        |
#ifndef DEDICATED
ConVar* rui_drawEnable                     = nullptr;
ConVar* rui_defaultDebugFontFace           = nullptr;
#endif // !DEDICATED

struct ConVarFlags_t
{
	int			bit;
	const char* desc;
	const char* shortdesc;
};

#define CONVARFLAG( x, y )	{ FCVAR_##x, #x, #y }

static ConVarFlags_t g_ConVarFlags[] =
{
	CONVARFLAG(UNREGISTERED, "u" ),
	CONVARFLAG(ARCHIVE, "a"),
	CONVARFLAG(ARCHIVE_PLAYERPROFILE, "ap"),
	CONVARFLAG(SPONLY, "sp"),
	CONVARFLAG(GAMEDLL, "sv"),
	CONVARFLAG(CHEAT, "cheat"),
	CONVARFLAG(USERINFO, "user"),
	CONVARFLAG(NOTIFY, "nf"),
	CONVARFLAG(PROTECTED, "prot"),
	CONVARFLAG(PRINTABLEONLY, "print"),
	CONVARFLAG(UNLOGGED, "ulog"),
	CONVARFLAG(NEVER_AS_STRING, "numeric"),
	CONVARFLAG(REPLICATED, "rep"),
	CONVARFLAG(DEMO, "demo"),
	CONVARFLAG(DONTRECORD, "norecord"),
	CONVARFLAG(SERVER_CAN_EXECUTE, "server_can_execute"),
	CONVARFLAG(CLIENTCMD_CAN_EXECUTE, "clientcmd_can_execute"),
	CONVARFLAG(CLIENTDLL, "cl"),
};

static void PrintListHeader(FileHandle_t& f)
{
	char csvflagstr[1024];

	csvflagstr[0] = 0;

	int c = ARRAYSIZE(g_ConVarFlags);
	for (int i = 0; i < c; ++i)
	{
		char csvf[64];

		ConVarFlags_t& entry = g_ConVarFlags[i];
		Q_snprintf(csvf, sizeof(csvf), "\"%s\",", entry.desc);
		Q_strncat(csvflagstr, csvf, sizeof(csvflagstr) - strlen(csvflagstr) - 1);
	}

	FileSystem()->FPrintf(f, "\"%s\",\"%s\",%s,\"%s\"\n", "Name", "Value", csvflagstr, "Help Text");
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *var - 
//			*f - 
//-----------------------------------------------------------------------------
static void PrintCvar(ConVar* var, bool logging, FileHandle_t& fh)
{
	char flagstr[128];
	char csvflagstr[1024];

	flagstr[0] = 0;
	csvflagstr[0] = 0;

	int c = ARRAYSIZE(g_ConVarFlags);
	for (int i = 0; i < c; ++i)
	{
		char f[32];
		char csvf[64];
		size_t flen = sizeof(csvflagstr) - strlen(csvflagstr) - 1;

		ConVarFlags_t& entry = g_ConVarFlags[i];
		if (var->IsFlagSet(entry.bit))
		{
			Q_snprintf(f, sizeof(f), ", %s", entry.shortdesc);
			Q_strncat(flagstr, f, sizeof(flagstr) - strlen(flagstr) - 1);
			Q_snprintf(csvf, sizeof(csvf), "\"%s\",", entry.desc);
		}
		else
		{
			Q_snprintf(csvf, sizeof(csvf), ",");
		}

		Q_strncat(csvflagstr, csvf, flen);
	}


	char valstr[32];
	char tempbuff[512] = { 0 };

	// Clean up integers
	if (var->GetInt() == (int)var->GetFloat())
	{
		Q_snprintf(valstr, sizeof(valstr), "%-8i", var->GetInt());
	}
	else
	{
		Q_snprintf(valstr, sizeof(valstr), "%-8.3f", var->GetFloat());
	}

	// Print to console
	DevMsg(eDLL_T::ENGINE, "%-40s : %-8s : %-16s : %s\n", var->GetName(), valstr, flagstr, StripTabsAndReturns(var->GetHelpText(), tempbuff, sizeof(tempbuff)));
	if (logging)
	{
		FileSystem()->FPrintf(fh, "\"%s\",\"%s\",%s,\"%s\"\n", var->GetName(), valstr, csvflagstr, StripQuotes(var->GetHelpText(), tempbuff, sizeof(tempbuff)));
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output :
//-----------------------------------------------------------------------------
static void PrintCommand(const ConCommand* cmd, bool logging, FileHandle_t& f)
{
	// Print to console
	char tempbuff[512] = { 0 };
	DevMsg(eDLL_T::ENGINE, "%-40s : %-8s : %-16s : %s\n", cmd->GetName(), "cmd", "", StripTabsAndReturns(cmd->GetHelpText(), tempbuff, sizeof(tempbuff)));
	if (logging)
	{
		char emptyflags[256];

		emptyflags[0] = 0;

		int c = ARRAYSIZE(g_ConVarFlags);
		for (int i = 0; i < c; ++i)
		{
			char csvf[64];
			size_t len = sizeof(emptyflags) - strlen(emptyflags) - 1;

			Q_snprintf(csvf, sizeof(csvf), ",");
			Q_strncat(emptyflags, csvf, len);
		}
		// Names staring with +/- need to be wrapped in single quotes
		char name[256];
		Q_snprintf(name, sizeof(name), "%s", cmd->GetName());
		if (name[0] == '+' || name[0] == '-')
		{
			Q_snprintf(name, sizeof(name), "'%s'", cmd->GetName());
		}
		FileSystem()->FPrintf(f, "\"%s\",\"%s\",%s,\"%s\"\n", name, "cmd", emptyflags, StripQuotes(cmd->GetHelpText(), tempbuff, sizeof(tempbuff)));
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output : bool
//-----------------------------------------------------------------------------
static bool ConCommandBaseLessFunc(ConCommandBase* const& lhs, ConCommandBase* const& rhs)
{
	const char* left = lhs->GetName();
	const char* right = rhs->GetName();

	if (*left == '-' || *left == '+')
		left++;
	if (*right == '-' || *right == '+')
		right++;

	return (Q_stricmp(left, right) < 0);
}

//-----------------------------------------------------------------------------
// Singleton CCvarUtilities
//-----------------------------------------------------------------------------
static CCvarUtilities g_CvarUtilities;
CCvarUtilities* cv = &g_CvarUtilities;

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output : int
//-----------------------------------------------------------------------------
int CCvarUtilities::CountVariablesWithFlags(int flags)
{
	int i = 0;
	ConCommandBase* var;
	CCvar::CCVarIteratorInternal* itint = g_pCVar->FactoryInternalIterator();

	for (itint->SetFirst(); itint->IsValid(); itint->Next()) // Loop through cvars...
	{
		var = itint->Get();
		if (!var->IsCommand())
		{
			if (var->IsFlagSet(flags))
			{
				i++;
			}
		}
	}

	MemAllocSingleton()->Free(itint);
	return i;
}

//-----------------------------------------------------------------------------
// Purpose: Removes the FCVAR_DEVELOPMENTONLY flag from all cvars, making them accessible
//-----------------------------------------------------------------------------
void CCvarUtilities::EnableDevCvars()
{
	// Loop through cvars...
	CCvar::CCVarIteratorInternal* itint = g_pCVar->FactoryInternalIterator();

	for (itint->SetFirst(); itint->IsValid(); itint->Next())
	{
		// remove flag from all cvars
		ConCommandBase* pCommandBase = itint->Get();
		pCommandBase->RemoveFlags(FCVAR_DEVELOPMENTONLY);
	}

	MemAllocSingleton()->Free(itint);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : void CCvar::CvarList_f
//-----------------------------------------------------------------------------
void CCvarUtilities::CvarList(const CCommand& args)
{
	ConCommandBase* var;	// Temporary Pointer to cvars
	int iArgs;						// Argument count
	const char* partial = NULL;		// Partial cvar to search for...
									// E.eg
	int ipLen = 0;					// Length of the partial cvar

	FileHandle_t f = FILESYSTEM_INVALID_HANDLE;         // FilePointer for logging
	bool bLogging = false;
	// Are we logging?
	iArgs = args.ArgC();		// Get count

	// Print usage?
	if (iArgs == 2 && !Q_strcasecmp(args[1], "?"))
	{
		DevMsg(eDLL_T::ENGINE, "cvarlist:  [log logfile] [ partial ]\n");
		return;
	}

	if (!Q_strcasecmp(args[1], "log") && iArgs >= 3)
	{
		char fn[256];
		Q_snprintf(fn, sizeof(fn), "%s", args[2]);
		f = FileSystem()->Open(fn, "wb", nullptr, 0);
		if (f)
		{
			bLogging = true;
		}
		else
		{
			DevMsg(eDLL_T::ENGINE, "Couldn't open '%s' for writing!\n", fn);
			return;
		}

		if (iArgs == 4)
		{
			partial = args[3];
			ipLen = Q_strlen(partial);
		}
	}
	else
	{
		partial = args[1];
		ipLen = Q_strlen(partial);
	}

	// Banner
	DevMsg(eDLL_T::ENGINE, "cvar list\n--------------\n");

	CUtlRBTree< ConCommandBase* > sorted(0, 0, ConCommandBaseLessFunc);
	CCvar::CCVarIteratorInternal* itint = g_pCVar->FactoryInternalIterator();

	for (itint->SetFirst(); itint->IsValid(); itint->Next()) // Loop through all instances.
	{
		var = itint->Get();

		if (!var->IsFlagSet(FCVAR_DEVELOPMENTONLY) &&
			!var->IsFlagSet(FCVAR_HIDDEN))
		{
			bool print = false;

			if (partial)  // Partial string searching?
			{
				if (!Q_strncasecmp(var->GetName(), partial, ipLen))
				{
					print = true;
				}
			}
			else
			{
				print = true;
			}

			if (print)
			{
				sorted.Insert(var);
			}
		}
	}

	MemAllocSingleton()->Free(itint);

	if (bLogging)
	{
		PrintListHeader(f);
	}
	for (int i = sorted.FirstInorder(); i != sorted.InvalidIndex(); i = sorted.NextInorder(i))
	{
		var = sorted[i];
		if (var->IsCommand())
		{
			PrintCommand((ConCommand*)var, bLogging, f);
		}
		else
		{
			PrintCvar((ConVar*)var, bLogging, f);
		}
	}


	// Show total and syntax help...
	if (partial && partial[0])
	{
		DevMsg(eDLL_T::ENGINE, "--------------\n%3i convars/concommands for [%s]\n", sorted.Count(), partial);
	}
	else
	{
		DevMsg(eDLL_T::ENGINE, "--------------\n%3i total convars/concommands\n", sorted.Count());
	}

	if (bLogging)
	{
		FileSystem()->Close(f);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCvarUtilities::CvarHelp(const CCommand& args)
{
	const char* search;
	ConCommandBase* var;

	if (args.ArgC() != 2)
	{
		DevMsg(eDLL_T::ENGINE, "Usage:  help <cvarname>\n");
		return;
	}

	// Get name of var to find
	search = args[1];

	// Search for it
	var = g_pCVar->FindCommandBase(search);
	if (!var)
	{
		DevMsg(eDLL_T::ENGINE, "help:  no cvar or command named %s\n", search);
		return;
	}

	// Show info
	ConVar_PrintDescription(var);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCvarUtilities::CvarDifferences(const CCommand& args)
{
	CCvar::CCVarIteratorInternal* itint = g_pCVar->FactoryInternalIterator();
	int i = 0;

	for (itint->SetFirst(); itint->IsValid(); itint->Next()) // Loop through all instances.
	{
		ConCommandBase* pCommandBase = itint->Get();

		if (!pCommandBase->IsCommand() &&
			!pCommandBase->IsFlagSet(FCVAR_HIDDEN))
		{
			ConVar* pConVar = reinterpret_cast<ConVar*>(pCommandBase);

			if (V_strcmp(pConVar->GetString(), "FCVAR_NEVER_AS_STRING") != NULL)
			{
				if (V_stricmp(pConVar->GetString(), pConVar->GetDefault()) != NULL)
				{
					ConVar_PrintDescription(pConVar);
					i++;
				}
			}
		}
	}

	MemAllocSingleton()->Free(itint);
	DevMsg(eDLL_T::ENGINE, "--------------\n%3i changed convars\n", i);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCvarUtilities::CvarFindFlags_f(const CCommand& args)
{
	if (args.ArgC() < 2)
	{
		DevMsg(eDLL_T::ENGINE, "Usage:  findflags <string>\n");
		DevMsg(eDLL_T::ENGINE, "Available flags to search for: \n");

		for (int i = 0; i < ARRAYSIZE(g_ConVarFlags); i++)
		{
			DevMsg(eDLL_T::ENGINE, "   - %s\n", g_ConVarFlags[i].desc);
		}
		return;
	}

	// Get substring to find
	const char* search = args[1];
	ConCommandBase* var;

	// Loop through vars and print out findings
	CCvar::CCVarIteratorInternal* itint = g_pCVar->FactoryInternalIterator();

	for (itint->SetFirst(); itint->IsValid(); itint->Next())
	{
		var = itint->Get();

		if (!var->IsFlagSet(FCVAR_DEVELOPMENTONLY) || !var->IsFlagSet(FCVAR_HIDDEN))
		{
			for (int i = 0; i < ARRAYSIZE(g_ConVarFlags); i++)
			{
				if (var->IsFlagSet(g_ConVarFlags[i].bit))
				{
					if (V_stristr(g_ConVarFlags[i].desc, search))
					{
						ConVar_PrintDescription(var);
					}
				}
			}
		}
	}

	MemAllocSingleton()->Free(itint);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CCvarUtilities::CvarFindFlagsCompletionCallback(const char* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH])
{
	int flagC = ARRAYSIZE(g_ConVarFlags);
	char const* pcmd = "findflags ";
	int len = Q_strlen(partial);

	if (len < Q_strlen(pcmd))
	{
		int i = 0;
		for (; i < MIN(flagC, COMMAND_COMPLETION_MAXITEMS); i++)
		{
			Q_snprintf(commands[i], sizeof(commands[i]), "%s %s", pcmd, g_ConVarFlags[i].desc);
			Q_strlower(commands[i]);
		}
		return i;
	}

	char const* pSub = partial + Q_strlen(pcmd);
	int nSubLen = Q_strlen(pSub);

	int values = 0;
	for (int i = 0; i < flagC; ++i)
	{
		if (Q_strnicmp(g_ConVarFlags[i].desc, pSub, nSubLen))
			continue;

		Q_snprintf(commands[values], sizeof(commands[values]), "%s %s", pcmd, g_ConVarFlags[i].desc);
		Q_strlower(commands[values]);
		++values;

		if (values >= COMMAND_COMPLETION_MAXITEMS)
			break;
	}
	return values;
}

//-----------------------------------------------------------------------------
// Purpose: registers input commands.
// Input  : *pszCommandName - 
//-----------------------------------------------------------------------------
ConCommandBase* CCvar::RegisterConCommand(ConCommandBase* pCommandToRemove)
{
	const static int index = 9;
	return CallVFunc<ConCommandBase*>(index, this, pCommandToRemove);
}

//-----------------------------------------------------------------------------
// Purpose: unregisters input commands.
// Input  : *pszCommandName - 
//-----------------------------------------------------------------------------
ConCommandBase* CCvar::UnregisterConCommand(ConCommandBase* pCommandToRemove)
{
	const static int index = 10;
	return CallVFunc<ConCommandBase*>(index, this, pCommandToRemove);
}

//-----------------------------------------------------------------------------
// Purpose: finds base commands.
// Input  : *pszCommandName - 
//-----------------------------------------------------------------------------
ConCommandBase* CCvar::FindCommandBase(const char* pszCommandName)
{
	const static int index = 14;
	return CallVFunc<ConCommandBase*>(index, this, pszCommandName);
}

//-----------------------------------------------------------------------------
// Purpose: finds ConVars.
// Input  : *pszVarName - 
//-----------------------------------------------------------------------------
ConVar* CCvar::FindVar(const char* pszVarName)
{
	const static int index = 16;
	return CallVFunc<ConVar*>(index, this, pszVarName);
}

//-----------------------------------------------------------------------------
// Purpose: finds ConCommands.
// Input  : *pszCommandName - 
//-----------------------------------------------------------------------------
ConCommand* CCvar::FindCommand(const char* pszCommandName)
{
	const static int index = 18;
	return CallVFunc<ConCommand*>(index, this, pszCommandName);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCvar::CallGlobalChangeCallbacks(ConVar* pConVar, const char* pOldString)
{
	const static int index = 23;
	CallVFunc<void>(index, this, pConVar, pOldString);
}

//-----------------------------------------------------------------------------
// Purpose: deal with queued material system ConVars
//-----------------------------------------------------------------------------
bool CCvar::IsMaterialThreadSetAllowed(void)
{
	const static int index = 35;
	return CallVFunc<bool>(index, this);
}
void CCvar::QueueMaterialThreadSetValue(ConVar* pConVar, float flValue)
{
	const static int index = 36;
	CallVFunc<void>(index, this, pConVar, flValue);
}
void CCvar::QueueMaterialThreadSetValue(ConVar* pConVar, int nValue)
{
	const static int index = 37;
	CallVFunc<void>(index, this, pConVar, nValue);
}
void CCvar::QueueMaterialThreadSetValue(ConVar* pConVar, const char* pValue)
{
	const static int index = 38;
	CallVFunc<void>(index, this, pConVar, pValue);
}

//-----------------------------------------------------------------------------
// Purpose: iterates over all ConVars
//-----------------------------------------------------------------------------
CCvar::CCVarIteratorInternal* CCvar::FactoryInternalIterator(void)
{
	const static int index = 41;
	return CallVFunc<CCVarIteratorInternal*>(index, this);
}

//-----------------------------------------------------------------------------
// Purpose: returns all ConVars
//-----------------------------------------------------------------------------
unordered_map<string, ConCommandBase*> CCvar::DumpToMap(void)
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

	MemAllocSingleton()->Free(itint);

	return allConVars;
}

///////////////////////////////////////////////////////////////////////////////
CCvar* g_pCVar = nullptr;


//-----------------------------------------------------------------------------
// Console command hash data structure
//-----------------------------------------------------------------------------
CConCommandHash::CConCommandHash()
{
	Purge(true);
}

CConCommandHash::~CConCommandHash()
{
	Purge(false);
}

void CConCommandHash::Purge(bool bReinitialize)
{
	m_aBuckets.Purge();
	m_aDataPool.Purge();
	if (bReinitialize)
	{
		Init();
	}
}

// Initialize.
void CConCommandHash::Init(void)
{
	// kNUM_BUCKETS must be a power of two.
	COMPILE_TIME_ASSERT((kNUM_BUCKETS & (kNUM_BUCKETS - 1)) == 0);

	// Set the bucket size.
	m_aBuckets.SetSize(kNUM_BUCKETS);
	for (int iBucket = 0; iBucket < kNUM_BUCKETS; ++iBucket)
	{
		m_aBuckets[iBucket] = m_aDataPool.InvalidIndex();
	}

	// Calculate the grow size.
	int nGrowSize = 4 * kNUM_BUCKETS;
	m_aDataPool.SetGrowSize(nGrowSize);
}

//-----------------------------------------------------------------------------
// Purpose: Insert data into the hash table given its key (unsigned int), 
//			WITH a check to see if the element already exists within the hash.
//-----------------------------------------------------------------------------
CConCommandHash::CCommandHashHandle_t CConCommandHash::Insert(ConCommandBase* cmd)
{
	// Check to see if that key already exists in the buckets (should be unique).
	CCommandHashHandle_t hHash = Find(cmd);
	if (hHash != InvalidHandle())
		return hHash;

	return FastInsert(cmd);
}
//-----------------------------------------------------------------------------
// Purpose: Insert data into the hash table given its key (unsigned int),
//          WITHOUT a check to see if the element already exists within the hash.
//-----------------------------------------------------------------------------
CConCommandHash::CCommandHashHandle_t CConCommandHash::FastInsert(ConCommandBase* cmd)
{
	// Get a new element from the pool.
	intptr_t iHashData = m_aDataPool.Alloc(true);
	HashEntry_t* RESTRICT pHashData = &m_aDataPool[iHashData];
	if (!pHashData)
		return InvalidHandle();

	HashKey_t key = Hash(cmd);

	// Add data to new element.
	pHashData->m_uiKey = key;
	pHashData->m_Data = cmd;

	// Link element.
	int iBucket = key & kBUCKETMASK; // HashFuncs::Hash( uiKey, m_uiBucketMask );
	m_aDataPool.LinkBefore(m_aBuckets[iBucket], iHashData);
	m_aBuckets[iBucket] = iHashData;

	return iHashData;
}

//-----------------------------------------------------------------------------
// Purpose: Remove a given element from the hash.
//-----------------------------------------------------------------------------
void CConCommandHash::Remove(CCommandHashHandle_t hHash) RESTRICT
{
	HashEntry_t* RESTRICT entry = &m_aDataPool[hHash];
	HashKey_t iBucket = entry->m_uiKey & kBUCKETMASK;
	if (m_aBuckets[iBucket] == hHash)
	{
		// It is a bucket head.
		m_aBuckets[iBucket] = m_aDataPool.Next(hHash);
	}
	else
	{
		// Not a bucket head.
		m_aDataPool.Unlink(hHash);
	}

	// Remove the element.
	m_aDataPool.Remove(hHash);
}

//-----------------------------------------------------------------------------
// Purpose: Remove all elements from the hash
//-----------------------------------------------------------------------------
void CConCommandHash::RemoveAll(void)
{
	m_aBuckets.RemoveAll();
	m_aDataPool.RemoveAll();
}

//-----------------------------------------------------------------------------
// Find hash entry corresponding to a string name
//-----------------------------------------------------------------------------
CConCommandHash::CCommandHashHandle_t CConCommandHash::Find(const char* name, HashKey_t hashkey) const RESTRICT
{
	// hash the "key" - get the correct hash table "bucket"
	int iBucket = hashkey & kBUCKETMASK;

	for (datapool_t::IndexLocalType_t iElement = m_aBuckets[iBucket]; iElement != m_aDataPool.InvalidIndex(); iElement = m_aDataPool.Next(iElement))
	{
		const HashEntry_t& element = m_aDataPool[iElement];
		if (element.m_uiKey == hashkey && // if hashes of strings match,
			Q_stricmp(name, element.m_Data->GetName()) == 0) // then test the actual strings
		{
			return iElement;
		}
	}

	// found nuffink
	return InvalidHandle();
}

//-----------------------------------------------------------------------------
// Find a command in the hash.
//-----------------------------------------------------------------------------
CConCommandHash::CCommandHashHandle_t CConCommandHash::Find(const ConCommandBase* cmd) const RESTRICT
{
	// Set this #if to 1 if the assert at bottom starts whining --
	// that indicates that a console command is being double-registered,
	// or something similarly non-fatally bad. With this #if 1, we'll search
	// by name instead of by pointer, which is more robust in the face
	// of double registered commands, but obviously slower.
#if 0 
	return Find(cmd->GetName());
#else
	HashKey_t hashkey = Hash(cmd);
	int iBucket = hashkey & kBUCKETMASK;

	// hunt through all entries in that bucket
	for (datapool_t::IndexLocalType_t iElement = m_aBuckets[iBucket]; iElement != m_aDataPool.InvalidIndex(); iElement = m_aDataPool.Next(iElement))
	{
		const HashEntry_t& element = m_aDataPool[iElement];
		if (element.m_uiKey == hashkey && // if the hashes match... 
			element.m_Data == cmd) // and the pointers...
		{
			// in debug, test to make sure we don't have commands under the same name
			// or something goofy like that
			Assert(iElement == Find(cmd->GetName()),
				"ConCommand %s had two entries in the hash!", cmd->GetName());

			// return this element
			return iElement;
		}
	}

	// found nothing.
#ifdef DBGFLAG_ASSERT // double check against search by name
	CCommandHashHandle_t dbghand = Find(cmd->GetName());

	Assert(InvalidHandle() == dbghand,
		"ConCommand %s couldn't be found by pointer, but was found by name!", cmd->GetName());
#endif
	return InvalidHandle();
#endif
}


//#ifdef _DEBUG
// Dump a report to MSG
void CConCommandHash::Report(void)
{
	DevMsg(eDLL_T::ENGINE, "Console command hash bucket load:\n");
	int total = 0;
	for (int iBucket = 0; iBucket < kNUM_BUCKETS; ++iBucket)
	{
		int count = 0;
		CCommandHashHandle_t iElement = m_aBuckets[iBucket]; // get the head of the bucket
		while (iElement != m_aDataPool.InvalidIndex())
		{
			++count;
			iElement = m_aDataPool.Next(iElement);
		}

		DevMsg(eDLL_T::ENGINE, "%d: %d\n", iBucket, count);
		total += count;
	}

	DevMsg(eDLL_T::ENGINE, "\tAverage: %.1f\n", total / ((float)(kNUM_BUCKETS)));
}
//#endif
