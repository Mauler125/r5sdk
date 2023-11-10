#ifndef GLOBAL_H
#define GLOBAL_H

//-------------------------------------------------------------------------
// ENGINE                                                                 |
extern ConVar* sdk_fixedframe_tickinterval;
extern ConVar* single_frame_shutdown_for_reload;
extern ConVar* old_gather_props;

extern ConVar* enable_debug_overlays;
extern ConVar* debug_draw_box_depth_test;

extern ConVar* developer;
extern ConVar* fps_max;
extern ConVar* fps_max_vsync;

#ifndef DEDICATED
extern ConVar* fps_max_rt;
extern ConVar* fps_max_rt_tolerance;
extern ConVar* fps_max_rt_sleep_threshold;
extern ConVar* fps_max_gfx;
#endif // !DEDICATED

extern ConVar* base_tickinterval_sp;
extern ConVar* base_tickinterval_mp;

// taken from S15:
extern ConVar* usercmd_frametime_max;
extern ConVar* usercmd_frametime_min;

extern ConVar* usercmd_dualwield_enable;

extern ConVar* staticProp_no_fade_scalar;
extern ConVar* staticProp_gather_size_weight;

extern ConVar* model_defaultFadeDistScale;
extern ConVar* model_defaultFadeDistMin;

extern ConVar* ip_cvar;
extern ConVar* hostname;
extern ConVar* hostdesc;
extern ConVar* hostip;
extern ConVar* hostport;

extern ConVar* host_hasIrreversibleShutdown;
extern ConVar* host_timescale;

extern ConVar* mp_gamemode;

extern ConVar* rcon_address;
extern ConVar* rcon_password;

extern ConVar* enable_CmdKeyValues;

extern ConVar* r_debug_overlay_nodecay;
extern ConVar* r_debug_overlay_invisible;
extern ConVar* r_debug_overlay_wireframe;
extern ConVar* r_debug_draw_depth_test;
extern ConVar* r_drawWorldMeshes;
extern ConVar* r_drawWorldMeshesDepthOnly;
extern ConVar* r_drawWorldMeshesDepthAtTheEnd;

#ifndef DEDICATED
extern ConVar* r_visualizetraces;
extern ConVar* r_visualizetraces_duration;

extern ConVar* gfx_nvnUseLowLatency;
extern ConVar* gfx_nvnUseLowLatencyBoost;
#endif // !DEDICATED

extern ConVar* stream_overlay;
extern ConVar* stream_overlay_mode;
//-------------------------------------------------------------------------
// SHARED                                                                 |
extern ConVar* modsystem_enable;
extern ConVar* modsystem_debug;

extern ConVar* eula_version;
extern ConVar* eula_version_accepted;

extern ConVar* promo_version_accepted;

//-------------------------------------------------------------------------
// SERVER                                                                 |
#ifndef CLIENT_DLL
extern ConVar* ai_ainDumpOnLoad;
extern ConVar* ai_ainDebugConnect;
extern ConVar* ai_script_nodes_draw;
extern ConVar* ai_script_nodes_draw_range;
extern ConVar* ai_script_nodes_draw_nearest;

extern ConVar* navmesh_always_reachable;
extern ConVar* navmesh_debug_type;
extern ConVar* navmesh_debug_tile_range;
extern ConVar* navmesh_debug_camera_range;
#ifndef DEDICATED
extern ConVar* navmesh_draw_bvtree;
extern ConVar* navmesh_draw_portal;
extern ConVar* navmesh_draw_polys;
extern ConVar* navmesh_draw_poly_bounds;
extern ConVar* navmesh_draw_poly_bounds_inner;
#endif // DEDICATED
extern ConVar* sv_language;
extern ConVar* sv_showconnecting;
extern ConVar* sv_globalBanlist;
extern ConVar* sv_pylonVisibility;
extern ConVar* sv_pylonRefreshRate;
extern ConVar* sv_banlistRefreshRate;
extern ConVar* sv_statusRefreshRate;
extern ConVar* sv_forceChatToTeamOnly;

extern ConVar* sv_single_core_dedi;

extern ConVar* sv_maxunlag;
extern ConVar* sv_unlag_clamp;
extern ConVar* sv_clockcorrection_msecs;

extern ConVar* sv_updaterate_sp;
extern ConVar* sv_updaterate_mp;

extern ConVar* sv_autoReloadRate;

extern ConVar* sv_simulateBots;
extern ConVar* sv_showhitboxes;
extern ConVar* sv_stats;

extern ConVar* sv_quota_stringCmdsPerSecond;

extern ConVar* sv_validatePersonaName;
extern ConVar* sv_minPersonaNameLength;
extern ConVar* sv_maxPersonaNameLength;

extern ConVar* sv_onlineAuthEnable;

extern ConVar* sv_onlineAuthValidateExpiry;
extern ConVar* sv_onlineAuthExpiryTolerance;

extern ConVar* sv_onlineAuthValidateIssuedAt;
extern ConVar* sv_onlineAuthIssuedAtTolerance;

extern ConVar* sv_voiceEcho;
extern ConVar* sv_voiceenable;
extern ConVar* sv_alltalk;

extern ConVar* player_userCmdsQueueWarning;

//#ifdef DEDICATED
extern ConVar* sv_rcon_debug;
extern ConVar* sv_rcon_sendlogs;
//extern ConVar* sv_rcon_banpenalty;
extern ConVar* sv_rcon_maxfailures;
extern ConVar* sv_rcon_maxignores;
extern ConVar* sv_rcon_maxsockets;
extern ConVar* sv_rcon_maxconnections;
extern ConVar* sv_rcon_maxpacketsize;
extern ConVar* sv_rcon_whitelist_address;
//#endif // DEDICATED
#endif // CLIENT_DLL
extern ConVar* sv_allowClientSideCfgExec;
extern ConVar* sv_cheats;
extern ConVar* sv_visualizetraces;
extern ConVar* sv_visualizetraces_duration;
#if !defined (GAMEDLL_S0) && !defined (GAMEDLL_S1)
extern ConVar* bhit_enable;
extern ConVar* bhit_depth_test;
extern ConVar* bhit_abs_origin;
#endif // !GAMEDLL_S0 && !GAMEDLL_S1
//-------------------------------------------------------------------------
// CLIENT                                                                 |
#ifndef DEDICATED
extern ConVar* cl_rcon_inputonly;
extern ConVar* cl_quota_stringCmdsPerSecond;

extern ConVar* cl_move_use_dt;

extern ConVar* enable_CmdKeyValues;

extern ConVar* cl_notify_invert_x;
extern ConVar* cl_notify_invert_y;
extern ConVar* cl_notify_offset_x;
extern ConVar* cl_notify_offset_y;

extern ConVar* cl_showsimstats;
extern ConVar* cl_simstats_invert_x;
extern ConVar* cl_simstats_invert_y;
extern ConVar* cl_simstats_offset_x;
extern ConVar* cl_simstats_offset_y;

extern ConVar* cl_showgpustats;
extern ConVar* cl_gpustats_invert_x;
extern ConVar* cl_gpustats_invert_y;
extern ConVar* cl_gpustats_offset_x;
extern ConVar* cl_gpustats_offset_y;

extern ConVar* cl_showmaterialinfo;
extern ConVar* cl_materialinfo_offset_x;
extern ConVar* cl_materialinfo_offset_y;

extern ConVar* cl_threaded_bone_setup;

extern ConVar* cl_language;

extern ConVar* cl_onlineAuthEnable;
extern ConVar* cl_onlineAuthToken;
extern ConVar* cl_onlineAuthTokenSignature1;
extern ConVar* cl_onlineAuthTokenSignature2;


extern ConVar* con_drawnotify;
extern ConVar* con_notifylines;
extern ConVar* con_notifytime;

extern ConVar* con_notify_invert_x;
extern ConVar* con_notify_invert_y;
extern ConVar* con_notify_offset_x;
extern ConVar* con_notify_offset_y;

extern ConVar* con_notify_script_server_clr;
extern ConVar* con_notify_script_client_clr;
extern ConVar* con_notify_script_ui_clr;
extern ConVar* con_notify_native_server_clr;
extern ConVar* con_notify_native_client_clr;
extern ConVar* con_notify_native_ui_clr;
extern ConVar* con_notify_native_engine_clr;
extern ConVar* con_notify_native_fs_clr;
extern ConVar* con_notify_native_rtech_clr;
extern ConVar* con_notify_native_ms_clr;
extern ConVar* con_notify_native_audio_clr;
extern ConVar* con_notify_native_video_clr;
extern ConVar* con_notify_netcon_clr;
extern ConVar* con_notify_common_clr;
extern ConVar* con_notify_warning_clr;
extern ConVar* con_notify_error_clr;

extern ConVar* con_max_lines;
extern ConVar* con_max_history;
extern ConVar* con_suggest_limit;
extern ConVar* con_suggest_showhelptext;
extern ConVar* con_suggest_showflags;

extern ConVar* origin_disconnectWhenOffline;
extern ConVar* discord_updatePresence;
#endif // !DEDICATED
//-------------------------------------------------------------------------
// FILESYSTEM                                                             |
extern ConVar* fs_showWarnings;
extern ConVar* fs_showAllReads;
extern ConVar* fs_packedstore_entryblock_stats;
extern ConVar* fs_packedstore_workspace;
extern ConVar* fs_packedstore_compression_level;
extern ConVar* fs_packedstore_max_helper_threads;
//-------------------------------------------------------------------------
// MATERIALSYSTEM                                                         |
#ifndef DEDICATED
extern ConVar* mat_alwaysComplain;
#endif // !DEDICATED
//-------------------------------------------------------------------------
// SQUIRREL                                                               |
extern ConVar* script_show_output;
extern ConVar* script_show_warning;
//-------------------------------------------------------------------------
// NETCHANNEL                                                             |
extern ConVar* net_tracePayload;
extern ConVar* net_encryptionEnable;
extern ConVar* net_useRandomKey;
extern ConVar* net_usesocketsforloopback;
extern ConVar* net_processTimeBudget;

extern ConVar* net_datablock_networkLossForSlowSpeed;

extern ConVar* pylon_matchmaking_hostname;
extern ConVar* pylon_host_update_interval;
extern ConVar* pylon_showdebuginfo;

extern ConVar* ssl_verify_peer;
extern ConVar* curl_timeout;
extern ConVar* curl_debug;
//-------------------------------------------------------------------------
// RTECH API                                                              |
extern ConVar* rtech_debug;
//-------------------------------------------------------------------------
// RUI                                                                    |
#ifndef DEDICATED
extern ConVar* rui_drawEnable;
extern ConVar* rui_defaultDebugFontFace;
#endif // !DEDICATED
//-------------------------------------------------------------------------
// MILES                                                                  |
#ifndef DEDICATED
extern ConVar* miles_debug;
extern ConVar* miles_language;
#endif

void ConVar_StaticInit(void);
void ConVar_InitShipped(void);
void ConVar_PurgeShipped(void);
void ConVar_PurgeHostNames(void);
void ConCommand_StaticInit(void);
void ConCommand_InitShipped(void);
void ConCommand_PurgeShipped(void);

#endif // GLOBAL_H
