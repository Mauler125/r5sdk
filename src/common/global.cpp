
#include "core/stdafx.h"
#include "const.h"
#include "tier0/commandline.h"
#include "tier1/cvar.h"
#include "tier1/cmd.h"
#include "tier1/NetAdr.h"
#include "tier2/curlutils.h" // For initializing the curl cvars.
#include "completion.h"
#include "callback.h"
#include "global.h"

//-----------------------------------------------------------------------------
// ENGINE                                                                     |
ConVar* sdk_fixedframe_tickinterval        = nullptr;
ConVar* single_frame_shutdown_for_reload   = nullptr;
ConVar* old_gather_props                   = nullptr;

ConVar* enable_debug_overlays              = nullptr;
ConVar* debug_draw_box_depth_test          = nullptr;

ConVar* developer                          = nullptr;
ConVar* fps_max                            = nullptr;
ConVar* fps_max_vsync                      = nullptr;

#ifndef DEDICATED
ConVar* fps_max_rt                         = nullptr;
ConVar* fps_max_rt_tolerance               = nullptr;
ConVar* fps_max_rt_sleep_threshold         = nullptr;
ConVar* fps_max_gfx                        = nullptr;
#endif // !DEDICATED

ConVar* base_tickinterval_sp               = nullptr;
ConVar* base_tickinterval_mp               = nullptr;

// Taken from S15:
ConVar* usercmd_frametime_max              = nullptr;
ConVar* usercmd_frametime_min              = nullptr;

ConVar* usercmd_dualwield_enable           = nullptr;

ConVar* staticProp_no_fade_scalar          = nullptr;
ConVar* staticProp_gather_size_weight      = nullptr;

ConVar* model_defaultFadeDistScale         = nullptr;
ConVar* model_defaultFadeDistMin           = nullptr;

ConVar* ip_cvar                            = nullptr;
ConVar* hostname                           = nullptr;
ConVar* hostdesc                           = nullptr;
ConVar* hostip                             = nullptr;
ConVar* hostport                           = nullptr;

ConVar* host_hasIrreversibleShutdown       = nullptr;
ConVar* host_timescale                     = nullptr;

ConVar* mp_gamemode                        = nullptr;

ConVar* rcon_address                       = nullptr;
ConVar* rcon_password                      = nullptr;

ConVar* enable_CmdKeyValues                = nullptr;

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

ConVar* gfx_nvnUseLowLatency               = nullptr;
ConVar* gfx_nvnUseLowLatencyBoost          = nullptr;
#endif // !DEDICATED

ConVar* stream_overlay                     = nullptr;
ConVar* stream_overlay_mode                = nullptr;
//-----------------------------------------------------------------------------
// SHARED                                                                     |
ConVar* modsystem_enable                   = nullptr;
ConVar* modsystem_debug                    = nullptr;

ConVar* eula_version                       = nullptr;
ConVar* eula_version_accepted              = nullptr;

ConVar* promo_version_accepted             = nullptr;

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

ConVar* sv_language                        = nullptr;

ConVar* sv_showconnecting                  = nullptr;
ConVar* sv_globalBanlist                   = nullptr;
ConVar* sv_pylonVisibility                 = nullptr;
ConVar* sv_pylonRefreshRate                = nullptr;
ConVar* sv_banlistRefreshRate              = nullptr;
ConVar* sv_statusRefreshRate               = nullptr;
ConVar* sv_forceChatToTeamOnly             = nullptr;

ConVar* sv_single_core_dedi                = nullptr;

ConVar* sv_maxunlag = nullptr;
ConVar* sv_unlag_clamp                     = nullptr;
ConVar* sv_clockcorrection_msecs = nullptr;

ConVar* sv_updaterate_sp                   = nullptr;
ConVar* sv_updaterate_mp                   = nullptr;

ConVar* sv_autoReloadRate                  = nullptr;

ConVar* sv_simulateBots                    = nullptr;
ConVar* sv_showhitboxes                    = nullptr;
ConVar* sv_stats                           = nullptr;

ConVar* sv_quota_stringCmdsPerSecond       = nullptr;

ConVar* sv_validatePersonaName             = nullptr;
ConVar* sv_minPersonaNameLength            = nullptr;
ConVar* sv_maxPersonaNameLength            = nullptr;

ConVar* sv_onlineAuthEnable                = nullptr;
ConVar* sv_onlineAuthValidateExpiry        = nullptr;
ConVar* sv_onlineAuthExpiryTolerance       = nullptr;

ConVar* sv_onlineAuthValidateIssuedAt      = nullptr;
ConVar* sv_onlineAuthIssuedAtTolerance     = nullptr;

ConVar* sv_voiceEcho                       = nullptr;
ConVar* sv_voiceenable                     = nullptr;
ConVar* sv_alltalk                         = nullptr;

ConVar* player_userCmdsQueueWarning        = nullptr;

//#ifdef DEDICATED
ConVar* sv_rcon_debug                      = nullptr;
ConVar* sv_rcon_sendlogs                   = nullptr;
//ConVar* sv_rcon_banpenalty                 = nullptr; // TODO
ConVar* sv_rcon_maxfailures                = nullptr;
ConVar* sv_rcon_maxignores                 = nullptr;
ConVar* sv_rcon_maxsockets                 = nullptr;
ConVar* sv_rcon_maxconnections             = nullptr;
ConVar* sv_rcon_maxpacketsize              = nullptr;
ConVar* sv_rcon_whitelist_address          = nullptr;
//#endif // DEDICATED
#endif // !CLIENT_DLL
ConVar* sv_allowClientSideCfgExec          = nullptr;
ConVar* sv_cheats                          = nullptr;
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
ConVar* cl_rcon_inputonly           = nullptr;
ConVar* cl_quota_stringCmdsPerSecond       = nullptr;

ConVar* cl_move_use_dt                     = nullptr;
ConVar* cl_updaterate_mp                   = nullptr;

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

ConVar* cl_language                        = nullptr;

ConVar* cl_onlineAuthEnable                = nullptr;
ConVar* cl_onlineAuthToken                 = nullptr;
ConVar* cl_onlineAuthTokenSignature1       = nullptr;
ConVar* cl_onlineAuthTokenSignature2       = nullptr;

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
ConVar* con_suggest_limit               = nullptr;
ConVar* con_suggest_showhelptext        = nullptr;
ConVar* con_suggest_showflags           = nullptr;

ConVar* origin_disconnectWhenOffline       = nullptr;
ConVar* discord_updatePresence = nullptr;

ConVar* settings_reflex = nullptr;
ConVar* serverbrowser_hideEmptyServers = nullptr;
ConVar* serverbrowser_mapFilter = nullptr;
ConVar* serverbrowser_gamemodeFilter = nullptr;
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

ConVar* net_datablock_networkLossForSlowSpeed = nullptr;

ConVar* pylon_matchmaking_hostname         = nullptr;
ConVar* pylon_host_update_interval         = nullptr;
ConVar* pylon_showdebuginfo                = nullptr;

ConVar* ssl_verify_peer                    = nullptr;
ConVar* curl_timeout                       = nullptr;
ConVar* curl_debug                         = nullptr;
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
// Purpose: initialize ConVar's
//-----------------------------------------------------------------------------
void ConVar_StaticInit(void)
{
	//-------------------------------------------------------------------------
	// ENGINE                                                                 |
	hostdesc                       = ConVar::StaticCreate("hostdesc", "", FCVAR_RELEASE, "Host game server description.", false, 0.f, false, 0.f, nullptr, nullptr);
	sdk_fixedframe_tickinterval    = ConVar::StaticCreate("sdk_fixedframe_tickinterval", "0.01", FCVAR_RELEASE, "The tick interval used by the SDK fixed frame.", false, 0.f, false, 0.f, nullptr, nullptr);

	curl_debug      = ConVar::StaticCreate("curl_debug"     , "0" , FCVAR_DEVELOPMENTONLY, "Determines whether or not to enable curl debug logging.", false, 0.f, false, 0.f, nullptr, "1 = curl logs; 0 (zero) = no logs");
	curl_timeout    = ConVar::StaticCreate("curl_timeout"   , "15", FCVAR_DEVELOPMENTONLY, "Maximum time in seconds a curl transfer operation could take.", false, 0.f, false, 0.f, nullptr, nullptr);
	ssl_verify_peer = ConVar::StaticCreate("ssl_verify_peer", "1" , FCVAR_DEVELOPMENTONLY, "Verify the authenticity of the peer's SSL certificate.", false, 0.f, false, 0.f, nullptr, "1 = curl verifies; 0 (zero) = no verification");

	rcon_address  = ConVar::StaticCreate("rcon_address",  "[loopback]:37015", FCVAR_SERVER_CANNOT_QUERY | FCVAR_DONTRECORD | FCVAR_RELEASE, "Remote server access address.", false, 0.f, false, 0.f, nullptr, nullptr);
	rcon_password = ConVar::StaticCreate("rcon_password", ""                , FCVAR_SERVER_CANNOT_QUERY | FCVAR_DONTRECORD | FCVAR_RELEASE, "Remote server access password (rcon is disabled if empty).", false, 0.f, false, 0.f, &RCON_PasswordChanged_f, nullptr);

	enable_CmdKeyValues = ConVar::StaticCreate("enable_CmdKeyValues", "0", FCVAR_DEVELOPMENTONLY, "Toggle CmdKeyValues transmit and receive.", false, 0.f, false, 0.f, nullptr, nullptr);

	r_debug_overlay_nodecay        = ConVar::StaticCreate("r_debug_overlay_nodecay"       , "0", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "Keeps all debug overlays alive regardless of their lifetime. Use command 'clear_debug_overlays' to clear everything.", false, 0.f, false, 0.f, nullptr, nullptr);
	r_debug_overlay_invisible      = ConVar::StaticCreate("r_debug_overlay_invisible"     , "1", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "Show invisible debug overlays (alpha < 1 = 255).", false, 0.f, false, 0.f, nullptr, nullptr);
	r_debug_overlay_wireframe      = ConVar::StaticCreate("r_debug_overlay_wireframe"     , "1", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "Use wireframe in debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	r_debug_draw_depth_test        = ConVar::StaticCreate("r_debug_draw_depth_test"       , "1", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "Toggle depth test for other debug draw functionality.", false, 0.f, false, 0.f, nullptr, nullptr);
	r_drawWorldMeshes              = ConVar::StaticCreate("r_drawWorldMeshes"             , "1", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "Render world meshes.", false, 0.f, false, 0.f, nullptr, nullptr);
	r_drawWorldMeshesDepthOnly     = ConVar::StaticCreate("r_drawWorldMeshesDepthOnly"    , "1", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "Render world meshes (depth only).", false, 0.f, false, 0.f, nullptr, nullptr);
	r_drawWorldMeshesDepthAtTheEnd = ConVar::StaticCreate("r_drawWorldMeshesDepthAtTheEnd", "1", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "Render world meshes (depth at the end).", false, 0.f, false, 0.f, nullptr, nullptr);

#ifndef DEDICATED
	fps_max_rt                 = ConVar::StaticCreate("fps_max_rt", "0", FCVAR_RELEASE, "Frame rate limiter within the render thread. -1 indicates use the desktop refresh. 0 is disabled.", true, -1.f, true, 295.f, nullptr, nullptr);
	fps_max_rt_tolerance       = ConVar::StaticCreate("fps_max_rt_tolerance", "0.25", FCVAR_RELEASE, "Maximum amount of frame time before frame limiter restarts.", true, 0.f, false, 0.f, nullptr, nullptr);
	fps_max_rt_sleep_threshold = ConVar::StaticCreate("fps_max_rt_sleep_threshold", "0.016666667", FCVAR_RELEASE, "Frame limiter starts to sleep when frame time exceeds this threshold.", true, 0.f, false, 0.f, nullptr, nullptr);

	fps_max_gfx = ConVar::StaticCreate("fps_max_gfx", "0", FCVAR_RELEASE, "Frame rate limiter using NVIDIA Reflex Low Latency SDK. -1 indicates use the desktop refresh. 0 is disabled.", true, -1.f, true, 295.f, &GFX_NVN_Changed_f, nullptr);
	gfx_nvnUseLowLatency      = ConVar::StaticCreate("gfx_nvnUseLowLatency"     , "1", FCVAR_RELEASE | FCVAR_ARCHIVE, "Enables NVIDIA Reflex Low Latency SDK."  , false, 0.f, false, 0.f, &GFX_NVN_Changed_f, nullptr);
	gfx_nvnUseLowLatencyBoost = ConVar::StaticCreate("gfx_nvnUseLowLatencyBoost", "0", FCVAR_RELEASE | FCVAR_ARCHIVE, "Enables NVIDIA Reflex Low Latency Boost.", false, 0.f, false, 0.f, &GFX_NVN_Changed_f, nullptr);
#endif // !DEDICATED

	//-------------------------------------------------------------------------
	// SHARED                                                                 |
	modsystem_enable = ConVar::StaticCreate("modsystem_enable", "1", FCVAR_RELEASE, "Enable the modsystem.", false, 0.f, false, 0.f, nullptr, nullptr);
	modsystem_debug  = ConVar::StaticCreate("modsystem_debug" , "0", FCVAR_RELEASE, "Debug the modsystem." , false, 0.f, false, 0.f, nullptr, nullptr);

	// TODO: Make this release|playerprofile (show promo only once until version change) or leave as release (show promo everytime user restarted the game)?
	promo_version_accepted = ConVar::StaticCreate("promo_version_accepted", "0", FCVAR_RELEASE /*| FCVAR_ARCHIVE_PLAYERPROFILE*/, "The accepted promo version.", false, 0.f, false, 0.f, nullptr, nullptr);

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

	sv_language = ConVar::StaticCreate("sv_language", "english", FCVAR_RELEASE, "Language of the server. Sent to MasterServer for localising error messages.", false, 0.f, false, 0.f, LanguageChanged_f, nullptr);

	sv_unlag_clamp = ConVar::StaticCreate("sv_unlag_clamp", "0", FCVAR_RELEASE, "Clamp the difference between the current time and received command time to sv_maxunlag + sv_clockcorrection_msecs.", false, 0.f, false, 0.f, nullptr, nullptr);

	sv_showconnecting  = ConVar::StaticCreate("sv_showconnecting" , "1", FCVAR_RELEASE, "Logs information about the connecting client to the console.", false, 0.f, false, 0.f, nullptr, nullptr);
	sv_globalBanlist   = ConVar::StaticCreate("sv_globalBanlist"  , "1", FCVAR_RELEASE, "Determines whether or not to use the global banned list.", false, 0.f, false, 0.f, nullptr, "0 = Disable, 1 = Enable.");
	sv_pylonVisibility = ConVar::StaticCreate("sv_pylonVisibility", "0", FCVAR_RELEASE, "Determines the visibility to the Pylon master server.", false, 0.f, false, 0.f, nullptr, "0 = Offline, 1 = Hidden, 2 = Public.");
	sv_pylonRefreshRate   = ConVar::StaticCreate("sv_pylonRefreshRate"  , "5.0" , FCVAR_DEVELOPMENTONLY, "Pylon host refresh rate (seconds).", true, 2.f, true, 8.f, nullptr, nullptr);
	sv_banlistRefreshRate = ConVar::StaticCreate("sv_banlistRefreshRate", "30.0", FCVAR_DEVELOPMENTONLY, "Banned list refresh rate (seconds).", true, 1.f, false, 0.f, nullptr, nullptr);
	sv_statusRefreshRate  = ConVar::StaticCreate("sv_statusRefreshRate" , "0.5", FCVAR_RELEASE, "Server status refresh rate (seconds).", true, 0.f, false, 0.f, nullptr, nullptr);
	sv_autoReloadRate     = ConVar::StaticCreate("sv_autoReloadRate"    , "0"  , FCVAR_RELEASE, "Time in seconds between each server auto-reload (disabled if null).", true, 0.f, false, 0.f, nullptr, nullptr);
	sv_simulateBots = ConVar::StaticCreate("sv_simulateBots", "1", FCVAR_RELEASE, "Simulate user commands for bots on the server.", true, 0.f, false, 0.f, nullptr, nullptr);

	sv_rcon_debug       = ConVar::StaticCreate("sv_rcon_debug"      , "0" , FCVAR_RELEASE, "Show rcon debug information ( !slower! ).", false, 0.f, false, 0.f, nullptr, nullptr);
	sv_rcon_sendlogs    = ConVar::StaticCreate("sv_rcon_sendlogs"   , "0" , FCVAR_RELEASE, "Network console logs to connected and authenticated sockets.", false, 0.f, false, 0.f, nullptr, nullptr);
	//sv_rcon_banpenalty  = ConVar::StaticCreate("sv_rcon_banpenalty" , "10", FCVAR_RELEASE, "Number of minutes to ban users who fail rcon authentication.", false, 0.f, false, 0.f, nullptr, nullptr);
	sv_rcon_maxfailures = ConVar::StaticCreate("sv_rcon_maxfailures", "10", FCVAR_RELEASE, "Max number of times an user can fail rcon authentication before being banned.", true, 1.f, false, 0.f, nullptr, nullptr);
	sv_rcon_maxignores  = ConVar::StaticCreate("sv_rcon_maxignores" , "15", FCVAR_RELEASE, "Max number of times an user can ignore the instruction message before being banned.", true, 1.f, false, 0.f, nullptr, nullptr);
	sv_rcon_maxsockets  = ConVar::StaticCreate("sv_rcon_maxsockets" , "32", FCVAR_RELEASE, "Max number of accepted sockets before the server starts closing redundant sockets.", true, 1.f, true, MAX_PLAYERS, nullptr, nullptr);
	sv_rcon_maxconnections    = ConVar::StaticCreate("sv_rcon_maxconnections"   , "1"   , FCVAR_RELEASE, "Max number of authenticated connections before the server closes the listen socket.", true, 1.f, true, MAX_PLAYERS, &RCON_ConnectionCountChanged_f, nullptr);
	sv_rcon_maxpacketsize     = ConVar::StaticCreate("sv_rcon_maxpacketsize"    , "1024", FCVAR_RELEASE, "Max number of bytes allowed in a command packet from a non-authenticated netconsole.", true, 0.f, false, 0.f, nullptr, nullptr);
	sv_rcon_whitelist_address = ConVar::StaticCreate("sv_rcon_whitelist_address", ""    , FCVAR_RELEASE, "This address is not considered a 'redundant' socket and will never be banned for failed authentication attempts.", false, 0.f, false, 0.f, &RCON_WhiteListAddresChanged_f, "Format: '::ffff:127.0.0.1'");

	sv_quota_stringCmdsPerSecond = ConVar::StaticCreate("sv_quota_stringCmdsPerSecond", "16", FCVAR_RELEASE, "How many string commands per second clients are allowed to submit, 0 to disallow all string commands.", true, 0.f, false, 0.f, nullptr, nullptr);
	sv_validatePersonaName  = ConVar::StaticCreate("sv_validatePersonaName" , "1" , FCVAR_RELEASE, "Validate the client's textual persona name on connect.", true, 0.f, false, 0.f, nullptr, nullptr);
	sv_minPersonaNameLength = ConVar::StaticCreate("sv_minPersonaNameLength", "4" , FCVAR_RELEASE, "The minimum length of the client's textual persona name.", true, 0.f, false, 0.f, nullptr, nullptr);
	sv_maxPersonaNameLength = ConVar::StaticCreate("sv_maxPersonaNameLength", "16", FCVAR_RELEASE, "The maximum length of the client's textual persona name.", true, 0.f, false, 0.f, nullptr, nullptr);

	sv_onlineAuthEnable            = ConVar::StaticCreate("sv_onlineAuthEnable"           , "1" , FCVAR_RELEASE, "Enables the server-side online authentication system.", true, 0.f, true, 1.f, nullptr, nullptr);
	sv_onlineAuthValidateExpiry    = ConVar::StaticCreate("sv_onlineAuthValidateExpiry"   , "1" , FCVAR_RELEASE, "Validate the online authentication token 'expiry' claim.", true, 0.f, true, 1.f, nullptr, nullptr);
	sv_onlineAuthExpiryTolerance   = ConVar::StaticCreate("sv_onlineAuthExpiryTolerance"  , "1" , FCVAR_RELEASE, "The online authentication token 'expiry' claim tolerance in seconds.", true, 0.f, true, float(UINT8_MAX), nullptr, "Must range between [0,255]");
	sv_onlineAuthValidateIssuedAt  = ConVar::StaticCreate("sv_onlineAuthValidateIssuedAt" , "1" , FCVAR_RELEASE, "Validate the online authentication token 'issued at' claim.", true, 0.f, true, 1.f, nullptr, nullptr);
	sv_onlineAuthIssuedAtTolerance = ConVar::StaticCreate("sv_onlineAuthIssuedAtTolerance", "30", FCVAR_RELEASE, "The online authentication token 'issued at' claim tolerance in seconds.", true, 0.f, true, float(UINT8_MAX), nullptr, "Must range between [0,255]");
#endif // !CLIENT_DLL
	sv_allowClientSideCfgExec = ConVar::StaticCreate("sv_allowClientSideCfgExec", "0", FCVAR_REPLICATED | FCVAR_RELEASE, "Allow clients to execute script files while being connected to this server.", false, 0.f, false, 0.f, nullptr, nullptr);
#if !defined (GAMEDLL_S0) && !defined (GAMEDLL_S1)
	bhit_depth_test = ConVar::StaticCreate("bhit_depth_test", "0", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Use depth test for bullet ray trace overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	bhit_abs_origin = ConVar::StaticCreate("bhit_abs_origin", "1", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Draw entity's predicted abs origin upon bullet impact for trajectory debugging (requires 'r_visualizetraces' to be set!).", false, 0.f, false, 0.f, nullptr, nullptr);
#endif // !GAMEDLL_S0 && !GAMEDLL_S1
	//-------------------------------------------------------------------------
	// CLIENT                                                                 |
#ifndef DEDICATED
	cl_rcon_inputonly = ConVar::StaticCreate("cl_rcon_inputonly", "0" , FCVAR_RELEASE, "Tells the rcon server whether or not we are input only.", false, 0.f, false, 0.f, RCON_InputOnlyChanged_f, nullptr);
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

	cl_onlineAuthEnable          = ConVar::StaticCreate("cl_onlineAuthEnable"         ,"1", FCVAR_RELEASE, "Enables the client-side online authentication system.", true, 0.f, true, 1.f, nullptr, nullptr);
	cl_onlineAuthToken           = ConVar::StaticCreate("cl_onlineAuthToken"          , "", FCVAR_HIDDEN | FCVAR_USERINFO | FCVAR_DONTRECORD | FCVAR_SERVER_CANNOT_QUERY | FCVAR_PLATFORM_SYSTEM, "The client's online authentication token.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_onlineAuthTokenSignature1 = ConVar::StaticCreate("cl_onlineAuthTokenSignature1", "", FCVAR_HIDDEN | FCVAR_USERINFO | FCVAR_DONTRECORD | FCVAR_SERVER_CANNOT_QUERY | FCVAR_PLATFORM_SYSTEM, "The client's online authentication token signature.", false, 0.f, false, 0.f, nullptr, "Primary");
	cl_onlineAuthTokenSignature2 = ConVar::StaticCreate("cl_onlineAuthTokenSignature2", "", FCVAR_HIDDEN | FCVAR_USERINFO | FCVAR_DONTRECORD | FCVAR_SERVER_CANNOT_QUERY | FCVAR_PLATFORM_SYSTEM, "The client's online authentication token signature.", false, 0.f, false, 0.f, nullptr, "Secondary");

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

	con_notify_netcon_clr  = ConVar::StaticCreate("con_notify_netcon_clr" , "255 255 255 255", FCVAR_MATERIAL_SYSTEM_THREAD, "Netconsole RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	con_notify_common_clr  = ConVar::StaticCreate("con_notify_common_clr" , "255 140 80 255" , FCVAR_MATERIAL_SYSTEM_THREAD, "Common RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);

	con_notify_warning_clr = ConVar::StaticCreate("con_notify_warning_clr", "180 180 20 255", FCVAR_MATERIAL_SYSTEM_THREAD, "Warning RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	con_notify_error_clr   = ConVar::StaticCreate("con_notify_error_clr"  , "225 20 20 255" , FCVAR_MATERIAL_SYSTEM_THREAD, "Error RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);

	con_max_lines            = ConVar::StaticCreate("con_max_lines"             , "1024", FCVAR_DEVELOPMENTONLY, "Maximum number of lines in the console before cleanup starts.", true, 1.f, false, 0.f, nullptr, nullptr);
	con_max_history          = ConVar::StaticCreate("con_max_history"           , "512" , FCVAR_DEVELOPMENTONLY, "Maximum number of command submission items before history cleanup starts.", true, 0.f, false, 0.f, nullptr, nullptr);
	con_suggest_limit        = ConVar::StaticCreate("con_suggest_limit"         , "128" , FCVAR_DEVELOPMENTONLY, "Maximum number of suggestions the autocomplete window will show for the console.", true, 0.f, false, 0.f, nullptr, nullptr);
	con_suggest_showhelptext = ConVar::StaticCreate("con_suggest_showhelptext"  , "1"   , FCVAR_DEVELOPMENTONLY, "Show CommandBase help text in autocomplete window.", false, 0.f, false, 0.f, nullptr, nullptr);
	con_suggest_showflags    = ConVar::StaticCreate("con_suggest_showflags"     , "1"   , FCVAR_DEVELOPMENTONLY, "Show CommandBase flags in autocomplete window.", false, 0.f, false, 0.f, nullptr, nullptr);

	settings_reflex                = ConVar::StaticCreate("settings_reflex", "1", FCVAR_RELEASE, "Selected NVIDIA Reflex mode.", false, 0.f, false, 0.f, nullptr, "0 = Off. 1 = On. 2 = On + Boost.");
	serverbrowser_hideEmptyServers = ConVar::StaticCreate("serverbrowser_hideEmptyServers", "0", FCVAR_RELEASE, "Hide empty servers in the server browser.", false, 0.f, false, 0.f, nullptr, nullptr);
	serverbrowser_mapFilter        = ConVar::StaticCreate("serverbrowser_mapFilter", "0", FCVAR_RELEASE, "Filter servers by map in the server browser.", false, 0.f, false, 0.f, nullptr, nullptr);
	serverbrowser_gamemodeFilter   = ConVar::StaticCreate("serverbrowser_gamemodeFilter", "0", FCVAR_RELEASE, "Filter servers by gamemode in the server browser.", false, 0.f, false, 0.f, nullptr, nullptr);

#endif // !DEDICATED
	// Taken from S15:
	usercmd_frametime_max = ConVar::StaticCreate("usercmd_frametime_max", "0.100",    FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY,  "The largest amount of simulation seconds a UserCmd can have.", false, 0.f, false, 0.f, nullptr, nullptr);
	usercmd_frametime_min = ConVar::StaticCreate("usercmd_frametime_min", "0.002857", FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY, "The smallest amount of simulation seconds a UserCmd can have.", false, 0.f, false, 0.f, nullptr, nullptr);

	usercmd_dualwield_enable = ConVar::StaticCreate("usercmd_dualwield_enable", "0", FCVAR_REPLICATED | FCVAR_RELEASE, "Allows setting dual wield cycle slots, and activating multiple inventory weapons from UserCmd.", false, 0.f, false, 0.f, nullptr, nullptr);
	//-------------------------------------------------------------------------
	// FILESYSTEM                                                             |
	fs_showWarnings                   = ConVar::StaticCreate("fs_showWarnings"                       , "0", FCVAR_DEVELOPMENTONLY, "Logs the FileSystem warnings to the console, filtered by 'fs_warning_level' ( !slower! ).", true, 0.f, true, 2.f, nullptr, "0 = log to file. 1 = 0 + log to console. 2 = 1 + log to notify");
	fs_packedstore_entryblock_stats   = ConVar::StaticCreate("fs_packedstore_entryblock_stats"       , "0", FCVAR_DEVELOPMENTONLY, "Logs the stats of each file entry in the VPK during decompression ( !slower! ).", false, 0.f, false, 0.f, nullptr, nullptr);
	fs_packedstore_workspace          = ConVar::StaticCreate("fs_packedstore_workspace"           , "ship", FCVAR_DEVELOPMENTONLY, "Determines the current VPK workspace.", false, 0.f, false, 0.f, nullptr, nullptr);
	fs_packedstore_compression_level  = ConVar::StaticCreate("fs_packedstore_compression_level", "default", FCVAR_DEVELOPMENTONLY, "Determines the VPK compression level.", false, 0.f, false, 0.f, nullptr, "fastest faster default better uber");
	fs_packedstore_max_helper_threads = ConVar::StaticCreate("fs_packedstore_max_helper_threads"    , "-1", FCVAR_DEVELOPMENTONLY, "Max # of additional \"helper\" threads to create during compression.", true, -1, true, LZHAM_MAX_HELPER_THREADS, nullptr, "Must range between [-1,LZHAM_MAX_HELPER_THREADS], where -1=max practical");
	//-------------------------------------------------------------------------
	// MATERIALSYSTEM                                                         |
#ifndef DEDICATED
	mat_alwaysComplain = ConVar::StaticCreate("mat_alwaysComplain", "0", FCVAR_RELEASE | FCVAR_MATERIAL_SYSTEM_THREAD, "Always complain when a material is missing.", false, 0.f, false, 0.f, nullptr, nullptr);
#endif // !DEDICATED
	//-------------------------------------------------------------------------
	// SQUIRREL                                                               |
	script_show_output      = ConVar::StaticCreate("script_show_output" , "0", FCVAR_RELEASE, "Prints the VM output to the console ( !slower! ).", true, 0.f, true, 2.f, nullptr, "0 = log to file. 1 = 0 + log to console. 2 = 1 + log to notify");
	script_show_warning     = ConVar::StaticCreate("script_show_warning", "0", FCVAR_RELEASE, "Prints the VM warning output to the console ( !slower! ).", true, 0.f, true, 2.f, nullptr, "0 = log to file. 1 = 0 + log to console. 2 = 1 + log to notify");
	//-------------------------------------------------------------------------
	// NETCHANNEL                                                             |
	net_tracePayload           = ConVar::StaticCreate("net_tracePayload"          , "0", FCVAR_DEVELOPMENTONLY                    , "Log the payload of the send/recv datagram to a file on the disk.", false, 0.f, false, 0.f, nullptr, nullptr);
	net_encryptionEnable       = ConVar::StaticCreate("net_encryptionEnable"      , "1", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED , "Use AES encryption on game packets.", false, 0.f, false, 0.f, nullptr, nullptr);
	net_useRandomKey           = ConVar::StaticCreate("net_useRandomKey"          , "1"                        , FCVAR_RELEASE    , "Use random AES encryption key for game packets.", false, 0.f, false, 0.f, &NET_UseRandomKeyChanged_f, nullptr);
	net_processTimeBudget      = ConVar::StaticCreate("net_processTimeBudget"     ,"200"                       , FCVAR_RELEASE    , "Net message process time budget in milliseconds (removing netchannel if exceeded).", true, 0.f, false, 0.f, nullptr, "0 = disabled");
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
	rui_drawEnable = ConVar::StaticCreate("rui_drawEnable", "1", FCVAR_RELEASE, "Draws the RUI if set.", false, 0.f, false, 0.f, nullptr, "1 = draw; 0 (zero) = no draw");
#endif // !DEDICATED
	//-------------------------------------------------------------------------
	// MILES                                                                  |
#ifndef DEDICATED
	miles_debug = ConVar::StaticCreate("miles_debug", "0", FCVAR_RELEASE, "Enables debug prints for the Miles Sound System.", false, 0.f, false, 0.f, nullptr, "1 = print; 0 (zero) = no print");
#endif // !DEDICATED
	//-------------------------------------------------------------------------
}

//-----------------------------------------------------------------------------
// Purpose: initialize shipped ConVar's
//-----------------------------------------------------------------------------
void ConVar_InitShipped(void)
{
#ifndef CLIENT_DLL
	ai_script_nodes_draw             = g_pCVar->FindVar("ai_script_nodes_draw");
#if !defined (GAMEDLL_S0) && !defined (GAMEDLL_S1)
	bhit_enable                      = g_pCVar->FindVar("bhit_enable");
#endif // !GAMEDLL_S0 && !GAMEDLL_S1
#endif // !CLIENT_DLL
	developer                        = g_pCVar->FindVar("developer");
	fps_max                          = g_pCVar->FindVar("fps_max");
	fps_max_vsync                    = g_pCVar->FindVar("fps_max_vsync");
	base_tickinterval_sp             = g_pCVar->FindVar("base_tickinterval_sp");
	base_tickinterval_mp             = g_pCVar->FindVar("base_tickinterval_mp");
	fs_showAllReads                  = g_pCVar->FindVar("fs_showAllReads");

	eula_version                     = g_pCVar->FindVar("eula_version");
	eula_version_accepted            = g_pCVar->FindVar("eula_version_accepted");
#ifndef DEDICATED
	cl_move_use_dt                   = g_pCVar->FindVar("cl_move_use_dt");
	cl_updaterate_mp                 = g_pCVar->FindVar("cl_updaterate_mp");
	cl_threaded_bone_setup           = g_pCVar->FindVar("cl_threaded_bone_setup");
	cl_language                      = g_pCVar->FindVar("cl_language");
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
	sv_cheats                        = g_pCVar->FindVar("sv_cheats");
	sv_visualizetraces               = g_pCVar->FindVar("sv_visualizetraces");
	sv_visualizetraces_duration      = g_pCVar->FindVar("sv_visualizetraces_duration");
	old_gather_props                 = g_pCVar->FindVar("old_gather_props");
#ifndef DEDICATED
	origin_disconnectWhenOffline     = g_pCVar->FindVar("origin_disconnectWhenOffline");
	discord_updatePresence           = g_pCVar->FindVar("discord_updatePresence");
#endif // !DEDICATED
	mp_gamemode                      = g_pCVar->FindVar("mp_gamemode");
	ip_cvar                          = g_pCVar->FindVar("ip");
	hostname                         = g_pCVar->FindVar("hostname");
	hostip                           = g_pCVar->FindVar("hostip");
	hostport                         = g_pCVar->FindVar("hostport");
	host_hasIrreversibleShutdown     = g_pCVar->FindVar("host_hasIrreversibleShutdown");
	host_timescale                   = g_pCVar->FindVar("host_timescale");
	net_datablock_networkLossForSlowSpeed = g_pCVar->FindVar("net_datablock_networkLossForSlowSpeed");
	net_usesocketsforloopback        = g_pCVar->FindVar("net_usesocketsforloopback");
#ifndef CLIENT_DLL
	sv_stats = g_pCVar->FindVar("sv_stats");

	sv_maxunlag = g_pCVar->FindVar("sv_maxunlag");
	sv_clockcorrection_msecs = g_pCVar->FindVar("sv_clockcorrection_msecs");

	sv_updaterate_sp = g_pCVar->FindVar("sv_updaterate_sp");
	sv_updaterate_mp = g_pCVar->FindVar("sv_updaterate_mp");

	sv_showhitboxes = g_pCVar->FindVar("sv_showhitboxes");
	sv_forceChatToTeamOnly = g_pCVar->FindVar("sv_forceChatToTeamOnly");

	sv_single_core_dedi = g_pCVar->FindVar("sv_single_core_dedi");

	sv_voiceenable = g_pCVar->FindVar("sv_voiceenable");
	sv_voiceEcho = g_pCVar->FindVar("sv_voiceEcho");
	sv_alltalk = g_pCVar->FindVar("sv_alltalk");
	player_userCmdsQueueWarning = g_pCVar->FindVar("player_userCmdsQueueWarning");

	sv_updaterate_sp->RemoveFlags(FCVAR_DEVELOPMENTONLY);
	sv_updaterate_mp->RemoveFlags(FCVAR_DEVELOPMENTONLY);

	sv_showhitboxes->SetMin(-1); // Allow user to go over each entity manually without going out of bounds.
	sv_showhitboxes->SetMax(NUM_ENT_ENTRIES - 1);

	sv_forceChatToTeamOnly->RemoveFlags(FCVAR_DEVELOPMENTONLY);
	sv_forceChatToTeamOnly->AddFlags(FCVAR_REPLICATED);

	sv_single_core_dedi->RemoveFlags(FCVAR_DEVELOPMENTONLY);

	ai_script_nodes_draw->SetValue(-1);
#if !defined (GAMEDLL_S0) && !defined (GAMEDLL_S1) && !defined (GAMEDLL_S2)
	bhit_enable->SetValue(0);
#endif // !(GAMEDLL_S0) || !(GAMEDLL_S1) || !(GAMEDLL_S2)
#endif // !CLIENT_DLL
#ifndef DEDICATED
	cl_updaterate_mp->RemoveFlags(FCVAR_DEVELOPMENTONLY);

	cl_threaded_bone_setup->RemoveFlags(FCVAR_DEVELOPMENTONLY);
	rui_defaultDebugFontFace->RemoveFlags(FCVAR_DEVELOPMENTONLY);
	origin_disconnectWhenOffline->RemoveFlags(FCVAR_DEVELOPMENTONLY);
	discord_updatePresence->RemoveFlags(FCVAR_DEVELOPMENTONLY);
#endif // !DEDICATED
	fps_max_vsync->RemoveFlags(FCVAR_DEVELOPMENTONLY);

	base_tickinterval_sp->RemoveFlags(FCVAR_DEVELOPMENTONLY);
	base_tickinterval_mp->RemoveFlags(FCVAR_DEVELOPMENTONLY);

	mp_gamemode->RemoveFlags(FCVAR_DEVELOPMENTONLY);
	mp_gamemode->RemoveChangeCallback(mp_gamemode->m_fnChangeCallbacks[0]);
	mp_gamemode->InstallChangeCallback(MP_GameMode_Changed_f, false);
	net_usesocketsforloopback->RemoveFlags(FCVAR_DEVELOPMENTONLY);
	net_usesocketsforloopback->InstallChangeCallback(NET_UseSocketsForLoopbackChanged_f, false);
#ifndef DEDICATED
	cl_language->InstallChangeCallback(LanguageChanged_f, false);
#endif // !DEDICATED
}

//-----------------------------------------------------------------------------
// Purpose: unregister/disable extraneous ConVar's.
//-----------------------------------------------------------------------------
void ConVar_PurgeShipped(void)
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
void ConVar_PurgeHostNames(void)
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
			pCVar->SetValue(NET_IPV4_UNSPEC);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: ConCommand registration
//-----------------------------------------------------------------------------
void ConCommand_StaticInit(void)
{
	//-------------------------------------------------------------------------
	// ENGINE DLL                                                             |
#if !defined (GAMEDLL_S0) && !defined (GAMEDLL_S1)
	ConCommand::StaticCreate("bhit", "Bullet-hit trajectory debug.", nullptr, FCVAR_DEVELOPMENTONLY | FCVAR_GAMEDLL, BHit_f, nullptr);
#endif // !GAMEDLL_S0 && !GAMEDLL_S1
#ifndef DEDICATED
	ConCommand::StaticCreate("line", "Draw a debug line.", nullptr, FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, Line_f, nullptr);
	ConCommand::StaticCreate("sphere", "Draw a debug sphere.", nullptr, FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, Sphere_f, nullptr);
	ConCommand::StaticCreate("capsule", "Draw a debug capsule.", nullptr, FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, Capsule_f, nullptr);
#endif //!DEDICATED
	ConCommand::StaticCreate("con_help", "Shows the colors and description of each context.", nullptr, FCVAR_RELEASE, CON_Help_f, nullptr);
#ifndef CLIENT_DLL
	ConCommand::StaticCreate("reload_playlists", "Reloads the playlists file.", nullptr, FCVAR_RELEASE, Host_ReloadPlaylists_f, nullptr);
#endif // !CLIENT_DLL
	//-------------------------------------------------------------------------
	// SERVER DLL                                                             |
#ifndef CLIENT_DLL
	ConCommand::StaticCreate("script", "Run input code as SERVER script on the VM.", nullptr, FCVAR_DEVELOPMENTONLY | FCVAR_GAMEDLL | FCVAR_CHEAT, SQVM_ServerScript_f, nullptr);
	ConCommand::StaticCreate("kick", "Kick a client from the server by user name.", "kick \"<userId>\"", FCVAR_RELEASE, Host_Kick_f, nullptr);
	ConCommand::StaticCreate("kickid", "Kick a client from the server by handle, nucleus id or ip address.", "kickid \"<handle>\"/\"<nucleusId>/<ipAddress>\"", FCVAR_RELEASE, Host_KickID_f, nullptr);
	ConCommand::StaticCreate("ban", "Bans a client from the server by user name.", "ban <userId>", FCVAR_RELEASE, Host_Ban_f, nullptr);
	ConCommand::StaticCreate("banid", "Bans a client from the server by handle, nucleus id or ip address.", "banid \"<handle>\"/\"<nucleusId>/<ipAddress>\"", FCVAR_RELEASE, Host_BanID_f, nullptr);
	ConCommand::StaticCreate("unban", "Unbans a client from the server by nucleus id or ip address.", "unban \"<nucleusId>\"/\"<ipAddress>\"", FCVAR_RELEASE, Host_Unban_f, nullptr);
	ConCommand::StaticCreate("sv_reloadbanlist", "Reloads the banned list.", nullptr, FCVAR_RELEASE, Host_ReloadBanList_f, nullptr);
	ConCommand::StaticCreate("sv_addbot", "Creates a bot on the server.", nullptr, FCVAR_RELEASE, CC_CreateFakePlayer_f, nullptr);
	ConCommand::StaticCreate("navmesh_hotswap", "Hot swap the NavMesh for all hulls.", nullptr, FCVAR_DEVELOPMENTONLY, Detour_HotSwap_f, nullptr);
#endif // !CLIENT_DLL
#ifndef DEDICATED
	//-------------------------------------------------------------------------
	// CLIENT DLL                                                             |
	ConCommand::StaticCreate("script_client", "Run input code as CLIENT script on the VM.", nullptr, FCVAR_DEVELOPMENTONLY| FCVAR_CLIENTDLL | FCVAR_CHEAT, SQVM_ClientScript_f, nullptr);
	ConCommand::StaticCreate("rcon", "Forward RCON query to remote server.", "rcon \"<query>\"", FCVAR_CLIENTDLL | FCVAR_RELEASE, RCON_CmdQuery_f, nullptr);
	ConCommand::StaticCreate("rcon_disconnect", "Disconnect from RCON server.", nullptr, FCVAR_CLIENTDLL | FCVAR_RELEASE, RCON_Disconnect_f, nullptr);

	ConCommand::StaticCreate("con_history", "Shows the developer console submission history.", nullptr, FCVAR_CLIENTDLL | FCVAR_RELEASE, CON_LogHistory_f, nullptr);
	ConCommand::StaticCreate("con_removeline", "Removes a range of lines from the developer console.", nullptr, FCVAR_CLIENTDLL | FCVAR_RELEASE, CON_RemoveLine_f, nullptr);
	ConCommand::StaticCreate("con_clearlines", "Clears all lines from the developer console.", nullptr, FCVAR_CLIENTDLL | FCVAR_RELEASE, CON_ClearLines_f, nullptr);
	ConCommand::StaticCreate("con_clearhistory", "Clears all submissions from the developer console history.", nullptr, FCVAR_CLIENTDLL | FCVAR_RELEASE, CON_ClearHistory_f, nullptr);

	ConCommand::StaticCreate("toggleconsole", "Show/hide the developer console.", nullptr, FCVAR_CLIENTDLL | FCVAR_RELEASE, ToggleConsole_f, nullptr);
	ConCommand::StaticCreate("togglebrowser", "Show/hide the server browser.", nullptr, FCVAR_CLIENTDLL | FCVAR_RELEASE, ToggleBrowser_f, nullptr);
	//-------------------------------------------------------------------------
	// UI DLL                                                                 |
	ConCommand::StaticCreate("script_ui", "Run input code as UI script on the VM.", nullptr, FCVAR_DEVELOPMENTONLY | FCVAR_CLIENTDLL | FCVAR_CHEAT, SQVM_UIScript_f, nullptr);
#endif // !DEDICATED
	//-------------------------------------------------------------------------
	// FILESYSTEM API                                                         |
	ConCommand::StaticCreate("fs_vpk_mount", "Mount a VPK file for FileSystem usage.", nullptr, FCVAR_DEVELOPMENTONLY, VPK_Mount_f, nullptr);
	ConCommand::StaticCreate("fs_vpk_unmount", "Unmount a VPK file and clear its cache.", nullptr, FCVAR_DEVELOPMENTONLY, VPK_Unmount_f, nullptr);
	ConCommand::StaticCreate("fs_vpk_pack", "Pack a VPK file from current workspace.", nullptr, FCVAR_DEVELOPMENTONLY, VPK_Pack_f, nullptr);
	ConCommand::StaticCreate("fs_vpk_unpack", "Unpack all files from a VPK file.", nullptr, FCVAR_DEVELOPMENTONLY, VPK_Unpack_f, nullptr);
	//-------------------------------------------------------------------------
	// RTECH API                                                              |
	ConCommand::StaticCreate("rtech_strtoguid", "Calculates the GUID from input data.", nullptr, FCVAR_DEVELOPMENTONLY, RTech_StringToGUID_f, nullptr);
	ConCommand::StaticCreate("pak_decompress", "Decompresses specified RPAK file.", nullptr, FCVAR_DEVELOPMENTONLY, RTech_Decompress_f, RTech_PakDecompress_f_CompletionFunc);
	ConCommand::StaticCreate("pak_requestload", "Requests asynchronous load for specified RPAK file.", nullptr, FCVAR_DEVELOPMENTONLY, Pak_RequestLoad_f, RTech_PakLoad_f_CompletionFunc);
	ConCommand::StaticCreate("pak_requestunload", "Requests unload for specified RPAK file or ID.", nullptr, FCVAR_DEVELOPMENTONLY, Pak_RequestUnload_f, RTech_PakUnload_f_CompletionFunc);
	ConCommand::StaticCreate("pak_swap", "Requests swap for specified RPAK file or ID", nullptr, FCVAR_DEVELOPMENTONLY, Pak_Swap_f, nullptr);
	ConCommand::StaticCreate("pak_listpaks", "Display a list of the loaded Pak files.", nullptr, FCVAR_RELEASE, Pak_ListPaks_f, nullptr);
	ConCommand::StaticCreate("pak_listtypes", "Display a list of the registered asset types.", nullptr, FCVAR_RELEASE, Pak_ListTypes_f, nullptr);
	//-------------------------------------------------------------------------
	// NETCHANNEL                                                             |
	ConCommand::StaticCreate("net_setkey", "Sets user specified base64 net key.", nullptr, FCVAR_RELEASE, NET_SetKey_f, nullptr);
	ConCommand::StaticCreate("net_generatekey", "Generates and sets a random base64 net key.", nullptr, FCVAR_RELEASE, NET_GenerateKey_f, nullptr);
	//-------------------------------------------------------------------------
	// TIER0                                                                  |
	if (!IsCert() && !IsRetail())
		ConCommand::StaticCreate("sig_getadr", "Logs the sigscan results to the console.", nullptr, FCVAR_DEVELOPMENTONLY | FCVAR_HIDDEN, SIG_GetAdr_f, nullptr);
}

//-----------------------------------------------------------------------------
// Purpose: shipped ConCommand initialization
//-----------------------------------------------------------------------------
void ConCommand_InitShipped(void)
{
	///------------------------------------------------------ [ CALLBACK SWAP ]
	//-------------------------------------------------------------------------
	// ENGINE DLL                                                             |
	ConCommand* changelevel = g_pCVar->FindCommand("changelevel");
	ConCommand* map = g_pCVar->FindCommand("map");
	ConCommand* map_background = g_pCVar->FindCommand("map_background");
	ConCommand* ss_map = g_pCVar->FindCommand("ss_map");
	ConCommand* migrateme = g_pCVar->FindCommand("migrateme");
	ConCommand* help = g_pCVar->FindCommand("help");
	ConCommand* convar_list = g_pCVar->FindCommand("convar_list");
	ConCommand* convar_differences = g_pCVar->FindCommand("convar_differences");
	ConCommand* convar_findByFlags = g_pCVar->FindCommand("convar_findByFlags");
#ifndef DEDICATED
	//-------------------------------------------------------------------------
	// MATERIAL SYSTEM
	ConCommand* mat_crosshair = g_pCVar->FindCommand("mat_crosshair"); // Patch callback function to working callback.
	//-------------------------------------------------------------------------
	// CLIENT DLL                                                             |
	ConCommand* give = g_pCVar->FindCommand("give");
#endif // !DEDICATED

	help->m_fnCommandCallback = CVHelp_f;
	convar_list->m_fnCommandCallback = CVList_f;
	convar_differences->m_fnCommandCallback = CVDiff_f;
	convar_findByFlags->m_fnCommandCallback = CVFlag_f;
#ifndef CLIENT_DLL
	changelevel->m_fnCommandCallback = Host_Changelevel_f;
#endif // !CLIENT_DLL
	changelevel->m_fnCompletionCallback = Host_Changelevel_f_CompletionFunc;

	map->m_fnCompletionCallback = Host_Map_f_CompletionFunc;
	map_background->m_fnCompletionCallback = Host_Background_f_CompletionFunc;
	ss_map->m_fnCompletionCallback = Host_SSMap_f_CompletionFunc;

#ifndef DEDICATED
	mat_crosshair->m_fnCommandCallback = Mat_CrossHair_f;
	give->m_fnCompletionCallback = Game_Give_f_CompletionFunc;
#endif // !DEDICATED

	/// ------------------------------------------------------ [ FLAG REMOVAL ]
	//-------------------------------------------------------------------------
	if (!CommandLine()->CheckParm("-devsdk"))
	{
		const char* pszMaskedBases[] =
		{
#ifndef DEDICATED
			"connect",
			"connectAsSpectator",
			"connectWithKey",
			"silentconnect",
			"set",
			"ping",
#endif // !DEDICATED
			"launchplaylist",
			"quit",
			"exit",
			"reload",
			"restart",
			"status",
			"version",
		};

		for (size_t i = 0; i < SDK_ARRAYSIZE(pszMaskedBases); i++)
		{
			if (ConCommandBase* pCommandBase = g_pCVar->FindCommandBase(pszMaskedBases[i]))
			{
				pCommandBase->RemoveFlags(FCVAR_DEVELOPMENTONLY);
			}
		}

		convar_list->RemoveFlags(FCVAR_DEVELOPMENTONLY);
		convar_differences->RemoveFlags(FCVAR_DEVELOPMENTONLY);
		convar_findByFlags->RemoveFlags(FCVAR_DEVELOPMENTONLY);
		help->RemoveFlags(FCVAR_DEVELOPMENTONLY);
		migrateme->RemoveFlags(FCVAR_SERVER_CAN_EXECUTE);
		changelevel->RemoveFlags(FCVAR_DEVELOPMENTONLY);
		map->RemoveFlags(FCVAR_DEVELOPMENTONLY | FCVAR_SERVER_CAN_EXECUTE);
		map_background->RemoveFlags(FCVAR_DEVELOPMENTONLY | FCVAR_SERVER_CAN_EXECUTE);
		ss_map->RemoveFlags(FCVAR_DEVELOPMENTONLY | FCVAR_SERVER_CAN_EXECUTE);
	}
}

//-----------------------------------------------------------------------------
// Purpose: unregister extraneous ConCommand's.
//-----------------------------------------------------------------------------
void ConCommand_PurgeShipped(void)
{
#ifdef DEDICATED
	const char* pszCommandToRemove[] =
	{
		"bind",
		"bind_held",
		"bind_list",
		"bind_list_abilities",
		"bind_US_standard",
		"bind_held_US_standard",
		"unbind",
		"unbind_US_standard",
		"unbindall",
		"unbind_all_gamepad",
		"unbindall_ignoreGamepad",
		"unbind_batch",
		"unbind_held",
		"unbind_held_US_standard",
		"uiscript_reset",
		"getpos_bind",
		"connect",
		"silent_connect",
		"ping",
		"gameui_activate",
		"gameui_hide",
		"weaponSelectOrdnance",
		"weaponSelectPrimary0",
		"weaponSelectPrimary1",
		"weaponSelectPrimary2",
		"+scriptCommand1",
		"-scriptCommand1",
		"+scriptCommand2",
		"-scriptCommand2",
		"+scriptCommand3",
		"-scriptCommand3",
		"+scriptCommand4",
		"-scriptCommand4",
		"+scriptCommand5",
		"-scriptCommand5",
		"+scriptCommand6",
		"-scriptCommand6",
		"+scriptCommand7",
		"-scriptCommand7",
		"+scriptCommand8",
		"-scriptCommand8",
		"+scriptCommand9",
		"-scriptCommand9",
	};

	for (size_t i = 0; i < SDK_ARRAYSIZE(pszCommandToRemove); i++)
	{
		ConCommandBase* pCommandBase = g_pCVar->FindCommandBase(pszCommandToRemove[i]);

		if (pCommandBase)
		{
			g_pCVar->UnregisterConCommand(pCommandBase);
		}
	}
#endif // DEDICATED
}