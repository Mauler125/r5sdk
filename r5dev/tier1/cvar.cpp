#include "core/stdafx.h"
#include "tier1/utlrbtree.h"
#include "tier1/cvar.h"
#include "public/const.h"
#include "engine/sys_dll2.h"
#include "filesystem/filesystem.h"
#include "vstdlib/concommandhash.h"
#include "vstdlib/completion.h"
#include "vstdlib/callback.h"

//-----------------------------------------------------------------------------
// ENGINE                                                                     |
ConVar* sdk_fixedframe_tickinterval        = nullptr;
ConVar* single_frame_shutdown_for_reload   = nullptr;
ConVar* old_gather_props                   = nullptr;

ConVar* enable_debug_overlays              = nullptr;
ConVar* debug_draw_box_depth_test          = nullptr;

ConVar* developer                          = nullptr;
ConVar* fps_max                            = nullptr;

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

ConVar* curl_debug                         = nullptr;
ConVar* curl_timeout                       = nullptr;
ConVar* ssl_verify_peer                    = nullptr;

ConVar* rcon_address                       = nullptr;
ConVar* rcon_password                      = nullptr;

ConVar* r_debug_overlay_nodecay            = nullptr;
ConVar* r_debug_overlay_invisible          = nullptr;
ConVar* r_debug_overlay_wireframe          = nullptr;
ConVar* r_debug_draw_depth_test            = nullptr;
ConVar* r_drawWorldMeshes                  = nullptr;
ConVar* r_drawWorldMeshesDepthOnly         = nullptr;
ConVar* r_drawWorldMeshesDepthAtTheEnd     = nullptr;

#ifndef DEDICATED
ConVar* r_visualizetraces                  = nullptr;
ConVar* r_visualizetraces_duration         = nullptr;
#endif // !DEDICATED

ConVar* stream_overlay                     = nullptr;
ConVar* stream_overlay_mode                = nullptr;
//-----------------------------------------------------------------------------
// SERVER                                                                     |
#ifndef CLIENT_DLL
ConVar* ai_ainDumpOnLoad                   = nullptr;
ConVar* ai_ainDebugConnect                 = nullptr;
ConVar* ai_script_nodes_draw               = nullptr;
ConVar* ai_script_nodes_draw_range         = nullptr;
ConVar* ai_script_nodes_draw_nearest       = nullptr;

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
ConVar* sv_globalBanlist                   = nullptr;
ConVar* sv_pylonVisibility                 = nullptr;
ConVar* sv_pylonRefreshRate                = nullptr;
ConVar* sv_banlistRefreshRate              = nullptr;
ConVar* sv_statusRefreshRate               = nullptr;
ConVar* sv_forceChatToTeamOnly             = nullptr;

ConVar* sv_updaterate_mp                   = nullptr;
ConVar* sv_updaterate_sp                   = nullptr;
ConVar* sv_autoReloadRate                  = nullptr;

ConVar* sv_simulateBots                    = nullptr;
ConVar* sv_showhitboxes                    = nullptr;
ConVar* sv_stats                           = nullptr;

ConVar* sv_quota_stringCmdsPerSecond       = nullptr;

ConVar* sv_validatePersonaName             = nullptr;
ConVar* sv_minPersonaNameLength            = nullptr;
ConVar* sv_maxPersonaNameLength            = nullptr;

//#ifdef DEDICATED
ConVar* sv_rcon_debug                      = nullptr;
ConVar* sv_rcon_sendlogs                   = nullptr;
ConVar* sv_rcon_banpenalty                 = nullptr; // TODO
ConVar* sv_rcon_maxfailures                = nullptr;
ConVar* sv_rcon_maxignores                 = nullptr;
ConVar* sv_rcon_maxsockets                 = nullptr;
ConVar* sv_rcon_whitelist_address          = nullptr;
//#endif // DEDICATED
#endif // !CLIENT_DLL
ConVar* sv_visualizetraces                 = nullptr;
ConVar* sv_visualizetraces_duration        = nullptr;
#if !defined (GAMEDLL_S0) && !defined (GAMEDLL_S1)
ConVar* bhit_enable                        = nullptr;
ConVar* bhit_depth_test                    = nullptr;
ConVar* bhit_abs_origin                    = nullptr;
#endif // !GAMEDLL_S0 && !GAMEDLL_S1
//-----------------------------------------------------------------------------
// CLIENT                                                                     |
#ifndef DEDICATED
ConVar* cl_rcon_request_sendlogs           = nullptr;
ConVar* cl_quota_stringCmdsPerSecond       = nullptr;

ConVar* cl_notify_invert_x                 = nullptr;
ConVar* cl_notify_invert_y                 = nullptr;
ConVar* cl_notify_offset_x                 = nullptr;
ConVar* cl_notify_offset_y                 = nullptr;

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
ConVar* con_notify_native_audio_clr        = nullptr;
ConVar* con_notify_native_video_clr        = nullptr;
ConVar* con_notify_netcon_clr              = nullptr;
ConVar* con_notify_common_clr              = nullptr;
ConVar* con_notify_warning_clr             = nullptr;
ConVar* con_notify_error_clr               = nullptr;

ConVar* con_max_lines                      = nullptr;
ConVar* con_max_history                    = nullptr;
ConVar* con_suggestion_limit               = nullptr;
ConVar* con_suggestion_showhelptext        = nullptr;
ConVar* con_suggestion_showflags           = nullptr;
ConVar* con_suggestion_flags_realtime      = nullptr;

ConVar* origin_disconnectWhenOffline       = nullptr;
#endif // !DEDICATED
//-----------------------------------------------------------------------------
// FILESYSTEM                                                                 |
ConVar* fs_showWarnings                    = nullptr;
ConVar* fs_showAllReads                    = nullptr;
ConVar* fs_packedstore_entryblock_stats    = nullptr;
ConVar* fs_packedstore_workspace           = nullptr;
ConVar* fs_packedstore_compression_level   = nullptr;
ConVar* fs_packedstore_max_helper_threads  = nullptr;
//-----------------------------------------------------------------------------
// MATERIALSYSTEM                                                             |
#ifndef DEDICATED
ConVar* mat_alwaysComplain                 = nullptr;
#endif // !DEDICATED
//-----------------------------------------------------------------------------
// SQUIRREL                                                                   |
ConVar* script_show_output                 = nullptr;
ConVar* script_show_warning                = nullptr;
//-----------------------------------------------------------------------------
// NETCHANNEL                                                                 |
ConVar* net_tracePayload                   = nullptr;
ConVar* net_encryptionEnable               = nullptr;
ConVar* net_useRandomKey                   = nullptr;
ConVar* net_usesocketsforloopback          = nullptr;
ConVar* net_processTimeBudget              = nullptr;

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
//-----------------------------------------------------------------------------
// MILES                                                                      |
#ifndef DEDICATED
ConVar* miles_debug                        = nullptr;
ConVar* miles_language                     = nullptr;
#endif

//-----------------------------------------------------------------------------
// Purpose: create
//-----------------------------------------------------------------------------
ConVar* ConVar::StaticCreate(const char* pszName, const char* pszDefaultValue, int nFlags, const char* pszHelpString, bool bMin, float fMin, bool bMax, float fMax, FnChangeCallback_t pCallback, const char* pszUsageString)
{
	ConVar* pNewConVar = MemAllocSingleton()->Alloc<ConVar>(sizeof(ConVar)); // Allocate new memory with StdMemAlloc else we crash.

	pNewConVar->m_bRegistered = false;
	*(ConVar**)pNewConVar = g_pConVarVBTable;
	char* pConVarVFTable = (char*)pNewConVar + sizeof(ConCommandBase);
	*(IConVar**)pConVarVFTable = g_pConVarVFTable;

	pNewConVar->m_pszName = nullptr;
	pNewConVar->m_pszHelpString = nullptr;
	pNewConVar->m_pszUsageString = nullptr;
	pNewConVar->s_pAccessor = nullptr;
	pNewConVar->m_nFlags = FCVAR_NONE;
	pNewConVar->m_pNext = nullptr;

	pNewConVar->m_fnChangeCallbacks.Init();

	v_ConVar_Register(pNewConVar, pszName, pszDefaultValue, nFlags, pszHelpString, bMin, fMin, bMax, fMax, pCallback, pszUsageString);
	return pNewConVar;
}

//-----------------------------------------------------------------------------
// Purpose: destroy
//-----------------------------------------------------------------------------
void ConVar::Destroy(void)
{
	v_ConVar_Unregister(this);
}

//-----------------------------------------------------------------------------
// Purpose: construct/allocate
//-----------------------------------------------------------------------------
ConVar::ConVar(void)
	: m_pParent(nullptr)
	, m_pszDefaultValue(nullptr)
	, m_bHasMin(false)
	, m_fMinVal(0.f)
	, m_bHasMax(false)
	, m_fMaxVal(0.f)
{
	m_Value.m_pszString = nullptr;
	m_Value.m_iStringLength = 0;
	m_Value.m_fValue = 0.0f;
	m_Value.m_nValue = 0;
}

//-----------------------------------------------------------------------------
// Purpose: destructor
//-----------------------------------------------------------------------------
//ConVar::~ConVar(void)
//{
//	if (m_Value.m_pszString)
//	{
//		MemAllocSingleton()->Free(m_Value.m_pszString);
//		m_Value.m_pszString = NULL;
//	}
//}

//-----------------------------------------------------------------------------
// Purpose: initialize ConVar's
//-----------------------------------------------------------------------------
void ConVar::StaticInit(void)
{
	//-------------------------------------------------------------------------
	// ENGINE                                                                 |
	hostdesc                       = ConVar::StaticCreate("hostdesc", "", FCVAR_RELEASE, "Host game server description.", false, 0.f, false, 0.f, nullptr, nullptr);
	sdk_fixedframe_tickinterval    = ConVar::StaticCreate("sdk_fixedframe_tickinterval", "0.02", FCVAR_RELEASE, "The tick interval used by the SDK fixed frame.", false, 0.f, false, 0.f, nullptr, nullptr);
	staticProp_defaultBuildFrustum = ConVar::StaticCreate("staticProp_defaultBuildFrustum", "0", FCVAR_DEVELOPMENTONLY, "Use the old solution for building static prop frustum culling.", false, 0.f, false, 0.f, nullptr, nullptr);

	curl_debug      = ConVar::StaticCreate("curl_debug"     , "0" , FCVAR_DEVELOPMENTONLY, "Determines whether or not to enable curl debug logging.", false, 0.f, false, 0.f, nullptr, "1 = curl logs; 0 (zero) = no logs.");
	curl_timeout    = ConVar::StaticCreate("curl_timeout"   , "15", FCVAR_DEVELOPMENTONLY, "Maximum time in seconds a curl transfer operation could take.", false, 0.f, false, 0.f, nullptr, nullptr);
	ssl_verify_peer = ConVar::StaticCreate("ssl_verify_peer", "1" , FCVAR_DEVELOPMENTONLY, "Verify the authenticity of the peer's SSL certificate.", false, 0.f, false, 0.f, nullptr, "1 = curl verifies; 0 (zero) = no verification.");

	rcon_address  = ConVar::StaticCreate("rcon_address",  "localhost", FCVAR_SERVER_CANNOT_QUERY | FCVAR_DONTRECORD | FCVAR_RELEASE, "Remote server access address.", false, 0.f, false, 0.f, nullptr, nullptr);
	rcon_password = ConVar::StaticCreate("rcon_password", ""         , FCVAR_SERVER_CANNOT_QUERY | FCVAR_DONTRECORD | FCVAR_RELEASE, "Remote server access password (rcon is disabled if empty).", false, 0.f, false, 0.f, &RCON_PasswordChanged_f, nullptr);

	r_debug_overlay_nodecay        = ConVar::StaticCreate("r_debug_overlay_nodecay"       , "0", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "Keeps all debug overlays alive regardless of their lifetime. Use command 'clear_debug_overlays' to clear everything.", false, 0.f, false, 0.f, nullptr, nullptr);
	r_debug_overlay_invisible      = ConVar::StaticCreate("r_debug_overlay_invisible"     , "1", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "Show invisible debug overlays (alpha < 1 = 255).", false, 0.f, false, 0.f, nullptr, nullptr);
	r_debug_overlay_wireframe      = ConVar::StaticCreate("r_debug_overlay_wireframe"     , "1", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "Use wireframe in debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	r_debug_draw_depth_test        = ConVar::StaticCreate("r_debug_draw_depth_test"       , "1", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "Toggle depth test for other debug draw functionality.", false, 0.f, false, 0.f, nullptr, nullptr);
	r_drawWorldMeshes              = ConVar::StaticCreate("r_drawWorldMeshes"             , "1", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "Render world meshes.", false, 0.f, false, 0.f, nullptr, nullptr);
	r_drawWorldMeshesDepthOnly     = ConVar::StaticCreate("r_drawWorldMeshesDepthOnly"    , "1", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "Render world meshes (depth only).", false, 0.f, false, 0.f, nullptr, nullptr);
	r_drawWorldMeshesDepthAtTheEnd = ConVar::StaticCreate("r_drawWorldMeshesDepthAtTheEnd", "1", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "Render world meshes (depth at the end).", false, 0.f, false, 0.f, nullptr, nullptr);
	//-------------------------------------------------------------------------
	// SERVER                                                                 |
#ifndef CLIENT_DLL
	ai_ainDumpOnLoad             = ConVar::StaticCreate("ai_ainDumpOnLoad"            , "0", FCVAR_DEVELOPMENTONLY, "Dumps AIN data from node graphs loaded from the disk on load.", false, 0.f, false, 0.f, nullptr, nullptr);
	ai_ainDebugConnect           = ConVar::StaticCreate("ai_ainDebugConnect"          , "0", FCVAR_DEVELOPMENTONLY, "Debug AIN node connections.", false, 0.f, false, 0.f, nullptr, nullptr);
	ai_script_nodes_draw_range   = ConVar::StaticCreate("ai_script_nodes_draw_range"  , "0", FCVAR_DEVELOPMENTONLY, "Debug draw AIN script nodes ranging from shift index to this cvar.", false, 0.f, false, 0.f, nullptr, nullptr);
	ai_script_nodes_draw_nearest = ConVar::StaticCreate("ai_script_nodes_draw_nearest", "1", FCVAR_DEVELOPMENTONLY, "Debug draw AIN script node links to nearest node (build order is used if null).", false, 0.f, false, 0.f, nullptr, nullptr);

	navmesh_always_reachable   = ConVar::StaticCreate("navmesh_always_reachable"   , "0"    , FCVAR_DEVELOPMENTONLY, "Marks goal poly from agent poly as reachable regardless of table data ( !slower! ).", false, 0.f, false, 0.f, nullptr, nullptr);
	navmesh_debug_type         = ConVar::StaticCreate("navmesh_debug_type"         , "0"    , FCVAR_DEVELOPMENTONLY, "NavMesh debug draw hull index.", true, 0.f, true, 4.f, nullptr, "0 = small, 1 = med_short, 2 = medium, 3 = large, 4 = extra large");
	navmesh_debug_tile_range   = ConVar::StaticCreate("navmesh_debug_tile_range"   , "0"    , FCVAR_DEVELOPMENTONLY, "NavMesh debug draw tiles ranging from shift index to this cvar.", true, 0.f, false, 0.f, nullptr, nullptr);
	navmesh_debug_camera_range = ConVar::StaticCreate("navmesh_debug_camera_range" , "2000" , FCVAR_DEVELOPMENTONLY, "Only debug draw tiles within this distance from camera origin.", true, 0.f, false, 0.f, nullptr, nullptr);
#ifndef DEDICATED
	navmesh_draw_bvtree            = ConVar::StaticCreate("navmesh_draw_bvtree"            , "-1", FCVAR_DEVELOPMENTONLY, "Draws the BVTree of the NavMesh tiles.", false, 0.f, false, 0.f, nullptr, "Index: > 0 && < mesh->m_tileCount");
	navmesh_draw_portal            = ConVar::StaticCreate("navmesh_draw_portal"            , "-1", FCVAR_DEVELOPMENTONLY, "Draws the portal of the NavMesh tiles.", false, 0.f, false, 0.f, nullptr, "Index: > 0 && < mesh->m_tileCount");
	navmesh_draw_polys             = ConVar::StaticCreate("navmesh_draw_polys"             , "-1", FCVAR_DEVELOPMENTONLY, "Draws the polys of the NavMesh tiles.", false, 0.f, false, 0.f, nullptr, "Index: > 0 && < mesh->m_tileCount");
	navmesh_draw_poly_bounds       = ConVar::StaticCreate("navmesh_draw_poly_bounds"       , "-1", FCVAR_DEVELOPMENTONLY, "Draws the bounds of the NavMesh polys.", false, 0.f, false, 0.f, nullptr, "Index: > 0 && < mesh->m_tileCount");
	navmesh_draw_poly_bounds_inner = ConVar::StaticCreate("navmesh_draw_poly_bounds_inner" , "0" , FCVAR_DEVELOPMENTONLY, "Draws the inner bounds of the NavMesh polys (requires navmesh_draw_poly_bounds).", false, 0.f, false, 0.f, nullptr, "Index: > 0 && < mesh->m_tileCount");
#endif // !DEDICATED
	sv_showconnecting  = ConVar::StaticCreate("sv_showconnecting" , "1", FCVAR_RELEASE, "Logs information about the connecting client to the console.", false, 0.f, false, 0.f, nullptr, nullptr);
	sv_globalBanlist   = ConVar::StaticCreate("sv_globalBanlist"  , "1", FCVAR_RELEASE, "Determines whether or not to use the global banned list.", false, 0.f, false, 0.f, nullptr, "0 = Disable, 1 = Enable.");
	sv_pylonVisibility = ConVar::StaticCreate("sv_pylonVisibility", "0", FCVAR_RELEASE, "Determines the visibility to the Pylon master server.", false, 0.f, false, 0.f, nullptr, "0 = Offline, 1 = Hidden, 2 = Public.");
	sv_pylonRefreshRate   = ConVar::StaticCreate("sv_pylonRefreshRate"  , "5.0", FCVAR_RELEASE, "Pylon host refresh rate (seconds).", true, 2.f, true, 8.f, nullptr, nullptr);
	sv_banlistRefreshRate = ConVar::StaticCreate("sv_banlistRefreshRate", "1.0", FCVAR_RELEASE, "Banned list refresh rate (seconds).", true, 1.f, false, 0.f, nullptr, nullptr);
	sv_statusRefreshRate  = ConVar::StaticCreate("sv_statusRefreshRate" , "0.5", FCVAR_RELEASE, "Server status refresh rate (seconds).", false, 0.f, false, 0.f, nullptr, nullptr);
	sv_autoReloadRate     = ConVar::StaticCreate("sv_autoReloadRate"    , "0"  , FCVAR_RELEASE, "Time in seconds between each server auto-reload (disabled if null). ", true, 0.f, false, 0.f, nullptr, nullptr);
	sv_simulateBots = ConVar::StaticCreate("sv_simulateBots", "1", FCVAR_RELEASE, "Simulate user commands for bots on the server.", true, 0.f, false, 0.f, nullptr, nullptr);

	sv_rcon_debug       = ConVar::StaticCreate("sv_rcon_debug"      , "0" , FCVAR_RELEASE, "Show rcon debug information ( !slower! ).", false, 0.f, false, 0.f, nullptr, nullptr);
	sv_rcon_sendlogs    = ConVar::StaticCreate("sv_rcon_sendlogs"   , "0" , FCVAR_RELEASE, "Network console logs to connected and authenticated sockets.", false, 0.f, false, 0.f, nullptr, nullptr);
	sv_rcon_banpenalty  = ConVar::StaticCreate("sv_rcon_banpenalty" , "10", FCVAR_RELEASE, "Number of minutes to ban users who fail rcon authentication.", false, 0.f, false, 0.f, nullptr, nullptr);
	sv_rcon_maxfailures = ConVar::StaticCreate("sv_rcon_maxfailures", "10", FCVAR_RELEASE, "Max number of times a user can fail rcon authentication before being banned.", true, 1.f, false, 0.f, nullptr, nullptr);
	sv_rcon_maxignores  = ConVar::StaticCreate("sv_rcon_maxignores" , "15", FCVAR_RELEASE, "Max number of times a user can ignore the instruction message before being banned.", true, 1.f, false, 0.f, nullptr, nullptr);
	sv_rcon_maxsockets  = ConVar::StaticCreate("sv_rcon_maxsockets" , "32", FCVAR_RELEASE, "Max number of accepted sockets before the server starts closing redundant sockets.", true, 1.f, false, 0.f, nullptr, nullptr);
	sv_rcon_whitelist_address = ConVar::StaticCreate("sv_rcon_whitelist_address", "", FCVAR_RELEASE, "This address is not considered a 'redundant' socket and will never be banned for failed authentication attempts.", false, 0.f, false, 0.f, nullptr, "Format: '::ffff:127.0.0.1'");

	sv_quota_stringCmdsPerSecond = ConVar::StaticCreate("sv_quota_stringCmdsPerSecond", "16", FCVAR_RELEASE, "How many string commands per second clients are allowed to submit, 0 to disallow all string commands.", true, 0.f, false, 0.f, nullptr, nullptr);
	sv_validatePersonaName  = ConVar::StaticCreate("sv_validatePersonaName" , "1" , FCVAR_RELEASE, "Validate the client's textual persona name on connect.", true, 0.f, false, 0.f, nullptr, nullptr);
	sv_minPersonaNameLength = ConVar::StaticCreate("sv_minPersonaNameLength", "4" , FCVAR_RELEASE, "The minimum length of the client's textual persona name.", true, 0.f, false, 0.f, nullptr, nullptr);
	sv_maxPersonaNameLength = ConVar::StaticCreate("sv_maxPersonaNameLength", "16", FCVAR_RELEASE, "The maximum length of the client's textual persona name.", true, 0.f, false, 0.f, nullptr, nullptr);
#endif // !CLIENT_DLL
#if !defined (GAMEDLL_S0) && !defined (GAMEDLL_S1)
	bhit_depth_test = ConVar::StaticCreate("bhit_depth_test", "0", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Use depth test for bullet ray trace overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	bhit_abs_origin = ConVar::StaticCreate("bhit_abs_origin", "1", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Draw entity's predicted abs origin upon bullet impact for trajectory debugging (requires 'r_visualizetraces' to be set!).", false, 0.f, false, 0.f, nullptr, nullptr);
#endif // !GAMEDLL_S0 && !GAMEDLL_S1
	//-------------------------------------------------------------------------
	// CLIENT                                                                 |
#ifndef DEDICATED
	cl_rcon_request_sendlogs = ConVar::StaticCreate("cl_rcon_request_sendlogs", "1" , FCVAR_RELEASE, "Request the rcon server to send console logs on connect.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_quota_stringCmdsPerSecond = ConVar::StaticCreate("cl_quota_stringCmdsPerSecond", "16" , FCVAR_RELEASE, "How many string commands per second user is allowed to submit, 0 to allow all submissions.", true, 0.f, false, 0.f, nullptr, nullptr);

	cl_notify_invert_x = ConVar::StaticCreate("cl_notify_invert_x", "0", FCVAR_DEVELOPMENTONLY, "Inverts the X offset for console notify debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_notify_invert_y = ConVar::StaticCreate("cl_notify_invert_y", "0", FCVAR_DEVELOPMENTONLY, "Inverts the Y offset for console notify debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_notify_offset_x = ConVar::StaticCreate("cl_notify_offset_x", "10", FCVAR_DEVELOPMENTONLY, "X offset for console notify debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_notify_offset_y = ConVar::StaticCreate("cl_notify_offset_y", "10", FCVAR_DEVELOPMENTONLY, "Y offset for console notify debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);

	cl_showsimstats      = ConVar::StaticCreate("cl_showsimstats"     , "0"  , FCVAR_DEVELOPMENTONLY, "Shows the tick counter for the server/client simulation and the render frame.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_simstats_invert_x = ConVar::StaticCreate("cl_simstats_invert_x", "1"  , FCVAR_DEVELOPMENTONLY, "Inverts the X offset for simulation debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_simstats_invert_y = ConVar::StaticCreate("cl_simstats_invert_y", "1"  , FCVAR_DEVELOPMENTONLY, "Inverts the Y offset for simulation debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_simstats_offset_x = ConVar::StaticCreate("cl_simstats_offset_x", "650", FCVAR_DEVELOPMENTONLY, "X offset for simulation debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_simstats_offset_y = ConVar::StaticCreate("cl_simstats_offset_y", "120", FCVAR_DEVELOPMENTONLY, "Y offset for simulation debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);

	cl_showgpustats      = ConVar::StaticCreate("cl_showgpustats"     , "0", FCVAR_DEVELOPMENTONLY, "Texture streaming debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_gpustats_invert_x = ConVar::StaticCreate("cl_gpustats_invert_x", "1", FCVAR_DEVELOPMENTONLY, "Inverts the X offset for texture streaming debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_gpustats_invert_y = ConVar::StaticCreate("cl_gpustats_invert_y", "1", FCVAR_DEVELOPMENTONLY, "Inverts the Y offset for texture streaming debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_gpustats_offset_x = ConVar::StaticCreate("cl_gpustats_offset_x", "650", FCVAR_DEVELOPMENTONLY, "X offset for texture streaming debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_gpustats_offset_y = ConVar::StaticCreate("cl_gpustats_offset_y", "105", FCVAR_DEVELOPMENTONLY, "Y offset for texture streaming debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);

	cl_showmaterialinfo      = ConVar::StaticCreate("cl_showmaterialinfo"     , "0"  , FCVAR_DEVELOPMENTONLY, "Draw info for the material under the crosshair on screen.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_materialinfo_offset_x = ConVar::StaticCreate("cl_materialinfo_offset_x", "0"  , FCVAR_DEVELOPMENTONLY, "X offset for material debug info overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_materialinfo_offset_y = ConVar::StaticCreate("cl_materialinfo_offset_y", "420", FCVAR_DEVELOPMENTONLY, "Y offset for material debug info overlay.", false, 0.f, false, 0.f, nullptr, nullptr);

	con_drawnotify = ConVar::StaticCreate("con_drawnotify", "0", FCVAR_RELEASE, "Draws the RUI console to the hud.", false, 0.f, false, 0.f, nullptr, nullptr);
	con_notifylines     = ConVar::StaticCreate("con_notifylines"    , "3" , FCVAR_MATERIAL_SYSTEM_THREAD, "Number of console lines to overlay for debugging.", true, 1.f, false, 0.f, nullptr, nullptr);
	con_notifytime      = ConVar::StaticCreate("con_notifytime"     , "6" , FCVAR_MATERIAL_SYSTEM_THREAD, "How long to display recent console text to the upper part of the game window.", false, 1.f, false, 50.f, nullptr, nullptr);

	con_notify_invert_x = ConVar::StaticCreate("con_notify_invert_x", "0" , FCVAR_MATERIAL_SYSTEM_THREAD, "Inverts the X offset for RUI console overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	con_notify_invert_y = ConVar::StaticCreate("con_notify_invert_y", "0" , FCVAR_MATERIAL_SYSTEM_THREAD, "Inverts the Y offset for RUI console overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	con_notify_offset_x = ConVar::StaticCreate("con_notify_offset_x", "10", FCVAR_MATERIAL_SYSTEM_THREAD, "X offset for RUI console overlay.", false, 1.f, false, 50.f, nullptr, nullptr);
	con_notify_offset_y = ConVar::StaticCreate("con_notify_offset_y", "10", FCVAR_MATERIAL_SYSTEM_THREAD, "Y offset for RUI console overlay.", false, 1.f, false, 50.f, nullptr, nullptr);

	con_notify_script_server_clr  = ConVar::StaticCreate("con_notify_script_server_clr", "130 120 245 255", FCVAR_MATERIAL_SYSTEM_THREAD, "Script SERVER VM RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	con_notify_script_client_clr  = ConVar::StaticCreate("con_notify_script_client_clr", "117 116 139 255", FCVAR_MATERIAL_SYSTEM_THREAD, "Script CLIENT VM RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	con_notify_script_ui_clr      = ConVar::StaticCreate("con_notify_script_ui_clr"    , "200 110 110 255", FCVAR_MATERIAL_SYSTEM_THREAD, "Script UI VM RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);

	con_notify_native_server_clr = ConVar::StaticCreate("con_notify_native_server_clr", "20 50 248 255"  , FCVAR_MATERIAL_SYSTEM_THREAD, "Native SERVER RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	con_notify_native_client_clr = ConVar::StaticCreate("con_notify_native_client_clr", "70 70 70 255"   , FCVAR_MATERIAL_SYSTEM_THREAD, "Native CLIENT RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	con_notify_native_ui_clr     = ConVar::StaticCreate("con_notify_native_ui_clr"    , "200 60 60 255"  , FCVAR_MATERIAL_SYSTEM_THREAD, "Native UI RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	con_notify_native_engine_clr = ConVar::StaticCreate("con_notify_native_engine_clr", "255 255 255 255", FCVAR_MATERIAL_SYSTEM_THREAD, "Native engine RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	con_notify_native_fs_clr     = ConVar::StaticCreate("con_notify_native_fs_clr"    , "0 100 225 255"  , FCVAR_MATERIAL_SYSTEM_THREAD, "Native FileSystem RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	con_notify_native_rtech_clr  = ConVar::StaticCreate("con_notify_native_rtech_clr" , "25 120 20 255"  , FCVAR_MATERIAL_SYSTEM_THREAD, "Native RTech RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	con_notify_native_ms_clr     = ConVar::StaticCreate("con_notify_native_ms_clr"    , "200 20 180 255" , FCVAR_MATERIAL_SYSTEM_THREAD, "Native MaterialSystem RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	con_notify_native_audio_clr  = ConVar::StaticCreate("con_notify_native_audio_clr" , "238 43 10 255"  , FCVAR_MATERIAL_SYSTEM_THREAD, "Native AudioSystem RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	con_notify_native_video_clr  = ConVar::StaticCreate("con_notify_native_video_clr" , "115 0 235 255"  , FCVAR_MATERIAL_SYSTEM_THREAD, "Native VideoSystem RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);

	con_notify_netcon_clr  = ConVar::StaticCreate("con_notify_netcon_clr" , "255 255 255 255", FCVAR_MATERIAL_SYSTEM_THREAD, "Net console RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	con_notify_common_clr  = ConVar::StaticCreate("con_notify_common_clr" , "255 140 80 255" , FCVAR_MATERIAL_SYSTEM_THREAD, "Common RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);

	con_notify_warning_clr = ConVar::StaticCreate("con_notify_warning_clr", "180 180 20 255", FCVAR_MATERIAL_SYSTEM_THREAD, "Warning RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	con_notify_error_clr   = ConVar::StaticCreate("con_notify_error_clr"  , "225 20 20 255" , FCVAR_MATERIAL_SYSTEM_THREAD, "Error RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);

	con_max_lines                 = ConVar::StaticCreate("con_max_lines"                , "1024", FCVAR_DEVELOPMENTONLY, "Maximum number of lines in the console before cleanup starts.", true, 1.f, false, 0.f, nullptr, nullptr);
	con_max_history               = ConVar::StaticCreate("con_max_history"              , "512" , FCVAR_DEVELOPMENTONLY, "Maximum number of command submission items before history cleanup starts.", true, 0.f, false, 0.f, nullptr, nullptr);
	con_suggestion_limit          = ConVar::StaticCreate("con_suggestion_limit"         , "128" , FCVAR_DEVELOPMENTONLY, "Maximum number of suggestions the autocomplete window will show for the console.", true, 0.f, false, 0.f, nullptr, nullptr);
	con_suggestion_showhelptext   = ConVar::StaticCreate("con_suggestion_showhelptext"  , "1"   , FCVAR_DEVELOPMENTONLY, "Show CommandBase help text in autocomplete window.", false, 0.f, false, 0.f, nullptr, nullptr);
	con_suggestion_showflags      = ConVar::StaticCreate("con_suggestion_showflags"     , "1"   , FCVAR_DEVELOPMENTONLY, "Show CommandBase flags in autocomplete window.", false, 0.f, false, 0.f, nullptr, nullptr);
	con_suggestion_flags_realtime = ConVar::StaticCreate("con_suggestion_flags_realtime", "1"   , FCVAR_DEVELOPMENTONLY, "Whether to show compile-time or run-time CommandBase flags.", false, 0.f, false, 0.f, nullptr, nullptr);
#endif // !DEDICATED
	//-------------------------------------------------------------------------
	// FILESYSTEM                                                             |
	fs_showWarnings                   = ConVar::StaticCreate("fs_showWarnings"                       , "0", FCVAR_DEVELOPMENTONLY, "Logs the FileSystem warnings to the console, filtered by 'fs_warning_level' ( !slower! ).", true, 0.f, true, 2.f, nullptr, "0 = log to file. 1 = 0 + log to console. 2 = 1 + log to notify.");
	fs_packedstore_entryblock_stats   = ConVar::StaticCreate("fs_packedstore_entryblock_stats"       , "0", FCVAR_DEVELOPMENTONLY, "Logs the stats of each file entry in the VPK during decompression ( !slower! ).", false, 0.f, false, 0.f, nullptr, nullptr);
	fs_packedstore_workspace          = ConVar::StaticCreate("fs_packedstore_workspace" , "platform/ship/", FCVAR_DEVELOPMENTONLY, "Determines the current VPK workspace.", false, 0.f, false, 0.f, nullptr, nullptr);
	fs_packedstore_compression_level  = ConVar::StaticCreate("fs_packedstore_compression_level", "default", FCVAR_DEVELOPMENTONLY, "Determines the VPK compression level.", false, 0.f, false, 0.f, nullptr, "fastest faster default better uber");
	fs_packedstore_max_helper_threads = ConVar::StaticCreate("fs_packedstore_max_helper_threads"    , "-1", FCVAR_DEVELOPMENTONLY, "Max # of additional \"helper\" threads to create during compression.", true, -1, true, LZHAM_MAX_HELPER_THREADS, nullptr, "Must range between [-1,LZHAM_MAX_HELPER_THREADS], where -1=max practical.");
	//-------------------------------------------------------------------------
	// MATERIALSYSTEM                                                         |
#ifndef DEDICATED
	mat_alwaysComplain = ConVar::StaticCreate("mat_alwaysComplain", "0", FCVAR_RELEASE | FCVAR_MATERIAL_SYSTEM_THREAD, "Always complain when a material is missing.", false, 0.f, false, 0.f, nullptr, nullptr);
#endif // !DEDICATED
	//-------------------------------------------------------------------------
	// SQUIRREL                                                               |
	script_show_output      = ConVar::StaticCreate("script_show_output" , "0", FCVAR_RELEASE, "Prints the VM output to the console ( !slower! ).", true, 0.f, true, 2.f, nullptr, "0 = log to file. 1 = 0 + log to console. 2 = 1 + log to notify.");
	script_show_warning     = ConVar::StaticCreate("script_show_warning", "0", FCVAR_RELEASE, "Prints the VM warning output to the console ( !slower! ).", true, 0.f, true, 2.f, nullptr, "0 = log to file. 1 = 0 + log to console. 2 = 1 + log to notify.");
	//-------------------------------------------------------------------------
	// NETCHANNEL                                                             |
	net_tracePayload           = ConVar::StaticCreate("net_tracePayload"          , "0", FCVAR_DEVELOPMENTONLY                    , "Log the payload of the send/recv datagram to a file on the disk.", false, 0.f, false, 0.f, nullptr, nullptr);
	net_encryptionEnable       = ConVar::StaticCreate("net_encryptionEnable"      , "1", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED , "Use AES encryption on game packets.", false, 0.f, false, 0.f, nullptr, nullptr);
	net_useRandomKey           = ConVar::StaticCreate("net_useRandomKey"          , "1"                        , FCVAR_RELEASE    , "Use random AES encryption key for game packets.", false, 0.f, false, 0.f, &NET_UseRandomKeyChanged_f, nullptr);
	net_processTimeBudget      = ConVar::StaticCreate("net_processTimeBudget"     ,"200"                       , FCVAR_RELEASE    , "Net message process time budget in milliseconds (removing netchannel if exceeded).", true, 0.f, false, 0.f, nullptr, "0 = disabled.");
	//-------------------------------------------------------------------------
	// NETWORKSYSTEM                                                          |
	pylon_matchmaking_hostname = ConVar::StaticCreate("pylon_matchmaking_hostname", "ms.r5reloaded.com", FCVAR_RELEASE, "Holds the pylon matchmaking hostname.", false, 0.f, false, 0.f, &MP_HostName_Changed_f, nullptr);
	pylon_host_update_interval = ConVar::StaticCreate("pylon_host_update_interval", "5"                , FCVAR_RELEASE, "Length of time in seconds between each status update interval to master server.", true, 5.f, false, 0.f, nullptr, nullptr);
	pylon_showdebuginfo        = ConVar::StaticCreate("pylon_showdebuginfo"       , "0"                , FCVAR_RELEASE, "Shows debug output for pylon.", false, 0.f, false, 0.f, nullptr, nullptr);
	//-------------------------------------------------------------------------
	// RTECH API                                                              |
	rtech_debug = ConVar::StaticCreate("rtech_debug", "0", FCVAR_DEVELOPMENTONLY, "Shows debug output for the RTech system.", false, 0.f, false, 0.f, nullptr, nullptr);
	//-------------------------------------------------------------------------
	// RUI                                                                    |
#ifndef DEDICATED
	rui_drawEnable = ConVar::StaticCreate("rui_drawEnable", "1", FCVAR_RELEASE, "Draws the RUI if set.", false, 0.f, false, 0.f, nullptr, " 1 = Draw, 0 = No Draw.");
#endif // !DEDICATED
	//-------------------------------------------------------------------------
	// MILES                                                                  |
#ifndef DEDICATED
	miles_debug = ConVar::StaticCreate("miles_debug", "0", FCVAR_RELEASE, "Enables debug prints for the Miles Sound System.", false, 0.f, false, 0.f, nullptr, " 1 = Print, 0 = No Print");
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
	fps_max                          = g_pCVar->FindVar("fps_max");
	fs_showAllReads                  = g_pCVar->FindVar("fs_showAllReads");
#ifndef DEDICATED
	cl_threaded_bone_setup           = g_pCVar->FindVar("cl_threaded_bone_setup");
#endif // !DEDICATED
	single_frame_shutdown_for_reload = g_pCVar->FindVar("single_frame_shutdown_for_reload");
	enable_debug_overlays            = g_pCVar->FindVar("enable_debug_overlays");
	debug_draw_box_depth_test        = g_pCVar->FindVar("debug_draw_box_depth_test");
	model_defaultFadeDistScale       = g_pCVar->FindVar("model_defaultFadeDistScale");
	model_defaultFadeDistMin         = g_pCVar->FindVar("model_defaultFadeDistMin");
#ifndef DEDICATED
	miles_language                   = g_pCVar->FindVar("miles_language");
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
	sv_stats = g_pCVar->FindVar("sv_stats");

	sv_updaterate_mp = g_pCVar->FindVar("sv_updaterate_mp");
	sv_updaterate_sp = g_pCVar->FindVar("sv_updaterate_sp");

	sv_showhitboxes = g_pCVar->FindVar("sv_showhitboxes");
	sv_forceChatToTeamOnly = g_pCVar->FindVar("sv_forceChatToTeamOnly");

	sv_showhitboxes->SetMin(-1); // Allow user to go over each entity manually without going out of bounds.
	sv_showhitboxes->SetMax(NUM_ENT_ENTRIES - 1);

	sv_forceChatToTeamOnly->RemoveFlags(FCVAR_DEVELOPMENTONLY);
	sv_forceChatToTeamOnly->AddFlags(FCVAR_REPLICATED);

	ai_script_nodes_draw->SetValue(-1);
#if !defined (GAMEDLL_S0) && !defined (GAMEDLL_S1) && !defined (GAMEDLL_S2)
	bhit_enable->SetValue(0);
#endif // !(GAMEDLL_S0) || !(GAMEDLL_S1) || !(GAMEDLL_S2)
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

////-----------------------------------------------------------------------------
//// Purpose: Returns the base ConVar name.
//// Output : const char*
////-----------------------------------------------------------------------------
//const char* ConVar::GetBaseName(void) const
//{
//	return m_pParent->m_pszName;
//}
//
////-----------------------------------------------------------------------------
//// Purpose: Returns the ConVar help text.
//// Output : const char*
////-----------------------------------------------------------------------------
//const char* ConVar::GetHelpText(void) const
//{
//	return m_pParent->m_pszHelpString;
//}
//
////-----------------------------------------------------------------------------
//// Purpose: Returns the ConVar usage text.
//// Output : const char*
////-----------------------------------------------------------------------------
//const char* ConVar::GetUsageText(void) const
//{
//	return m_pParent->m_pszUsageString;
//}

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
			pColorElement[0] = unsigned char(nRGBA[0]);
			pColorElement[1] = unsigned char(nRGBA[1]);
			pColorElement[2] = unsigned char(nRGBA[2]);
			pColorElement[3] = unsigned char(nRGBA[3]);

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
		m_fnChangeCallbacks[i](this, pszOldValue, NULL);
	}

	if (g_pCVar)
	{
		g_pCVar->CallGlobalChangeCallbacks(this, pszOldValue);
	}

	stackfree(pszOldValue);
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
		Warning(eDLL_T::ENGINE, "%s: Called with NULL callback; ignoring!!!\n", __FUNCTION__);
		return;
	}

	if (m_pParent->m_fnChangeCallbacks.Find(callback) != m_pParent->m_fnChangeCallbacks.InvalidIndex())
	{
		// Same ptr added twice, sigh...
		Warning(eDLL_T::ENGINE, "%s: Ignoring duplicate change callback!!!\n", __FUNCTION__);
		return;
	}

	m_pParent->m_fnChangeCallbacks.AddToTail(callback);

	// Call it immediately to set the initial value...
	if (bInvoke)
	{
		callback(this, m_Value.m_pszString, m_Value.m_fValue);
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
	int64 iArgs;						// Argument count
	const char* partial = NULL;		// Partial cvar to search for...
									// E.eg
	size_t ipLen = 0;				// Length of the partial cvar

	FileHandle_t f = FILESYSTEM_INVALID_HANDLE;         // FilePointer for logging
	bool bLogging = false;
	// Are we logging?
	iArgs = args.ArgC();		// Get count

	// Print usage?
	if (iArgs == 2 && !Q_strcasecmp(args[1], "?"))
	{
		DevMsg(eDLL_T::ENGINE, "convar_list:  [ log logfile ] [ partial ]\n");
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
	DevMsg(eDLL_T::ENGINE, "convar list\n--------------\n");

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
	for (unsigned short i = sorted.FirstInorder(); i != sorted.InvalidIndex(); i = sorted.NextInorder(i))
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
		DevMsg(eDLL_T::ENGINE, "Usage:  convar_findByFlags <string>\n");
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
	size_t len = Q_strlen(partial);

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
	size_t nSubLen = Q_strlen(pSub);

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
void CConCommandHash::Remove(CCommandHashHandle_t hHash) /*RESTRICT*/
{
	HashEntry_t* /*RESTRICT*/ entry = &m_aDataPool[hHash];
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
CConCommandHash::CCommandHashHandle_t CConCommandHash::Find(const char* name, HashKey_t hashkey) const /*RESTRICT*/
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
CConCommandHash::CCommandHashHandle_t CConCommandHash::Find(const ConCommandBase* cmd) const /*RESTRICT*/
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

///////////////////////////////////////////////////////////////////////////////
void VCVar::Attach() const
{
	DetourAttach((LPVOID*)&v_ConVar_PrintDescription, &ConVar_PrintDescription);
}
void VCVar::Detach() const
{
	DetourDetach((LPVOID*)&v_ConVar_PrintDescription, &ConVar_PrintDescription);
}
