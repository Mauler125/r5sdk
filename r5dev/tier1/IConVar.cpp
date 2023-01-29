//=============================================================================//
//
// Purpose: Console Variables
//
//=============================================================================//

#include "core/stdafx.h"
#include "tier0/tslist.h"
#include "tier0/memstd.h"
#include "tier1/IConVar.h"
#include "tier1/cvar.h"
#include "tier1/utlvector.h"
#include "mathlib/fbits.h"
#include "vstdlib/callback.h"
#include "public/const.h"
#include "public/iconvar.h"
#include "public/iconcommand.h"

//-----------------------------------------------------------------------------
// Purpose: create
//-----------------------------------------------------------------------------
ConVar* ConVar::Create(const char* pszName, const char* pszDefaultValue, int nFlags, const char* pszHelpString, bool bMin, float fMin, bool bMax, float fMax, FnChangeCallback_t pCallback, const char* pszUsageString)
{
	ConVar* pNewConVar = MemAllocSingleton()->Alloc<ConVar>(sizeof(ConVar)); // Allocate new memory with StdMemAlloc else we crash.
	memset(pNewConVar, '\0', sizeof(ConVar));                                // Set all to null.

	pNewConVar->m_pConCommandBaseVFTable = g_pConVarVFTable.RCast<IConCommandBase*>();
	pNewConVar->m_pIConVarVFTable = g_pIConVarVFTable.RCast<IConVar*>();

	v_ConVar_Register(pNewConVar, pszName, pszDefaultValue, nFlags, pszHelpString, bMin, fMin, bMax, fMax, pCallback, pszUsageString);

	return pNewConVar;
}

//-----------------------------------------------------------------------------
// Purpose: construct/allocate
//-----------------------------------------------------------------------------
ConVar::ConVar(void)
	: m_pIConVarVFTable(nullptr)
	, m_pParent(nullptr)
	, m_pszDefaultValue(nullptr)
	, m_bHasMin(false)
	, m_fMinVal(0.f)
	, m_bHasMax(false)
	, m_fMaxVal(0.f)
{
	memset(&m_Value, '\0', sizeof(CVValue_t));
}

//-----------------------------------------------------------------------------
// Purpose: destructor
//-----------------------------------------------------------------------------
ConVar::~ConVar(void)
{
	if (m_Value.m_pszString)
	{
		MemAllocSingleton()->Free(m_Value.m_pszString);
		m_Value.m_pszString = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: initialize ConVar's
//-----------------------------------------------------------------------------
void ConVar::Init(void)
{
	//-------------------------------------------------------------------------
	// ENGINE                                                                 |
	hostdesc                       = ConVar::Create("hostdesc", "", FCVAR_RELEASE, "Host game server description.", false, 0.f, false, 0.f, nullptr, nullptr);
	sdk_fixedframe_tickinterval    = ConVar::Create("sdk_fixedframe_tickinterval", "0.02", FCVAR_RELEASE, "The tick interval used by the SDK fixed frame.", false, 0.f, false, 0.f, nullptr, nullptr);
	staticProp_defaultBuildFrustum = ConVar::Create("staticProp_defaultBuildFrustum", "0", FCVAR_DEVELOPMENTONLY, "Use the old solution for building static prop frustum culling.", false, 0.f, false, 0.f, nullptr, nullptr);

	cm_unset_all_cmdquery   = ConVar::Create("cm_unset_all_cmdquery"  , "0", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Returns false on every ConVar/ConCommand query ( !warning! ).", false, 0.f, false, 0.f, nullptr, nullptr);

	rcon_address  = ConVar::Create("rcon_address",  "localhost", FCVAR_SERVER_CANNOT_QUERY | FCVAR_DONTRECORD | FCVAR_RELEASE, "Remote server access address.", false, 0.f, false, 0.f, nullptr, nullptr);
	rcon_password = ConVar::Create("rcon_password", ""         , FCVAR_SERVER_CANNOT_QUERY | FCVAR_DONTRECORD | FCVAR_RELEASE, "Remote server access password (rcon is disabled if empty).", false, 0.f, false, 0.f, &RCON_PasswordChanged_f, nullptr);

	r_debug_overlay_nodecay        = ConVar::Create("r_debug_overlay_nodecay"       , "0", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "Keeps all debug overlays alive regardless of their lifetime. Use command 'clear_debug_overlays' to clear everything.", false, 0.f, false, 0.f, nullptr, nullptr);
	r_debug_overlay_invisible      = ConVar::Create("r_debug_overlay_invisible"     , "1", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "Show invisible debug overlays (alpha < 1 = 255).", false, 0.f, false, 0.f, nullptr, nullptr);
	r_debug_overlay_wireframe      = ConVar::Create("r_debug_overlay_wireframe"     , "1", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "Use wireframe in debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	r_debug_draw_depth_test        = ConVar::Create("r_debug_draw_depth_test"       , "1", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "Toggle depth test for other debug draw functionality.", false, 0.f, false, 0.f, nullptr, nullptr);
	r_drawWorldMeshes              = ConVar::Create("r_drawWorldMeshes"             , "1", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "Render world meshes.", false, 0.f, false, 0.f, nullptr, nullptr);
	r_drawWorldMeshesDepthOnly     = ConVar::Create("r_drawWorldMeshesDepthOnly"    , "1", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "Render world meshes (depth only).", false, 0.f, false, 0.f, nullptr, nullptr);
	r_drawWorldMeshesDepthAtTheEnd = ConVar::Create("r_drawWorldMeshesDepthAtTheEnd", "1", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "Render world meshes (depth at the end).", false, 0.f, false, 0.f, nullptr, nullptr);
	//-------------------------------------------------------------------------
	// SERVER                                                                 |
#ifndef CLIENT_DLL
	ai_ainDumpOnLoad             = ConVar::Create("ai_ainDumpOnLoad"            , "0", FCVAR_DEVELOPMENTONLY, "Dumps AIN data from node graphs loaded from the disk on load.", false, 0.f, false, 0.f, nullptr, nullptr);
	ai_ainDebugConnect           = ConVar::Create("ai_ainDebugConnect"          , "0", FCVAR_DEVELOPMENTONLY, "Debug AIN node connections.", false, 0.f, false, 0.f, nullptr, nullptr);
	ai_script_nodes_draw_range   = ConVar::Create("ai_script_nodes_draw_range"  , "0", FCVAR_DEVELOPMENTONLY, "Debug draw AIN script nodes ranging from shift index to this cvar.", false, 0.f, false, 0.f, nullptr, nullptr);
	ai_script_nodes_draw_nearest = ConVar::Create("ai_script_nodes_draw_nearest", "1", FCVAR_DEVELOPMENTONLY, "Debug draw AIN script node links to nearest node (build order is used if null).", false, 0.f, false, 0.f, nullptr, nullptr);

	navmesh_always_reachable   = ConVar::Create("navmesh_always_reachable"   , "0"    , FCVAR_DEVELOPMENTONLY, "Marks goal poly from agent poly as reachable regardless of table data ( !slower! ).", false, 0.f, false, 0.f, nullptr, nullptr);
	navmesh_debug_type         = ConVar::Create("navmesh_debug_type"         , "0"    , FCVAR_DEVELOPMENTONLY, "NavMesh debug draw hull index.", true, 0.f, true, 4.f, nullptr, "0 = small, 1 = med_short, 2 = medium, 3 = large, 4 = extra large");
	navmesh_debug_tile_range   = ConVar::Create("navmesh_debug_tile_range"   , "0"    , FCVAR_DEVELOPMENTONLY, "NavMesh debug draw tiles ranging from shift index to this cvar.", true, 0.f, false, 0.f, nullptr, nullptr);
	navmesh_debug_camera_range = ConVar::Create("navmesh_debug_camera_range" , "2000" , FCVAR_DEVELOPMENTONLY, "Only debug draw tiles within this distance from camera origin.", true, 0.f, false, 0.f, nullptr, nullptr);
#ifndef DEDICATED
	navmesh_draw_bvtree            = ConVar::Create("navmesh_draw_bvtree"            , "-1", FCVAR_DEVELOPMENTONLY, "Draws the BVTree of the NavMesh tiles.", false, 0.f, false, 0.f, nullptr, "Index: > 0 && < mesh->m_tileCount");
	navmesh_draw_portal            = ConVar::Create("navmesh_draw_portal"            , "-1", FCVAR_DEVELOPMENTONLY, "Draws the portal of the NavMesh tiles.", false, 0.f, false, 0.f, nullptr, "Index: > 0 && < mesh->m_tileCount");
	navmesh_draw_polys             = ConVar::Create("navmesh_draw_polys"             , "-1", FCVAR_DEVELOPMENTONLY, "Draws the polys of the NavMesh tiles.", false, 0.f, false, 0.f, nullptr, "Index: > 0 && < mesh->m_tileCount");
	navmesh_draw_poly_bounds       = ConVar::Create("navmesh_draw_poly_bounds"       , "-1", FCVAR_DEVELOPMENTONLY, "Draws the bounds of the NavMesh polys.", false, 0.f, false, 0.f, nullptr, "Index: > 0 && < mesh->m_tileCount");
	navmesh_draw_poly_bounds_inner = ConVar::Create("navmesh_draw_poly_bounds_inner" , "0" , FCVAR_DEVELOPMENTONLY, "Draws the inner bounds of the NavMesh polys (requires navmesh_draw_poly_bounds).", false, 0.f, false, 0.f, nullptr, "Index: > 0 && < mesh->m_tileCount");
#endif // !DEDICATED
	sv_showconnecting  = ConVar::Create("sv_showconnecting" , "1", FCVAR_RELEASE, "Logs information about the connecting client to the console.", false, 0.f, false, 0.f, nullptr, nullptr);
	sv_globalBanlist   = ConVar::Create("sv_globalBanlist"  , "1", FCVAR_RELEASE, "Determines whether or not to use the global banned list.", false, 0.f, false, 0.f, nullptr, "0 = Disable, 1 = Enable.");
	sv_pylonVisibility = ConVar::Create("sv_pylonVisibility", "0", FCVAR_RELEASE, "Determines the visibility to the Pylon master server.", false, 0.f, false, 0.f, nullptr, "0 = Offline, 1 = Hidden, 2 = Public.");
	sv_pylonRefreshRate   = ConVar::Create("sv_pylonRefreshRate"  , "5.0", FCVAR_RELEASE, "Pylon host refresh rate (seconds).", true, 2.f, true, 8.f, nullptr, nullptr);
	sv_banlistRefreshRate = ConVar::Create("sv_banlistRefreshRate", "1.0", FCVAR_RELEASE, "Banned list refresh rate (seconds).", true, 1.f, false, 0.f, nullptr, nullptr);
	sv_statusRefreshRate  = ConVar::Create("sv_statusRefreshRate" , "0.5", FCVAR_RELEASE, "Server status refresh rate (seconds).", false, 0.f, false, 0.f, nullptr, nullptr);
	sv_autoReloadRate     = ConVar::Create("sv_autoReloadRate"    , "0"  , FCVAR_RELEASE, "Time in seconds between each server auto-reload (disabled if null). ", true, 0.f, false, 0.f, nullptr, nullptr);
	sv_quota_stringCmdsPerSecond = ConVar::Create("sv_quota_stringCmdsPerSecond", "16", FCVAR_RELEASE, "How many string commands per second clients are allowed to submit, 0 to disallow all string commands.", true, 0.f, false, 0.f, nullptr, nullptr);
	sv_simulateBots = ConVar::Create("sv_simulateBots", "1", FCVAR_RELEASE, "Simulate user commands for bots on the server.", true, 0.f, false, 0.f, nullptr, nullptr);

	sv_rcon_debug       = ConVar::Create("sv_rcon_debug"      , "0" , FCVAR_RELEASE, "Show rcon debug information ( !slower! ).", false, 0.f, false, 0.f, nullptr, nullptr);
	sv_rcon_sendlogs    = ConVar::Create("sv_rcon_sendlogs"   , "0" , FCVAR_RELEASE, "Network console logs to connected and authenticated sockets.", false, 0.f, false, 0.f, nullptr, nullptr);
	sv_rcon_banpenalty  = ConVar::Create("sv_rcon_banpenalty" , "10", FCVAR_RELEASE, "Number of minutes to ban users who fail rcon authentication.", false, 0.f, false, 0.f, nullptr, nullptr);
	sv_rcon_maxfailures = ConVar::Create("sv_rcon_maxfailures", "10", FCVAR_RELEASE, "Max number of times a user can fail rcon authentication before being banned.", true, 1.f, false, 0.f, nullptr, nullptr);
	sv_rcon_maxignores  = ConVar::Create("sv_rcon_maxignores" , "15", FCVAR_RELEASE, "Max number of times a user can ignore the no-auth message before being banned.", true, 1.f, false, 0.f, nullptr, nullptr);
	sv_rcon_maxsockets  = ConVar::Create("sv_rcon_maxsockets" , "32", FCVAR_RELEASE, "Max number of accepted sockets before the server starts closing redundant sockets.", true, 1.f, false, 0.f, nullptr, nullptr);
	sv_rcon_whitelist_address = ConVar::Create("sv_rcon_whitelist_address", "", FCVAR_RELEASE, "This address is not considered a 'redundant' socket and will never be banned for failed authentication attempts.", false, 0.f, false, 0.f, nullptr, "Format: '::ffff:127.0.0.1'");
#endif // !CLIENT_DLL
#if !defined (GAMEDLL_S0) && !defined (GAMEDLL_S1)
	bhit_depth_test = ConVar::Create("bhit_depth_test", "0", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Use depth test for bullet ray trace overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	bhit_abs_origin = ConVar::Create("bhit_abs_origin", "1", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Draw entity's predicted abs origin upon bullet impact for trajectory debugging (requires 'r_visualizetraces' to be set!).", false, 0.f, false, 0.f, nullptr, nullptr);
#endif // !GAMEDLL_S0 && !GAMEDLL_S1
	//-------------------------------------------------------------------------
	// CLIENT                                                                 |
#ifndef DEDICATED
	cl_rcon_request_sendlogs = ConVar::Create("cl_rcon_request_sendlogs", "1" , FCVAR_RELEASE, "Request the rcon server to send console logs on connect.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_quota_stringCmdsPerSecond = ConVar::Create("cl_quota_stringCmdsPerSecond", "16" , FCVAR_RELEASE, "How many string commands per second user is allowed to submit, 0 to allow all submissions.", true, 0.f, false, 0.f, nullptr, nullptr);

	cl_showhoststats      = ConVar::Create("cl_showhoststats"     , "0", FCVAR_DEVELOPMENTONLY, "Host speeds debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_hoststats_invert_x = ConVar::Create("cl_hoststats_invert_x", "0", FCVAR_DEVELOPMENTONLY, "Inverts the X offset for host speeds debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_hoststats_invert_y = ConVar::Create("cl_hoststats_invert_y", "0", FCVAR_DEVELOPMENTONLY, "Inverts the Y offset for host speeds debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_hoststats_offset_x = ConVar::Create("cl_hoststats_offset_x", "10", FCVAR_DEVELOPMENTONLY, "X offset for host speeds debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_hoststats_offset_y = ConVar::Create("cl_hoststats_offset_y", "10", FCVAR_DEVELOPMENTONLY, "Y offset for host speeds debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);

	cl_showsimstats      = ConVar::Create("cl_showsimstats"     , "0"  , FCVAR_DEVELOPMENTONLY, "Shows the tick counter for the server/client simulation and the render frame.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_simstats_invert_x = ConVar::Create("cl_simstats_invert_x", "1"  , FCVAR_DEVELOPMENTONLY, "Inverts the X offset for simulation debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_simstats_invert_y = ConVar::Create("cl_simstats_invert_y", "1"  , FCVAR_DEVELOPMENTONLY, "Inverts the Y offset for simulation debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_simstats_offset_x = ConVar::Create("cl_simstats_offset_x", "650", FCVAR_DEVELOPMENTONLY, "X offset for simulation debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_simstats_offset_y = ConVar::Create("cl_simstats_offset_y", "120", FCVAR_DEVELOPMENTONLY, "Y offset for simulation debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);

	cl_showgpustats      = ConVar::Create("cl_showgpustats"     , "0", FCVAR_DEVELOPMENTONLY, "Texture streaming debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_gpustats_invert_x = ConVar::Create("cl_gpustats_invert_x", "1", FCVAR_DEVELOPMENTONLY, "Inverts the X offset for texture streaming debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_gpustats_invert_y = ConVar::Create("cl_gpustats_invert_y", "1", FCVAR_DEVELOPMENTONLY, "Inverts the Y offset for texture streaming debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_gpustats_offset_x = ConVar::Create("cl_gpustats_offset_x", "650", FCVAR_DEVELOPMENTONLY, "X offset for texture streaming debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_gpustats_offset_y = ConVar::Create("cl_gpustats_offset_y", "105", FCVAR_DEVELOPMENTONLY, "Y offset for texture streaming debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);

	cl_showmaterialinfo      = ConVar::Create("cl_showmaterialinfo"     , "0"  , FCVAR_DEVELOPMENTONLY, "Draw info for the material under the crosshair on screen.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_materialinfo_offset_x = ConVar::Create("cl_materialinfo_offset_x", "0"  , FCVAR_DEVELOPMENTONLY, "X offset for material debug info overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_materialinfo_offset_y = ConVar::Create("cl_materialinfo_offset_y", "420", FCVAR_DEVELOPMENTONLY, "Y offset for material debug info overlay.", false, 0.f, false, 0.f, nullptr, nullptr);

	con_drawnotify = ConVar::Create("con_drawnotify", "0", FCVAR_RELEASE, "Draws the RUI console to the hud.", false, 0.f, false, 0.f, nullptr, nullptr);
	con_notifylines     = ConVar::Create("con_notifylines"    , "3" , FCVAR_MATERIAL_SYSTEM_THREAD, "Number of console lines to overlay for debugging.", true, 1.f, false, 0.f, nullptr, nullptr);
	con_notifytime      = ConVar::Create("con_notifytime"     , "6" , FCVAR_MATERIAL_SYSTEM_THREAD, "How long to display recent console text to the upper part of the game window.", false, 1.f, false, 50.f, nullptr, nullptr);

	con_notify_invert_x = ConVar::Create("con_notify_invert_x", "0" , FCVAR_MATERIAL_SYSTEM_THREAD, "Inverts the X offset for RUI console overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	con_notify_invert_y = ConVar::Create("con_notify_invert_y", "0" , FCVAR_MATERIAL_SYSTEM_THREAD, "Inverts the Y offset for RUI console overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	con_notify_offset_x = ConVar::Create("con_notify_offset_x", "10", FCVAR_MATERIAL_SYSTEM_THREAD, "X offset for RUI console overlay.", false, 1.f, false, 50.f, nullptr, nullptr);
	con_notify_offset_y = ConVar::Create("con_notify_offset_y", "10", FCVAR_MATERIAL_SYSTEM_THREAD, "Y offset for RUI console overlay.", false, 1.f, false, 50.f, nullptr, nullptr);

	con_notify_script_server_clr  = ConVar::Create("con_notify_script_server_clr", "130 120 245 255", FCVAR_MATERIAL_SYSTEM_THREAD, "Script SERVER VM RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	con_notify_script_client_clr  = ConVar::Create("con_notify_script_client_clr", "117 116 139 255", FCVAR_MATERIAL_SYSTEM_THREAD, "Script CLIENT VM RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	con_notify_script_ui_clr      = ConVar::Create("con_notify_script_ui_clr"    , "200 110 110 255", FCVAR_MATERIAL_SYSTEM_THREAD, "Script UI VM RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);

	con_notify_native_server_clr = ConVar::Create("con_notify_native_server_clr", "20 50 248 255"  , FCVAR_MATERIAL_SYSTEM_THREAD, "Native SERVER RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	con_notify_native_client_clr = ConVar::Create("con_notify_native_client_clr", "70 70 70 255"   , FCVAR_MATERIAL_SYSTEM_THREAD, "Native CLIENT RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	con_notify_native_ui_clr     = ConVar::Create("con_notify_native_ui_clr"    , "200 60 60 255"  , FCVAR_MATERIAL_SYSTEM_THREAD, "Native UI RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	con_notify_native_engine_clr = ConVar::Create("con_notify_native_engine_clr", "255 255 255 255", FCVAR_MATERIAL_SYSTEM_THREAD, "Native engine RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	con_notify_native_fs_clr     = ConVar::Create("con_notify_native_fs_clr"    , "0 100 225 255"  , FCVAR_MATERIAL_SYSTEM_THREAD, "Native FileSystem RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	con_notify_native_rtech_clr  = ConVar::Create("con_notify_native_rtech_clr" , "25 120 20 255"  , FCVAR_MATERIAL_SYSTEM_THREAD, "Native RTech RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	con_notify_native_ms_clr     = ConVar::Create("con_notify_native_ms_clr"    , "200 20 180 255" , FCVAR_MATERIAL_SYSTEM_THREAD, "Native MaterialSystem RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	con_notify_native_audio_clr  = ConVar::Create("con_notify_native_audio_clr" , "238 43 10 255"  , FCVAR_MATERIAL_SYSTEM_THREAD, "Native AudioSystem RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	con_notify_native_video_clr  = ConVar::Create("con_notify_native_video_clr" , "115 0 235 255"  , FCVAR_MATERIAL_SYSTEM_THREAD, "Native VideoSystem RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);

	con_notify_netcon_clr  = ConVar::Create("con_notify_netcon_clr" , "255 255 255 255", FCVAR_MATERIAL_SYSTEM_THREAD, "Net console RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	con_notify_common_clr  = ConVar::Create("con_notify_common_clr" , "255 140 80 255" , FCVAR_MATERIAL_SYSTEM_THREAD, "Common RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);

	con_notify_warning_clr = ConVar::Create("con_notify_warning_clr", "180 180 20 255", FCVAR_MATERIAL_SYSTEM_THREAD, "Warning RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	con_notify_error_clr   = ConVar::Create("con_notify_error_clr"  , "225 20 20 255" , FCVAR_MATERIAL_SYSTEM_THREAD, "Error RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);

	con_max_lines                 = ConVar::Create("con_max_lines"                , "1024", FCVAR_DEVELOPMENTONLY, "Maximum number of lines in the console before cleanup starts.", true, 1.f, false, 0.f, nullptr, nullptr);
	con_max_history               = ConVar::Create("con_max_history"              , "512" , FCVAR_DEVELOPMENTONLY, "Maximum number of command submission items before history cleanup starts.", true, 0.f, false, 0.f, nullptr, nullptr);
	con_suggestion_limit          = ConVar::Create("con_suggestion_limit"         , "128" , FCVAR_DEVELOPMENTONLY, "Maximum number of suggestions the autocomplete window will show for the console.", true, 0.f, false, 0.f, nullptr, nullptr);
	con_suggestion_showhelptext   = ConVar::Create("con_suggestion_showhelptext"  , "1"   , FCVAR_DEVELOPMENTONLY, "Show CommandBase help text in autocomplete window.", false, 0.f, false, 0.f, nullptr, nullptr);
	con_suggestion_showflags      = ConVar::Create("con_suggestion_showflags"     , "1"   , FCVAR_DEVELOPMENTONLY, "Show CommandBase flags in autocomplete window.", false, 0.f, false, 0.f, nullptr, nullptr);
	con_suggestion_flags_realtime = ConVar::Create("con_suggestion_flags_realtime", "1"   , FCVAR_DEVELOPMENTONLY, "Whether to show compile-time or run-time CommandBase flags.", false, 0.f, false, 0.f, nullptr, nullptr);
#endif // !DEDICATED
	//-------------------------------------------------------------------------
	// FILESYSTEM                                                             |
	fs_warning_level_sdk              = ConVar::Create("fs_warning_level_sdk"                  , "0", FCVAR_DEVELOPMENTONLY, "Set the SDK FileSystem warning level.", false, 0.f, false, 0.f, nullptr, nullptr);
	fs_show_warning_output            = ConVar::Create("fs_show_warning_output"                , "0", FCVAR_DEVELOPMENTONLY, "Logs the FileSystem warnings to the console, filtered by 'fs_warning_level_native' ( !slower! ).", false, 0.f, false, 0.f, nullptr, nullptr);
	fs_packedstore_entryblock_stats   = ConVar::Create("fs_packedstore_entryblock_stats"       , "0", FCVAR_DEVELOPMENTONLY, "Logs the stats of each file entry in the VPK during decompression ( !slower! ).", false, 0.f, false, 0.f, nullptr, nullptr);
	fs_packedstore_workspace          = ConVar::Create("fs_packedstore_workspace"      , "platform/", FCVAR_DEVELOPMENTONLY, "Determines the current VPK workspace.", false, 0.f, false, 0.f, nullptr, nullptr);
	fs_packedstore_compression_level  = ConVar::Create("fs_packedstore_compression_level", "default", FCVAR_DEVELOPMENTONLY, "Determines the VPK compression level.", false, 0.f, false, 0.f, nullptr, "fastest faster default better uber");
	fs_packedstore_max_helper_threads = ConVar::Create("fs_packedstore_max_helper_threads"    , "-1", FCVAR_DEVELOPMENTONLY, "Max # of additional \"helper\" threads to create during compression.", true, -1, true, LZHAM_MAX_HELPER_THREADS, nullptr, "Must range between [-1,LZHAM_MAX_HELPER_THREADS], where -1=max practical.");
	//-------------------------------------------------------------------------
	// MATERIALSYSTEM                                                         |
#ifndef DEDICATED
	mat_showdxoutput   = ConVar::Create("mat_showdxoutput", "0", FCVAR_DEVELOPMENTONLY | FCVAR_MATERIAL_SYSTEM_THREAD, "Shows debug output for the DirectX system.", false, 0.f, false, 0.f, nullptr, nullptr);
	mat_alwaysComplain = ConVar::Create("mat_alwaysComplain", "0", FCVAR_RELEASE | FCVAR_MATERIAL_SYSTEM_THREAD, "Always complain when a material is missing.", false, 0.f, false, 0.f, nullptr, nullptr);
#endif // !DEDICATED
	//-------------------------------------------------------------------------
	// SQUIRREL                                                               |
	sq_showrsonloading   = ConVar::Create("sq_showrsonloading"  , "0", FCVAR_DEVELOPMENTONLY, "Logs all RSON files loaded by the SQVM ( !slower! ).", false, 0.f, false, 0.f, nullptr, nullptr);
	sq_showscriptloading = ConVar::Create("sq_showscriptloading", "0", FCVAR_DEVELOPMENTONLY, "Logs all scripts loaded by the SQVM to be pre-compiled ( !slower! ).", false, 0.f, false, 0.f, nullptr, nullptr);
	sq_showvmoutput      = ConVar::Create("sq_showvmoutput"     , "0", FCVAR_RELEASE, "Prints the VM output to the console ( !slower! ).", false, 0.f, false, 0.f, nullptr, "1 = Log to file. 2 = 1 + log to game console. 3 = 1 + 2 + log to overhead console.");
	sq_showvmwarning     = ConVar::Create("sq_showvmwarning"    , "0", FCVAR_RELEASE, "Prints the VM warning output to the console ( !slower! ).", false, 0.f, false, 0.f, nullptr, "1 = Log to file. 2 = 1 + log to game console and overhead console.");
	//-------------------------------------------------------------------------
	// NETCHANNEL                                                             |
	net_tracePayload           = ConVar::Create("net_tracePayload"          , "0", FCVAR_DEVELOPMENTONLY                    , "Log the payload of the send/recv datagram to a file on the disk.", false, 0.f, false, 0.f, nullptr, nullptr);
	net_encryptionEnable       = ConVar::Create("net_encryptionEnable"      , "1", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED , "Use AES encryption on game packets.", false, 0.f, false, 0.f, nullptr, nullptr);
	net_useRandomKey           = ConVar::Create("net_useRandomKey"          , "1"                        , FCVAR_RELEASE    , "Use random AES encryption key for game packets.", false, 0.f, false, 0.f, &NET_UseRandomKeyChanged_f, nullptr);
	net_processTimeBudget      = ConVar::Create("net_processTimeBudget"     ,"200"                       , FCVAR_RELEASE    , "Net message process budget in milliseconds (removing netchannel if exceeded).", true, 0.f, false, 0.f, nullptr, "0 = disabled.");
	//-------------------------------------------------------------------------
	// NETWORKSYSTEM                                                          |
	pylon_matchmaking_hostname = ConVar::Create("pylon_matchmaking_hostname", "ms.r5reloaded.com", FCVAR_RELEASE, "Holds the pylon matchmaking hostname.", false, 0.f, false, 0.f, &MP_HostName_Changed_f, nullptr);
	pylon_host_update_interval = ConVar::Create("pylon_host_update_interval", "5"                , FCVAR_RELEASE, "Length of time in seconds between each status update interval to master server.", true, 5.f, false, 0.f, nullptr, nullptr);
	pylon_showdebuginfo        = ConVar::Create("pylon_showdebuginfo"       , "0"                , FCVAR_RELEASE, "Shows debug output for pylon.", false, 0.f, false, 0.f, nullptr, nullptr);
	//-------------------------------------------------------------------------
	// RTECH API                                                              |
	rtech_debug = ConVar::Create("rtech_debug", "0", FCVAR_DEVELOPMENTONLY, "Shows debug output for the RTech system.", false, 0.f, false, 0.f, nullptr, nullptr);
	//-------------------------------------------------------------------------
	// RUI                                                                    |
#ifndef DEDICATED
	rui_drawEnable = ConVar::Create("rui_drawEnable", "1", FCVAR_RELEASE, "Draws the RUI if set.", false, 0.f, false, 0.f, nullptr, " 1 = Draw, 0 = No Draw.");
#endif // !DEDICATED
	//-------------------------------------------------------------------------
	// MILES                                                                  |
#ifndef DEDICATED
	miles_debug = ConVar::Create("miles_debug", "0", FCVAR_RELEASE, "Enables debug prints for the Miles Sound System", false, 0.f, false, 0.f, nullptr, " 1 = Print, 0 = No Print");
#endif // !DEDICATED
	//-------------------------------------------------------------------------
}

//-----------------------------------------------------------------------------
// Purpose: initialize shipped ConVar's
//-----------------------------------------------------------------------------
void ConVar::InitShipped(void)
{
#ifndef CLIENT_DLL
	ai_script_nodes_draw             = g_pCVar->FindVar("ai_script_nodes_draw");
#if !defined (GAMEDLL_S0) && !defined (GAMEDLL_S1)
	bhit_enable                      = g_pCVar->FindVar("bhit_enable");
#endif // !GAMEDLL_S0 && !GAMEDLL_S1
#endif // !CLIENT_DLL
	developer                        = g_pCVar->FindVar("developer");
#ifndef DEDICATED
	cl_threaded_bone_setup           = g_pCVar->FindVar("cl_threaded_bone_setup");
#endif // !DEDICATED
	single_frame_shutdown_for_reload = g_pCVar->FindVar("single_frame_shutdown_for_reload");
	enable_debug_overlays            = g_pCVar->FindVar("enable_debug_overlays");
	debug_draw_box_depth_test        = g_pCVar->FindVar("debug_draw_box_depth_test");
	model_defaultFadeDistScale       = g_pCVar->FindVar("model_defaultFadeDistScale");
	model_defaultFadeDistMin         = g_pCVar->FindVar("model_defaultFadeDistMin");
#ifndef DEDICATED
	rui_defaultDebugFontFace         = g_pCVar->FindVar("rui_defaultDebugFontFace");
	r_visualizetraces                = g_pCVar->FindVar("r_visualizetraces");
	r_visualizetraces_duration       = g_pCVar->FindVar("r_visualizetraces_duration");
#endif // !DEDICATED
	staticProp_no_fade_scalar        = g_pCVar->FindVar("staticProp_no_fade_scalar");
	staticProp_gather_size_weight    = g_pCVar->FindVar("staticProp_gather_size_weight");
	stream_overlay                   = g_pCVar->FindVar("stream_overlay");
	stream_overlay_mode              = g_pCVar->FindVar("stream_overlay_mode");
	sv_visualizetraces               = g_pCVar->FindVar("sv_visualizetraces");
	sv_visualizetraces_duration      = g_pCVar->FindVar("sv_visualizetraces_duration");
	old_gather_props                 = g_pCVar->FindVar("old_gather_props");
#ifndef DEDICATED
	origin_disconnectWhenOffline     = g_pCVar->FindVar("origin_disconnectWhenOffline");
#endif // !DEDICATED
	mp_gamemode                      = g_pCVar->FindVar("mp_gamemode");
	hostname                         = g_pCVar->FindVar("hostname");
	hostip                           = g_pCVar->FindVar("hostip");
	hostport                         = g_pCVar->FindVar("hostport");
	host_hasIrreversibleShutdown     = g_pCVar->FindVar("host_hasIrreversibleShutdown");
	net_usesocketsforloopback        = g_pCVar->FindVar("net_usesocketsforloopback");
#ifndef CLIENT_DLL
	sv_showhitboxes = g_pCVar->FindVar("sv_showhitboxes");

	sv_showhitboxes->SetMin(-1); // Allow user to go over each entity manually without going out of bounds.
	sv_showhitboxes->SetMax(NUM_ENT_ENTRIES - 1);

	sv_forceChatToTeamOnly = g_pCVar->FindVar("sv_forceChatToTeamOnly");

	sv_forceChatToTeamOnly->RemoveFlags(FCVAR_DEVELOPMENTONLY);
	sv_forceChatToTeamOnly->AddFlags(FCVAR_REPLICATED);

	ai_script_nodes_draw->SetValue(-1);
	bhit_enable->SetValue(0);
#endif // !CLIENT_DLL
#ifndef DEDICATED
	cl_threaded_bone_setup->RemoveFlags(FCVAR_DEVELOPMENTONLY);
	rui_defaultDebugFontFace->RemoveFlags(FCVAR_DEVELOPMENTONLY);
	origin_disconnectWhenOffline->RemoveFlags(FCVAR_DEVELOPMENTONLY);
#endif // !DEDICATED
	mp_gamemode->RemoveFlags(FCVAR_DEVELOPMENTONLY);
	mp_gamemode->RemoveChangeCallback(mp_gamemode->m_fnChangeCallbacks[0]);
	mp_gamemode->InstallChangeCallback(MP_GameMode_Changed_f, false);
}

//-----------------------------------------------------------------------------
// Purpose: unregister/disable extraneous ConVar's.
//-----------------------------------------------------------------------------
void ConVar::PurgeShipped(void)
{
#ifdef DEDICATED
	const char* pszToPurge[] =
	{
		"bink_materials_enabled",
		"communities_enabled",
		"community_frame_run",
		"ime_enabled",
		"origin_igo_mutes_sound_enabled",
		"twitch_shouldQuery",
		"voice_enabled",
	};

	for (size_t i = 0; i < SDK_ARRAYSIZE(pszToPurge); i++)
	{
		if (ConVar* pCVar = g_pCVar->FindVar(pszToPurge[i]))
		{
			pCVar->SetValue(0);
		}
	}
#endif // DEDICATED
}

//-----------------------------------------------------------------------------
// Purpose: clear all hostname ConVar's.
//-----------------------------------------------------------------------------
void ConVar::PurgeHostNames(void)
{
	const char* pszHostNames[] =
	{
		"assetdownloads_hostname",
		"communities_hostname",
		"matchmaking_hostname",
		"party_hostname",
		"persistence_hostname",
		"persistenceDef_hostname",
		"pin_telemetry_hostname",
		"publication_hostname",
		"serverReports_hostname",
		"skill_hostname",
		"speechtotext_hostname",
		"staticfile_hostname",
		"stats_hostname",
		"steamlink_hostname",
		"subscription_hostname",
		"users_hostname"
	};

	for (size_t i = 0; i < SDK_ARRAYSIZE(pszHostNames); i++)
	{
		if (ConVar* pCVar = g_pCVar->FindVar(pszHostNames[i]))
		{
			pCVar->SetValue("0.0.0.0");
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Add's flags to ConVar.
// Input  : nFlags - 
//-----------------------------------------------------------------------------
void ConVar::AddFlags(int nFlags)
{
	m_pParent->m_nFlags |= nFlags;

#ifdef ALLOW_DEVELOPMENT_CVARS
	m_pParent->m_nFlags &= ~FCVAR_DEVELOPMENTONLY;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Removes flags from ConVar.
// Input  : nFlags - 
//-----------------------------------------------------------------------------
void ConVar::RemoveFlags(int nFlags)
{
	m_nFlags &= ~nFlags;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the base ConVar name.
// Output : const char*
//-----------------------------------------------------------------------------
const char* ConVar::GetBaseName(void) const
{
	return m_pParent->m_pszName;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the ConVar help text.
// Output : const char*
//-----------------------------------------------------------------------------
const char* ConVar::GetHelpText(void) const
{
	return m_pParent->m_pszHelpString;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the ConVar usage text.
// Output : const char*
//-----------------------------------------------------------------------------
const char* ConVar::GetUsageText(void) const
{
	return m_pParent->m_pszUsageString;
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a boolean.
// Output : bool
//-----------------------------------------------------------------------------
bool ConVar::GetBool(void) const
{
	return !!GetInt();
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a float.
// Output : float
//-----------------------------------------------------------------------------
float ConVar::GetFloat(void) const
{
	return m_pParent->m_Value.m_fValue;
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a double.
// Output : double
//-----------------------------------------------------------------------------
double ConVar::GetDouble(void) const
{
	return static_cast<double>(m_pParent->m_Value.m_fValue);
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as an integer.
// Output : int
//-----------------------------------------------------------------------------
int ConVar::GetInt(void) const
{
	return m_pParent->m_Value.m_nValue;
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as an integer (64-bit).
// Output : int
//-----------------------------------------------------------------------------
int64_t ConVar::GetInt64(void) const
{
	return static_cast<int64_t>(m_pParent->m_Value.m_nValue);
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a size type.
// Output : int
//-----------------------------------------------------------------------------
size_t ConVar::GetSizeT(void) const
{
	return static_cast<size_t>(m_pParent->m_Value.m_nValue);
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a color.
// Output : Color
//-----------------------------------------------------------------------------
Color ConVar::GetColor(void) const
{
	unsigned char* pColorElement = (reinterpret_cast<unsigned char*>(&m_pParent->m_Value.m_nValue));
	return Color(pColorElement[0], pColorElement[1], pColorElement[2], pColorElement[3]);
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a string.
// Output : const char *
//-----------------------------------------------------------------------------
const char* ConVar::GetString(void) const
{
	if (m_nFlags & FCVAR_NEVER_AS_STRING)
	{
		return "FCVAR_NEVER_AS_STRING";
	}

	char const* str = m_pParent->m_Value.m_pszString;
	return str ? str : "";
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flMaxVal - 
//-----------------------------------------------------------------------------
void ConVar::SetMax(float flMaxVal)
{
	m_pParent->m_fMaxVal = flMaxVal;
	m_pParent->m_bHasMax = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flMinVal - 
//-----------------------------------------------------------------------------
void ConVar::SetMin(float flMinVal)
{
	m_pParent->m_fMinVal = flMinVal;
	m_pParent->m_bHasMin = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flMinVal - 
// Output : true if there is a min set.
//-----------------------------------------------------------------------------
bool ConVar::GetMin(float& flMinVal) const
{
	flMinVal = m_pParent->m_fMinVal;
	return m_pParent->m_bHasMin;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flMaxVal - 
// Output : true if there is a max set.
//-----------------------------------------------------------------------------
bool ConVar::GetMax(float& flMaxVal) const
{
	flMaxVal = m_pParent->m_fMaxVal;
	return m_pParent->m_bHasMax;
}

//-----------------------------------------------------------------------------
// Purpose: returns the min value.
// Output : float
//-----------------------------------------------------------------------------
float ConVar::GetMinValue(void) const
{
	return m_pParent->m_fMinVal;
}

//-----------------------------------------------------------------------------
// Purpose: returns the max value.
// Output : float
//-----------------------------------------------------------------------------
float ConVar::GetMaxValue(void) const
{
	return m_pParent->m_fMaxVal;
}

//-----------------------------------------------------------------------------
// Purpose: checks if ConVar has min value.
// Output : bool
//-----------------------------------------------------------------------------
bool ConVar::HasMin(void) const
{
	return m_pParent->m_bHasMin;
}

//-----------------------------------------------------------------------------
// Purpose: checks if ConVar has max value.
// Output : bool
//-----------------------------------------------------------------------------
bool ConVar::HasMax(void) const
{
	return m_pParent->m_bHasMax;
}

//-----------------------------------------------------------------------------
// Purpose: sets the ConVar int value.
// Input  : nValue - 
//-----------------------------------------------------------------------------
void ConVar::SetValue(int nValue)
{
	ConVar* pCVar = reinterpret_cast<ConVar*>(m_pParent);
	pCVar->InternalSetIntValue(nValue);
}

//-----------------------------------------------------------------------------
// Purpose: sets the ConVar float value.
// Input  : flValue - 
//-----------------------------------------------------------------------------
void ConVar::SetValue(float flValue)
{
	ConVar* pCVar = reinterpret_cast<ConVar*>(m_pParent);
	pCVar->InternalSetFloatValue(flValue);
}

//-----------------------------------------------------------------------------
// Purpose: sets the ConVar string value.
// Input  : *szValue - 
//-----------------------------------------------------------------------------
void ConVar::SetValue(const char* pszValue)
{
	ConVar* pCVar = reinterpret_cast<ConVar*>(m_pParent);
	pCVar->InternalSetValue(pszValue);
}

//-----------------------------------------------------------------------------
// Purpose: sets the ConVar color value.
// Input  : value - 
//-----------------------------------------------------------------------------
void ConVar::SetValue(Color value)
{
	ConVar* pCVar = reinterpret_cast<ConVar*>(m_pParent);
	pCVar->InternalSetColorValue(value);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pszValue - 
//-----------------------------------------------------------------------------
void ConVar::InternalSetValue(const char* pszValue)
{
	if (IsFlagSet(FCVAR_MATERIAL_THREAD_MASK))
	{
		if (g_pCVar && !g_pCVar->IsMaterialThreadSetAllowed())
		{
			g_pCVar->QueueMaterialThreadSetValue(this, pszValue);
			return;
		}
	}

	char szTempValue[32];
	const char* pszNewValue;

	// Only valid for root convars.
	assert(m_pParent == this);

	pszNewValue = const_cast<char*>(pszValue);
	if (!pszNewValue)
	{
		pszNewValue = "";
	}

	if (!SetColorFromString(pszNewValue))
	{
		// Not a color, do the standard thing
		double dblValue = atof(pszNewValue); // Use double to avoid 24-bit restriction on integers and allow storing timestamps or dates in convars
		float flNewValue = static_cast<float>(dblValue);

		if (!IsFinite(flNewValue))
		{
			Warning(eDLL_T::ENGINE, "Warning: ConVar '%s' = '%s' is infinite, clamping value.\n", GetBaseName(), pszNewValue);
			flNewValue = FLT_MAX;
		}

		if (ClampValue(flNewValue))
		{
			snprintf(szTempValue, sizeof(szTempValue), "%f", flNewValue);
			pszNewValue = szTempValue;
		}

		// Redetermine value
		m_Value.m_fValue = flNewValue;
		m_Value.m_nValue = static_cast<int>(m_Value.m_fValue);
	}

	if (!(m_nFlags & FCVAR_NEVER_AS_STRING))
	{
		ChangeStringValue(pszNewValue);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *nValue - 
//-----------------------------------------------------------------------------
void ConVar::InternalSetIntValue(int nValue)
{
	if (nValue == m_Value.m_nValue)
		return;

	if (IsFlagSet(FCVAR_MATERIAL_THREAD_MASK))
	{
		if (g_pCVar && !g_pCVar->IsMaterialThreadSetAllowed())
		{
			g_pCVar->QueueMaterialThreadSetValue(this, nValue);
			return;
		}
	}

	assert(m_pParent == this); // Only valid for root convars.

	float fValue = static_cast<float>(nValue);
	if (ClampValue(fValue))
	{
		nValue = static_cast<int>(fValue);
	}

	// Redetermine value
	m_Value.m_fValue = fValue;
	m_Value.m_nValue = nValue;

	if (!(m_nFlags & FCVAR_NEVER_AS_STRING))
	{
		char szTempVal[32];
		snprintf(szTempVal, sizeof(szTempVal), "%d", nValue);
		ChangeStringValue(szTempVal);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *flValue - 
//-----------------------------------------------------------------------------
void ConVar::InternalSetFloatValue(float flValue)
{
	if (flValue == m_Value.m_fValue)
		return;

	if (IsFlagSet(FCVAR_MATERIAL_THREAD_MASK))
	{
		if (g_pCVar && !g_pCVar->IsMaterialThreadSetAllowed())
		{
			g_pCVar->QueueMaterialThreadSetValue(this, flValue);
			return;
		}
	}

	assert(m_pParent == this); // Only valid for root convars.

	// Check bounds
	ClampValue(flValue);

	// Redetermine value
	m_Value.m_fValue = flValue;
	m_Value.m_nValue = static_cast<int>(flValue);

	if (!(m_nFlags & FCVAR_NEVER_AS_STRING))
	{
		char szTempVal[32];
		snprintf(szTempVal, sizeof(szTempVal), "%f", flValue);
		ChangeStringValue(szTempVal);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *value - 
//-----------------------------------------------------------------------------
void ConVar::InternalSetColorValue(Color value)
{
	// Stuff color values into an int
	int nValue = 0;

	unsigned char* pColorElement = (reinterpret_cast<unsigned char*>(&nValue));
	pColorElement[0] = value[0];
	pColorElement[1] = value[1];
	pColorElement[2] = value[2];
	pColorElement[3] = value[3];

	// Call the int internal set
	InternalSetIntValue(nValue);
}

//-----------------------------------------------------------------------------
// Purpose: Reset to default value.
//-----------------------------------------------------------------------------
void ConVar::Revert(void)
{
	SetValue(m_pszDefaultValue);
}

//-----------------------------------------------------------------------------
// Purpose: Check whether to clamp and then perform clamp.
// Input  : flValue - 
// Output : Returns true if value changed.
//-----------------------------------------------------------------------------
bool ConVar::ClampValue(float& flValue)
{
	if (m_bHasMin && (flValue < m_fMinVal))
	{
		flValue = m_fMinVal;
		return true;
	}

	if (m_bHasMax && (flValue > m_fMaxVal))
	{
		flValue = m_fMaxVal;
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: returns the default ConVar value.
// Output : const char
//-----------------------------------------------------------------------------
const char* ConVar::GetDefault(void) const
{
	return m_pParent->m_pszDefaultValue;
}

//-----------------------------------------------------------------------------
// Purpose: sets the default ConVar value.
// Input  : *pszDefault -
//-----------------------------------------------------------------------------
void ConVar::SetDefault(const char* pszDefault)
{
	static const char* pszEmpty = "";
	m_pszDefaultValue = pszDefault ? pszDefault : pszEmpty;
	assert(m_pszDefaultValue);
}

//-----------------------------------------------------------------------------
// Purpose: sets the ConVar color value from string.
// Input  : *pszValue - 
//-----------------------------------------------------------------------------
bool ConVar::SetColorFromString(const char* pszValue)
{
	bool bColor = false;

	// Try pulling RGBA color values out of the string.
	int nRGBA[4];
	int nParamsRead = sscanf_s(pszValue, "%i %i %i %i", &(nRGBA[0]), &(nRGBA[1]), &(nRGBA[2]), &(nRGBA[3]));

	if (nParamsRead >= 3)
	{
		// This is probably a color!
		if (nParamsRead == 3)
		{
			// Assume they wanted full alpha.
			nRGBA[3] = 255;
		}

		if (nRGBA[0] >= 0 && nRGBA[0] <= 255 &&
			nRGBA[1] >= 0 && nRGBA[1] <= 255 &&
			nRGBA[2] >= 0 && nRGBA[2] <= 255 &&
			nRGBA[3] >= 0 && nRGBA[3] <= 255)
		{
			//printf("*** WOW! Found a color!! ***\n");

			// This is definitely a color!
			bColor = true;

			// Stuff all the values into each byte of our int.
			unsigned char* pColorElement = (reinterpret_cast<unsigned char*>(&m_Value.m_nValue));
			pColorElement[0] = nRGBA[0];
			pColorElement[1] = nRGBA[1];
			pColorElement[2] = nRGBA[2];
			pColorElement[3] = nRGBA[3];

			// Copy that value into our float.
			m_Value.m_fValue = static_cast<float>(m_Value.m_nValue);
		}
	}

	return bColor;
}

//-----------------------------------------------------------------------------
// Purpose: changes the ConVar string value.
// Input  : *pszTempVal - flOldValue
//-----------------------------------------------------------------------------
void ConVar::ChangeStringValue(const char* pszTempVal)
{
	Assert(!(m_nFlags & FCVAR_NEVER_AS_STRING));

	char* pszOldValue = (char*)stackalloc(m_Value.m_iStringLength);
	memcpy(pszOldValue, m_Value.m_pszString, m_Value.m_iStringLength);

	size_t len = strlen(pszTempVal) + 1;

	if (len > m_Value.m_iStringLength)
	{
		if (m_Value.m_pszString)
		{
			MemAllocSingleton()->Free(m_Value.m_pszString);
		}

		m_Value.m_pszString = MemAllocSingleton()->Alloc<char>(len);
		m_Value.m_iStringLength = len;
	}

	memcpy(reinterpret_cast<void*>(m_Value.m_pszString), pszTempVal, len);

	// Invoke any necessary callback function
	for (int i = 0; i < m_fnChangeCallbacks.Count(); ++i)
	{
		m_fnChangeCallbacks[i](reinterpret_cast<IConVar*>(&m_pIConVarVFTable), pszOldValue, NULL);
	}

	if (g_pCVar)
	{
		g_pCVar->CallGlobalChangeCallbacks(this, pszOldValue);
	}

	stackfree(pszOldValue);
}

//-----------------------------------------------------------------------------
// Purpose: changes the ConVar string value without calling the callback
// (Size of new string must be equal or lower than m_iStringLength!!!)
// Input  : *pszTempVal - flOldValue
//-----------------------------------------------------------------------------
void ConVar::ChangeStringValueUnsafe(const char* pszNewValue)
{
	m_Value.m_pszString = const_cast<char*>(pszNewValue);
}

//-----------------------------------------------------------------------------
// Purpose: Install a change callback (there shouldn't already be one....)
// Input  : callback - 
//			bInvoke - 
//-----------------------------------------------------------------------------
void ConVar::InstallChangeCallback(FnChangeCallback_t callback, bool bInvoke /*=true*/)
{
	if (!callback)
	{
		Warning(eDLL_T::ENGINE, "%s: called with NULL callback, ignoring!!!\n", __FUNCTION__);
		return;
	}

	if (m_pParent->m_fnChangeCallbacks.Find(callback) != m_pParent->m_fnChangeCallbacks.InvalidIndex())
	{
		// Same ptr added twice, sigh...
		Warning(eDLL_T::ENGINE, "%s: ignoring duplicate change callback!!!\n", __FUNCTION__);
		return;
	}

	m_pParent->m_fnChangeCallbacks.AddToTail(callback);

	// Call it immediately to set the initial value...
	if (bInvoke)
	{
		callback(reinterpret_cast<IConVar*>(&m_pIConVarVFTable), m_Value.m_pszString, m_Value.m_fValue);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Install a change callback (there shouldn't already be one....)
// Input  : callback - 
//-----------------------------------------------------------------------------
void ConVar::RemoveChangeCallback(FnChangeCallback_t callback)
{
	m_pParent->m_fnChangeCallbacks.FindAndRemove(callback);
}

//-----------------------------------------------------------------------------
// Purpose: Checks if ConVar is registered.
// Output : bool
//-----------------------------------------------------------------------------
bool ConVar::IsRegistered(void) const
{
	return m_pParent->m_bRegistered;
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if this is a command 
// Output : bool
//-----------------------------------------------------------------------------
bool ConVar::IsCommand(void) const
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Test each ConVar query before setting the value.
// Input  : *pConVar - nFlags
// Output : False if change is permitted, true if not.
//-----------------------------------------------------------------------------
bool ConVar::IsFlagSetInternal(const ConVar* pConVar, int nFlags)
{
	if (cm_unset_all_cmdquery->GetBool())
	{
		// Returning false on all queries may cause problems.
		return false;
	}
	// Default behavior.
	return pConVar->HasFlags(nFlags) != 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ConVar_AppendFlags(ConCommandBase* var, char* buf, size_t bufsize)
{
	for (int i = 0; i < ARRAYSIZE(g_PrintConVarFlags); ++i)
	{
		const ConVarFlagsToString_t& info = g_PrintConVarFlags[i];
		if (var->IsFlagSet(info.m_nFlag))
		{
			char append[128];
			V_snprintf(append, sizeof(append), " %s", info.m_pszDesc);
			V_strncat(buf, append, bufsize);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ConVar_PrintDescription(ConCommandBase* pVar)
{
	bool bMin, bMax;
	float fMin, fMax;
	const char* pStr;

	Assert(pVar);

	Color clr(255, 100, 100, 255);

	char outstr[4096];
	outstr[0] = 0;

	if (!pVar->IsCommand())
	{
		ConVar* var = (ConVar*)pVar;

		bMin = var->GetMin(fMin);
		bMax = var->GetMax(fMax);

		const char* value = NULL;
		char tempVal[256];

		if (var->IsFlagSet(FCVAR_NEVER_AS_STRING))
		{
			value = tempVal;

			int intVal = var->GetInt();
			float floatVal = var->GetFloat();

			if (fabs((float)intVal - floatVal) < 0.000001)
			{
				V_snprintf(tempVal, sizeof(tempVal), "%d", intVal);
			}
			else
			{
				V_snprintf(tempVal, sizeof(tempVal), "%f", floatVal);
			}
		}
		else
		{
			value = var->GetString();
		}

		if (value)
		{
			AppendPrintf(outstr, sizeof(outstr), "\"%s\" = \"%s\"", var->GetName(), value);

			if (V_stricmp(value, var->GetDefault()))
			{
				AppendPrintf(outstr, sizeof(outstr), " ( def. \"%s\" )", var->GetDefault());
			}
		}

		if (bMin)
		{
			AppendPrintf(outstr, sizeof(outstr), " min. %f", fMin);
		}
		if (bMax)
		{
			AppendPrintf(outstr, sizeof(outstr), " max. %f", fMax);
		}
	}
	else
	{
		ConCommand* var = (ConCommand*)pVar;

		AppendPrintf(outstr, sizeof(outstr), "\"%s\" ", var->GetName());
	}

	ConVar_AppendFlags(pVar, outstr, sizeof(outstr));

	pStr = pVar->GetHelpText();
	if (pStr && *pStr)
	{
		DevMsg(eDLL_T::ENGINE, "%-80s - %.80s\n", outstr, pStr);
	}
	else
	{
		DevMsg(eDLL_T::ENGINE, "%-80s\n", outstr);
	}
}

///////////////////////////////////////////////////////////////////////////////
void VConVar::Attach() const
{
	DetourAttach((LPVOID*)&v_ConVar_IsFlagSet, &ConVar::IsFlagSetInternal);
	DetourAttach((LPVOID*)&v_ConVar_PrintDescription, &ConVar_PrintDescription);
}

void VConVar::Detach() const
{
	DetourDetach((LPVOID*)&v_ConVar_IsFlagSet, &ConVar::IsFlagSetInternal);
	DetourDetach((LPVOID*)&v_ConVar_PrintDescription, &ConVar_PrintDescription);
}

///////////////////////////////////////////////////////////////////////////////
