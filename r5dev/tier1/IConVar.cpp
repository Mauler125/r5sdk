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
#include "mathlib/bits.h"
#include "vstdlib/callback.h"
#include "public/include/iconvar.h"
#include "public/include/iconcommand.h"
#include <tier1/utlvector.h>

//-----------------------------------------------------------------------------
// Purpose: construct/allocate
//-----------------------------------------------------------------------------
ConVar::ConVar(const char* pszName, const char* pszDefaultValue, int nFlags, const char* pszHelpString, bool bMin, float fMin, bool bMax, float fMax, FnChangeCallback_t pCallback, const char* pszUsageString)
{
	ConVar* pNewConVar = MemAllocSingleton()->Alloc<ConVar>(sizeof(ConVar)); // Allocate new memory with StdMemAlloc else we crash.
	memset(pNewConVar, '\0', sizeof(ConVar));                                // Set all to null.

	pNewConVar->m_pConCommandBaseVFTable = g_pConVarVFTable.RCast<IConCommandBase*>();
	pNewConVar->m_pIConVarVFTable = g_pIConVarVFTable.RCast<IConVar*>();

	ConVar_Register(pNewConVar, pszName, pszDefaultValue, nFlags, pszHelpString, bMin, fMin, bMax, fMax, pCallback, pszUsageString);
	*this = *pNewConVar;
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
void ConVar::Init(void) const
{
	//-------------------------------------------------------------------------
	// ENGINE                                                                 |
	hostdesc = new ConVar("hostdesc", "", FCVAR_RELEASE, "Host game server description.", false, 0.f, false, 0.f, nullptr, nullptr);
	staticProp_defaultBuildFrustum = new ConVar("staticProp_defaultBuildFrustum", "0", FCVAR_DEVELOPMENTONLY, "Use the old solution for building static prop frustum culling.", false, 0.f, false, 0.f, nullptr, nullptr);

	cm_debug_cmdquery       = new ConVar("cm_debug_cmdquery"      , "0", FCVAR_DEVELOPMENTONLY, "Prints the flags of each ConVar/ConCommand query to the console ( !slower! ).", false, 0.f, false, 0.f, nullptr, nullptr);
	cm_unset_all_cmdquery   = new ConVar("cm_unset_all_cmdquery"  , "0", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Returns false on every ConVar/ConCommand query ( !warning! ).", false, 0.f, false, 0.f, nullptr, nullptr);
	cm_unset_dev_cmdquery   = new ConVar("cm_unset_dev_cmdquery"  , "1", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Returns false on all FCVAR_DEVELOPMENTONLY ConVar/ConCommand queries ( !warning! ).", false, 0.f, false, 0.f, nullptr, nullptr);
	cm_unset_cheat_cmdquery = new ConVar("cm_unset_cheat_cmdquery", "0", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Returns false on all FCVAR_DEVELOPMENTONLY and FCVAR_CHEAT ConVar/ConCommand queries ( !warning! ).", false, 0.f, false, 0.f, nullptr, nullptr);

	rcon_address  = new ConVar("rcon_address",  "::", FCVAR_SERVER_CANNOT_QUERY | FCVAR_DONTRECORD | FCVAR_RELEASE, "Remote server access address.", false, 0.f, false, 0.f, nullptr, nullptr);
	rcon_password = new ConVar("rcon_password", ""  , FCVAR_SERVER_CANNOT_QUERY | FCVAR_DONTRECORD | FCVAR_RELEASE, "Remote server access password (rcon is disabled if empty).", false, 0.f, false, 0.f, &RCON_PasswordChanged_f, nullptr);

	r_debug_overlay_nodecay        = new ConVar("r_debug_overlay_nodecay"       , "0", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "Keeps all debug overlays alive regardless of their lifetime. Use command 'clear_debug_overlays' to clear everything.", false, 0.f, false, 0.f, nullptr, nullptr);
	r_debug_overlay_invisible      = new ConVar("r_debug_overlay_invisible"     , "1", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "Show invisible debug overlays (alpha < 1 = 255).", false, 0.f, false, 0.f, nullptr, nullptr);
	r_debug_overlay_wireframe      = new ConVar("r_debug_overlay_wireframe"     , "1", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "Use wireframe in debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	r_debug_overlay_zbuffer        = new ConVar("r_debug_overlay_zbuffer"       , "1", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "Use z-buffer for debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	r_drawWorldMeshes              = new ConVar("r_drawWorldMeshes"             , "1", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "Render world meshes.", false, 0.f, false, 0.f, nullptr, nullptr);
	r_drawWorldMeshesDepthOnly     = new ConVar("r_drawWorldMeshesDepthOnly"    , "1", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "Render world meshes (depth only).", false, 0.f, false, 0.f, nullptr, nullptr);
	r_drawWorldMeshesDepthAtTheEnd = new ConVar("r_drawWorldMeshesDepthAtTheEnd", "1", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT, "Render world meshes (depth at the end).", false, 0.f, false, 0.f, nullptr, nullptr);
	//-------------------------------------------------------------------------
	// SERVER                                                                 |
#ifndef CLIENT_DLL
	ai_ainDumpOnLoad           = new ConVar("ai_ainDumpOnLoad"          , "0", FCVAR_DEVELOPMENTONLY, "Dumps AIN data from node graphs loaded from the disk on load.", false, 0.f, false, 0.f, nullptr, nullptr);
	ai_ainDebugConnect         = new ConVar("ai_ainDebugConnect"        , "0", FCVAR_DEVELOPMENTONLY, "Debug AIN node connections.", false, 0.f, false, 0.f, nullptr, nullptr);
	ai_script_nodes_draw_range = new ConVar("ai_script_nodes_draw_range", "0", FCVAR_DEVELOPMENTONLY, "AIN debug draw script nodes ranging from shift index to this cvar.", false, 0.f, false, 0.f, nullptr, nullptr);

	navmesh_always_reachable   = new ConVar("navmesh_always_reachable"   , "0"    , FCVAR_DEVELOPMENTONLY, "Marks goal poly from agent poly as reachable regardless of table data ( !slower! ).", false, 0.f, false, 0.f, nullptr, nullptr);
	navmesh_debug_type         = new ConVar("navmesh_debug_type"         , "0"    , FCVAR_DEVELOPMENTONLY, "NavMesh debug draw hull index.", true, 0.f, true, 4.f, nullptr, "0 = small, 1 = med_short, 2 = medium, 3 = large, 4 = extra large");
	navmesh_debug_tile_range   = new ConVar("navmesh_debug_tile_range"   , "0"    , FCVAR_DEVELOPMENTONLY, "NavMesh debug draw tiles ranging from shift index to this cvar.", true, 0.f, false, 0.f, nullptr, nullptr);
	navmesh_debug_camera_range = new ConVar("navmesh_debug_camera_range" , "2000" , FCVAR_DEVELOPMENTONLY, "Only debug draw tiles within this distance from camera origin.", true, 0.f, false, 0.f, nullptr, nullptr);
#ifndef DEDICATED
	navmesh_draw_bvtree            = new ConVar("navmesh_draw_bvtree"            , "-1", FCVAR_DEVELOPMENTONLY, "Draws the BVTree of the NavMesh tiles.", false, 0.f, false, 0.f, nullptr, "Index: > 0 && < mesh->m_tileCount");
	navmesh_draw_portal            = new ConVar("navmesh_draw_portal"            , "-1", FCVAR_DEVELOPMENTONLY, "Draws the portal of the NavMesh tiles.", false, 0.f, false, 0.f, nullptr, "Index: > 0 && < mesh->m_tileCount");
	navmesh_draw_polys             = new ConVar("navmesh_draw_polys"             , "-1", FCVAR_DEVELOPMENTONLY, "Draws the polys of the NavMesh tiles.", false, 0.f, false, 0.f, nullptr, "Index: > 0 && < mesh->m_tileCount");
	navmesh_draw_poly_bounds       = new ConVar("navmesh_draw_poly_bounds"       , "-1", FCVAR_DEVELOPMENTONLY, "Draws the bounds of the NavMesh polys.", false, 0.f, false, 0.f, nullptr, "Index: > 0 && < mesh->m_tileCount");
	navmesh_draw_poly_bounds_inner = new ConVar("navmesh_draw_poly_bounds_inner" , "0" , FCVAR_DEVELOPMENTONLY, "Draws the inner bounds of the NavMesh polys (requires navmesh_draw_poly_bounds).", false, 0.f, false, 0.f, nullptr, "Index: > 0 && < mesh->m_tileCount");
#endif // !DEDICATED
	sv_showconnecting  = new ConVar("sv_showconnecting" , "1", FCVAR_RELEASE, "Logs information about the connecting client to the console.", false, 0.f, false, 0.f, nullptr, nullptr);
	sv_pylonVisibility = new ConVar("sv_pylonVisibility", "0", FCVAR_RELEASE, "Determines the visiblity to the Pylon master server, 0 = Offline, 1 = Hidden, 2 = Public.", false, 0.f, false, 0.f, nullptr, nullptr);
	sv_pylonRefreshInterval   = new ConVar("sv_pylonRefreshInterval"  , "5.0", FCVAR_RELEASE, "Pylon server host request post update interval (seconds).", true, 2.f, true, 8.f, nullptr, nullptr);
	sv_banlistRefreshInterval = new ConVar("sv_banlistRefreshInterval", "1.0", FCVAR_RELEASE, "Banlist refresh interval (seconds).", true, 1.f, false, 0.f, nullptr, nullptr);
	sv_statusRefreshInterval  = new ConVar("sv_statusRefreshInterval" , "0.5", FCVAR_RELEASE, "Server status bar update interval (seconds).", false, 0.f, false, 0.f, nullptr, nullptr);
#endif // !CLIENT_DLL
#ifdef DEDICATED
	sv_rcon_debug       = new ConVar("sv_rcon_debug"      , "0" , FCVAR_RELEASE, "Show rcon debug information ( !slower! ).", false, 0.f, false, 0.f, nullptr, nullptr);
	sv_rcon_banpenalty  = new ConVar("sv_rcon_banpenalty" , "10", FCVAR_RELEASE, "Number of minutes to ban users who fail rcon authentication.", false, 0.f, false, 0.f, nullptr, nullptr);
	sv_rcon_maxfailures = new ConVar("sv_rcon_maxfailures", "10", FCVAR_RELEASE, "Max number of times a user can fail rcon authentication before being banned.", false, 0.f, false, 0.f, nullptr, nullptr);
	sv_rcon_maxignores  = new ConVar("sv_rcon_maxignores" , "15", FCVAR_RELEASE, "Max number of times a user can ignore the no-auth message before being banned.", false, 0.f, false, 0.f, nullptr, nullptr);
	sv_rcon_maxsockets  = new ConVar("sv_rcon_maxsockets" , "32", FCVAR_RELEASE, "Max number of accepted sockets before the server starts closing redundant sockets.", false, 0.f, false, 0.f, nullptr, nullptr);
	sv_rcon_whitelist_address = new ConVar("sv_rcon_whitelist_address", "", FCVAR_RELEASE, "This address is not considered a 'redundant' socket and will never be banned for failed authentications.", false, 0.f, false, 0.f, nullptr, "Format: '::ffff:127.0.0.1'.");
#endif // DEDICATED
	bhit_abs_origin = new ConVar("bhit_abs_origin", "0", FCVAR_RELEASE, "Use player's absolute origin for bhit tracing.", false, 0.f, false, 0.f, nullptr, nullptr);
	//-------------------------------------------------------------------------
	// CLIENT                                                                 |
#ifndef DEDICATED
	cl_drawconsoleoverlay           = new ConVar("cl_drawconsoleoverlay"          , "0" , FCVAR_DEVELOPMENTONLY, "Draws the RUI console overlay at the top of the screen.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_consoleoverlay_lines         = new ConVar("cl_consoleoverlay_lines"        , "3" , FCVAR_DEVELOPMENTONLY, "Number of lines of console output to draw.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_consoleoverlay_invert_rect_x = new ConVar("cl_consoleoverlay_invert_rect_x", "0" , FCVAR_DEVELOPMENTONLY, "Inverts the X rect for RUI console overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_consoleoverlay_invert_rect_y = new ConVar("cl_consoleoverlay_invert_rect_y", "0" , FCVAR_DEVELOPMENTONLY, "Inverts the Y rect for RUI console overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_consoleoverlay_offset_x      = new ConVar("cl_consoleoverlay_offset_x"     , "10", FCVAR_DEVELOPMENTONLY, "X offset for RUI console overlay.", false, 1.f, false, 50.f, nullptr, nullptr);
	cl_consoleoverlay_offset_y      = new ConVar("cl_consoleoverlay_offset_y"     , "10", FCVAR_DEVELOPMENTONLY, "Y offset for RUI console overlay.", false, 1.f, false, 50.f, nullptr, nullptr);

	cl_conoverlay_script_server_clr  = new ConVar("cl_conoverlay_script_server_clr", "130 120 245 255", FCVAR_DEVELOPMENTONLY, "Script SERVER VM RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	cl_conoverlay_script_client_clr  = new ConVar("cl_conoverlay_script_client_clr", "117 116 139 255", FCVAR_DEVELOPMENTONLY, "Script CLIENT VM RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	cl_conoverlay_script_ui_clr      = new ConVar("cl_conoverlay_script_ui_clr"    , "200 110 110 255", FCVAR_DEVELOPMENTONLY, "Script UI VM RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);

	cl_conoverlay_native_server_clr = new ConVar("cl_conoverlay_native_server_clr", "20 50 248 255", FCVAR_DEVELOPMENTONLY, "Native SERVER RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	cl_conoverlay_native_client_clr = new ConVar("cl_conoverlay_native_client_clr", "70 70 70 255", FCVAR_DEVELOPMENTONLY, "Native CLIENT RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	cl_conoverlay_native_ui_clr     = new ConVar("cl_conoverlay_native_ui_clr"    , "200 60 60 255", FCVAR_DEVELOPMENTONLY, "Native UI RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	cl_conoverlay_native_engine_clr = new ConVar("cl_conoverlay_native_engine_clr", "255 255 255 255", FCVAR_DEVELOPMENTONLY, "Native engine RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	cl_conoverlay_native_fs_clr     = new ConVar("cl_conoverlay_native_fs_clr"    , "0 100 225 255", FCVAR_DEVELOPMENTONLY, "Native filesystem RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	cl_conoverlay_native_rtech_clr  = new ConVar("cl_conoverlay_native_rtech_clr" , "25 100 100 255", FCVAR_DEVELOPMENTONLY, "Native rtech RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	cl_conoverlay_native_ms_clr     = new ConVar("cl_conoverlay_native_ms_clr"    , "200 20 180 255", FCVAR_DEVELOPMENTONLY, "Native materialsystem RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);

	cl_conoverlay_netcon_clr  = new ConVar("cl_conoverlay_netcon_clr" , "255 255 255 255", FCVAR_DEVELOPMENTONLY, "Net console RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	cl_conoverlay_common_clr  = new ConVar("cl_conoverlay_common_clr" , "255 140 80 255" , FCVAR_DEVELOPMENTONLY, "Common RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);

	cl_conoverlay_warning_clr = new ConVar("cl_conoverlay_warning_clr", "180 180 20 255", FCVAR_DEVELOPMENTONLY, "Warning RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);
	cl_conoverlay_error_clr   = new ConVar("cl_conoverlay_error_clr"  , "225 20 20 255" , FCVAR_DEVELOPMENTONLY, "Error RUI console overlay log color.", false, 1.f, false, 50.f, nullptr, nullptr);

	cl_showhoststats           = new ConVar("cl_showhoststats"          , "0", FCVAR_DEVELOPMENTONLY, "Host speeds debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_hoststats_invert_rect_x = new ConVar("cl_hoststats_invert_rect_x", "0", FCVAR_DEVELOPMENTONLY, "Inverts the X rect for host speeds debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_hoststats_invert_rect_y = new ConVar("cl_hoststats_invert_rect_y", "0", FCVAR_DEVELOPMENTONLY, "Inverts the Y rect for host speeds debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_hoststats_offset_x      = new ConVar("cl_hoststats_offset_x"    , "10", FCVAR_DEVELOPMENTONLY, "X offset for host speeds debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_hoststats_offset_y      = new ConVar("cl_hoststats_offset_y"    , "10", FCVAR_DEVELOPMENTONLY, "Y offset for host speeds debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);

	cl_showsimstats           = new ConVar("cl_showsimstats"          , "0"  , FCVAR_DEVELOPMENTONLY, "Shows the tick counter for the server/client simulation and the render frame.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_simstats_invert_rect_x = new ConVar("cl_simstats_invert_rect_x", "1"  , FCVAR_DEVELOPMENTONLY, "Inverts the X rect for simulation debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_simstats_invert_rect_y = new ConVar("cl_simstats_invert_rect_y", "1"  , FCVAR_DEVELOPMENTONLY, "Inverts the Y rect for simulation debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_simstats_offset_x      = new ConVar("cl_simstats_offset_x"     , "650", FCVAR_DEVELOPMENTONLY, "X offset for simulation debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_simstats_offset_y      = new ConVar("cl_simstats_offset_y"     , "120", FCVAR_DEVELOPMENTONLY, "Y offset for simulation debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);

	cl_showgpustats           = new ConVar("cl_showgpustats"            , "0", FCVAR_DEVELOPMENTONLY, "Texture streaming debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_gpustats_invert_rect_x = new ConVar("cl_gpustats_invert_rect_x"  , "1", FCVAR_DEVELOPMENTONLY, "Inverts the X rect for texture streaming debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_gpustats_invert_rect_y = new ConVar("cl_gpustats_invert_rect_y"  , "1", FCVAR_DEVELOPMENTONLY, "Inverts the Y rect for texture streaming debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_gpustats_offset_x      = new ConVar("cl_gpustats_offset_x"     , "650", FCVAR_DEVELOPMENTONLY, "X offset for texture streaming debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_gpustats_offset_y      = new ConVar("cl_gpustats_offset_y"     , "105", FCVAR_DEVELOPMENTONLY, "Y offset for texture streaming debug overlay.", false, 0.f, false, 0.f, nullptr, nullptr);

	cl_showmaterialinfo      = new ConVar("cl_showmaterialinfo"     , "0"  , FCVAR_DEVELOPMENTONLY, "Draw info for the material under the crosshair on screen.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_materialinfo_offset_x = new ConVar("cl_materialinfo_offset_x", "0"  , FCVAR_DEVELOPMENTONLY, "X offset for material debug info overlay.", false, 0.f, false, 0.f, nullptr, nullptr);
	cl_materialinfo_offset_y = new ConVar("cl_materialinfo_offset_y", "420", FCVAR_DEVELOPMENTONLY, "Y offset for material debug info overlay.", false, 0.f, false, 0.f, nullptr, nullptr);

	con_max_size_logvector        = new ConVar("con_max_size_logvector"        , "1000", FCVAR_DEVELOPMENTONLY, "Maximum number of logs in the console until cleanup starts.", false, 0.f, false, 0.f, nullptr, nullptr);
	con_suggestion_limit          = new ConVar("con_suggestion_limit"          , "120" , FCVAR_DEVELOPMENTONLY, "Maximum number of suggestions the autocomplete window will show for the console.", false, 0.f, false, 0.f, nullptr, nullptr);
	con_suggestion_showhelptext   = new ConVar("con_suggestion_showhelptext"   , "1"   , FCVAR_DEVELOPMENTONLY, "Show CommandBase help text in autocomplete window.", false, 0.f, false, 0.f, nullptr, nullptr);
	con_suggestion_showflags      = new ConVar("con_suggestion_showflags"      , "1"   , FCVAR_DEVELOPMENTONLY, "Show CommandBase flags in autocomplete window.", false, 0.f, false, 0.f, nullptr, nullptr);
	con_suggestion_flags_realtime = new ConVar("con_suggestion_flags_realtime" , "0"   , FCVAR_DEVELOPMENTONLY, "Whether to show compile-time or run-time CommandBase flags.", false, 0.f, false, 0.f, nullptr, nullptr);
#endif // !DEDICATED
	//-------------------------------------------------------------------------
	// FILESYSTEM                                                             |
	fs_warning_level_sdk            = new ConVar("fs_warning_level_sdk"                , "0", FCVAR_DEVELOPMENTONLY, "Set the SDK filesystem warning level.", false, 0.f, false, 0.f, nullptr, nullptr);
	fs_show_warning_output          = new ConVar("fs_show_warning_output"              , "0", FCVAR_DEVELOPMENTONLY, "Logs the filesystem warnings to the console, filtered by 'fs_warning_level_native' ( !slower! ).", false, 0.f, false, 0.f, nullptr, nullptr);
	fs_packedstore_entryblock_stats = new ConVar("fs_packedstore_entryblock_stats"     , "0", FCVAR_DEVELOPMENTONLY, "If set to 1, prints the stats of each file entry in the VPK during decompression ( !slower! ).", false, 0.f, false, 0.f, nullptr, nullptr);
	fs_packedstore_workspace        = new ConVar("fs_packedstore_workspace", "platform/vpk/", FCVAR_DEVELOPMENTONLY, "Determines the current VPK workspace.", false, 0.f, false, 0.f, nullptr, nullptr);
	//-------------------------------------------------------------------------
	// MATERIALSYSTEM                                                         |
#ifndef DEDICATED
	mat_showdxoutput = new ConVar("mat_showdxoutput", "0", FCVAR_DEVELOPMENTONLY, "Shows debug output for the DirectX system.", false, 0.f, false, 0.f, nullptr, nullptr);
#endif // !DEDICATED
	//-------------------------------------------------------------------------
	// SQUIRREL                                                               |
	sq_showrsonloading   = new ConVar("sq_showrsonloading"  , "0", FCVAR_DEVELOPMENTONLY, "Logs all 'rson' files loaded by the SQVM ( !slower! ).", false, 0.f, false, 0.f, nullptr, nullptr);
	sq_showscriptloading = new ConVar("sq_showscriptloading", "0", FCVAR_DEVELOPMENTONLY, "Logs all scripts loaded by the SQVM to be pre-compiled ( !slower! ).", false, 0.f, false, 0.f, nullptr, nullptr);
	sq_showvmoutput      = new ConVar("sq_showvmoutput"     , "0", FCVAR_DEVELOPMENTONLY, "Prints the VM output to the console. 1 = Log to file. 2 = 1 + log to console. 3 = 1 + 2 + log to overhead console. 4 = only log to overhead console.", false, 0.f, false, 0.f, nullptr, nullptr);
	sq_showvmwarning     = new ConVar("sq_showvmwarning"    , "0", FCVAR_DEVELOPMENTONLY, "Prints the VM warning output to the console. 1 = Log to file. 2 = 1 + log to console.", false, 0.f, false, 0.f, nullptr, nullptr);
	//-------------------------------------------------------------------------
	// NETCHANNEL                                                             |
	net_tracePayload           = new ConVar("net_tracePayload"          , "0", FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT          , "Log the payload of the send/recv datagram to a file on the disk.", false, 0.f, false, 0.f, nullptr, nullptr);
	net_encryptionEnable       = new ConVar("net_encryptionEnable"      , "1", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED     , "Use AES encryption on game packets.", false, 0.f, false, 0.f, nullptr, nullptr);
	net_useRandomKey           = new ConVar("net_useRandomKey"          , "1"                        , FCVAR_RELEASE        , "Use random base64 netkey for game packets.", false, 0.f, false, 0.f, nullptr, nullptr);
	//-------------------------------------------------------------------------
	// NETWORKSYSTEM                                                          |
	pylon_matchmaking_hostname = new ConVar("pylon_matchmaking_hostname", "r5a-comp-sv.herokuapp.com", FCVAR_RELEASE        , "Holds the pylon matchmaking hostname.", false, 0.f, false, 0.f, nullptr, nullptr);
	pylon_showdebug            = new ConVar("pylon_showdebug"           , "0"                        , FCVAR_DEVELOPMENTONLY, "Shows debug output for pylon.", false, 0.f, false, 0.f, nullptr, nullptr);
	//-------------------------------------------------------------------------
	// RTECH API                                                              |
	//-------------------------------------------------------------------------
	// RUI                                                                    |
#ifndef DEDICATED
	rui_drawEnable = new ConVar("rui_drawEnable", "1", FCVAR_RELEASE, "Draws the RUI, 1 = Draw, 0 = No Draw.", false, 0.f, false, 0.f, nullptr, nullptr);
#endif // !DEDICATED
	//-------------------------------------------------------------------------
}

//-----------------------------------------------------------------------------
// Purpose: initialize shipped ConVar's
//-----------------------------------------------------------------------------
void ConVar::InitShipped(void) const
{
#ifndef CLIENT_DLL
	ai_script_nodes_draw             = g_pCVar->FindVar("ai_script_nodes_draw");
	bhit_enable                      = g_pCVar->FindVar("bhit_enable");
#endif // !CLIENT_DLL
	single_frame_shutdown_for_reload = g_pCVar->FindVar("single_frame_shutdown_for_reload");
	enable_debug_overlays            = g_pCVar->FindVar("enable_debug_overlays");
	model_defaultFadeDistScale       = g_pCVar->FindVar("model_defaultFadeDistScale");
	model_defaultFadeDistMin         = g_pCVar->FindVar("model_defaultFadeDistMin");
	staticProp_no_fade_scalar        = g_pCVar->FindVar("staticProp_no_fade_scalar");
	staticProp_gather_size_weight    = g_pCVar->FindVar("staticProp_gather_size_weight");
	stream_overlay                   = g_pCVar->FindVar("stream_overlay");
	stream_overlay_mode              = g_pCVar->FindVar("stream_overlay_mode");
	sv_visualizetraces               = g_pCVar->FindVar("sv_visualizetraces");
	old_gather_props                 = g_pCVar->FindVar("old_gather_props");
	mp_gamemode                      = g_pCVar->FindVar("mp_gamemode");
	hostname                         = g_pCVar->FindVar("hostname");
	hostip                           = g_pCVar->FindVar("hostip");
	hostport                         = g_pCVar->FindVar("hostport");
	host_hasIrreversibleShutdown     = g_pCVar->FindVar("host_hasIrreversibleShutdown");
	net_usesocketsforloopback        = g_pCVar->FindVar("net_usesocketsforloopback");

#ifndef CLIENT_DLL
	ai_script_nodes_draw->SetValue(-1);
#endif // !CLIENT_DLL
	mp_gamemode->SetCallback(&MP_GameMode_Changed_f);
}

//-----------------------------------------------------------------------------
// Purpose: unregister/disable extraneous ConVar's.
//-----------------------------------------------------------------------------
void ConVar::PurgeShipped(void) const
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
void ConVar::PurgeHostNames(void) const
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
	if (IsFlagSet(this, FCVAR_MATERIAL_THREAD_MASK))
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

	if (!SetColorFromString(pszValue))
	{
		// Not a color, do the standard thing
		double dblValue = atof(pszValue); // Use double to avoid 24-bit restriction on integers and allow storing timestamps or dates in convars
		float flNewValue = static_cast<float>(dblValue);

		if (!IsFinite(flNewValue))
		{
			Warning(eDLL_T::ENGINE, "Warning: ConVar '%s' = '%s' is infinite, clamping value.\n", GetBaseName(), pszValue);
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

	if (IsFlagSet(this, FCVAR_MATERIAL_THREAD_MASK))
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

	if (IsFlagSet(this, FCVAR_MATERIAL_THREAD_MASK))
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
// Purpose: sets the ConVar callback.
// Input  : *pCallback -
//-----------------------------------------------------------------------------
void ConVar::SetCallback(void* pCallback)
{
	*m_Callback.m_ppCallback = *&pCallback;
}

//-----------------------------------------------------------------------------
// Purpose: sets the ConVar color value from string.
// Input  : *pszValue - 
//-----------------------------------------------------------------------------
bool ConVar::SetColorFromString(const char* pszValue)
{
	bool bColor = false;

	// Try pulling RGBA color values out of the string.
	int nRGBA[4]{};
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
	assert(!(m_nFlags & FCVAR_NEVER_AS_STRING));

	char* pszOldValue = reinterpret_cast<char*>(_malloca(m_Value.m_iStringLength));
	if (pszOldValue != nullptr)
	{
		memcpy(pszOldValue, m_Value.m_pszString, m_Value.m_iStringLength);
	}

	if (pszTempVal)
	{
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
		else if (!m_Value.m_pszString)
		{
			m_Value.m_pszString = MemAllocSingleton()->Alloc<char>(len);
			m_Value.m_iStringLength = len;
		}
		memmove(m_Value.m_pszString, pszTempVal, len);

		/*****
		!FIXME:
			Respawn put additional code here which
			seems to itterate over a 64bit integer
			to call the callback several times (as many times as m_iCallbackCount?).
		******/
	}
	else
	{
		m_Value.m_pszString = nullptr;
	}

	pszOldValue = nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: changes the ConVar string value (this is faster than ChangeStringValue, 
//          only use if the new string is equal or lower than this->m_iStringLength).
// Input  : *pszTempVal - flOldValue
//-----------------------------------------------------------------------------
void ConVar::ChangeStringValueUnsafe(const char* pszNewValue)
{
	m_Value.m_pszString = const_cast<char*>(pszNewValue);
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
bool ConVar::IsFlagSet(ConVar* pConVar, int nFlags)
{
	if (cm_debug_cmdquery->GetBool())
	{
		printf("--------------------------------------------------\n");
		printf(" Flaged: %08X\n", pConVar->m_nFlags);
	}
	if (cm_unset_cheat_cmdquery->GetBool())
	{
		// Mask off FCVAR_CHEATS and FCVAR_DEVELOPMENTONLY.
		pConVar->RemoveFlags(FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT);
	}
	else if(cm_unset_dev_cmdquery->GetBool())// Mask off FCVAR_DEVELOPMENTONLY.
	{
		pConVar->RemoveFlags(FCVAR_DEVELOPMENTONLY);
	}
	if (cm_debug_cmdquery->GetBool())
	{
		printf(" Masked: %08X\n", pConVar->m_nFlags);
		printf(" Verify: %08X\n", nFlags);
		printf("--------------------------------------------------\n");
	}
	if (nFlags & FCVAR_RELEASE && !cm_unset_all_cmdquery->GetBool())
	{
		// Default retail behaviour.
		return IConVar_IsFlagSet(pConVar, nFlags);
	}
	if (cm_unset_all_cmdquery->GetBool())
	{
		// Returning false on all queries may cause problems.
		return false;
	}
	// Default behavior.
	return pConVar->HasFlags(nFlags) != 0;
}

///////////////////////////////////////////////////////////////////////////////
void IConVar_Attach()
{
	DetourAttach((LPVOID*)&IConVar_IsFlagSet, &ConVar::IsFlagSet);
}

void IConVar_Detach()
{
	DetourDetach((LPVOID*)&IConVar_IsFlagSet, &ConVar::IsFlagSet);
}

///////////////////////////////////////////////////////////////////////////////
ConVar* g_pConVar = new ConVar();
