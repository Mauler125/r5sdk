#pragma once
#include "tier1/IConVar.h"
#include <vstdlib/concommandhash.h>

//-------------------------------------------------------------------------
// ENGINE                                                                 |
extern ConVar* sdk_fixedframe_tickinterval;
extern ConVar* single_frame_shutdown_for_reload;
extern ConVar* old_gather_props;
extern ConVar* cm_unset_all_cmdquery;

extern ConVar* enable_debug_overlays;
extern ConVar* debug_draw_box_depth_test;

extern ConVar* developer;
extern ConVar* fps_max;

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

extern ConVar* curl_debug;
extern ConVar* ssl_verify_peer;

extern ConVar* rcon_address;
extern ConVar* rcon_password;

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
#endif // !DEDICATED

extern ConVar* stream_overlay;
extern ConVar* stream_overlay_mode;
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
extern ConVar* sv_showconnecting;
extern ConVar* sv_globalBanlist;
extern ConVar* sv_pylonVisibility;
extern ConVar* sv_pylonRefreshRate;
extern ConVar* sv_banlistRefreshRate;
extern ConVar* sv_statusRefreshRate;
extern ConVar* sv_forceChatToTeamOnly;

extern ConVar* sv_updaterate_mp;
extern ConVar* sv_updaterate_sp;

extern ConVar* sv_autoReloadRate;
extern ConVar* sv_quota_stringCmdsPerSecond;

extern ConVar* sv_simulateBots;
extern ConVar* sv_showhitboxes;
extern ConVar* sv_stats;

//#ifdef DEDICATED
extern ConVar* sv_rcon_debug;
extern ConVar* sv_rcon_sendlogs;
extern ConVar* sv_rcon_banpenalty;
extern ConVar* sv_rcon_maxfailures;
extern ConVar* sv_rcon_maxignores;
extern ConVar* sv_rcon_maxsockets;
extern ConVar* sv_rcon_whitelist_address;
//#endif // DEDICATED
#endif // CLIENT_DLL
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
extern ConVar* cl_rcon_request_sendlogs;
extern ConVar* cl_quota_stringCmdsPerSecond;

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
extern ConVar* con_suggestion_limit;
extern ConVar* con_suggestion_showhelptext;
extern ConVar* con_suggestion_showflags;
extern ConVar* con_suggestion_flags_realtime;

extern ConVar* origin_disconnectWhenOffline;
#endif // !DEDICATED
//-------------------------------------------------------------------------
// FILESYSTEM                                                             |
extern ConVar* fs_warning_level_sdk;
extern ConVar* fs_show_warning_output;
extern ConVar* fs_packedstore_entryblock_stats;
extern ConVar* fs_packedstore_workspace;
extern ConVar* fs_packedstore_compression_level;
extern ConVar* fs_packedstore_max_helper_threads;
//-------------------------------------------------------------------------
// MATERIALSYSTEM                                                         |
#ifndef DEDICATED
extern ConVar* mat_showdxoutput;
extern ConVar* mat_alwaysComplain;
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
extern ConVar* net_processTimeBudget;

extern ConVar* pylon_matchmaking_hostname;
extern ConVar* pylon_host_update_interval;
extern ConVar* pylon_showdebuginfo;
//-------------------------------------------------------------------------
// RTECH API                                                              |
extern ConVar* rtech_debug;
//-------------------------------------------------------------------------
// RUI                                                                    |
#ifndef DEDICATED
extern ConVar* rui_drawEnable;
extern ConVar* rui_defaultDebugFontFace;
#endif // !DEDICATED
//-----------------------------------------------------------------------------
// MILES                                                                      |
#ifndef DEDICATED
extern ConVar* miles_debug;
extern ConVar* miles_language;
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CCvarUtilities
{
public:
	//bool IsCommand(const CCommand& args);

	// Writes lines containing "set variable value" for all variables
	// with the archive flag set to true.
	//void WriteVariables(CUtlBuffer& buff, bool bAllVars);

	// Returns the # of cvars with the server flag set.
	int	CountVariablesWithFlags(int flags);

	// Enable cvars marked with FCVAR_DEVELOPMENTONLY
	void EnableDevCvars();

	// Lists cvars to console
	void CvarList(const CCommand& args);

	// Prints help text for cvar
	void CvarHelp(const CCommand& args);

	// Revert all cvar values
	//void CvarRevert(const CCommand& args);

	// Revert all cvar values
	void CvarDifferences(const CCommand& args);

	// Toggles a cvar on/off, or cycles through a set of values
	//void CvarToggle(const CCommand& args);

	// Finds commands with a specified flag.
	void CvarFindFlags_f(const CCommand& args);


	int CvarFindFlagsCompletionCallback(const char* partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH]);

private:
	// just like Cvar_set, but optimizes out the search
	//void SetDirect(ConVar* var, const char* value);

	//bool IsValidToggleCommand(const char* cmd);
};

extern CCvarUtilities* cv;

class CCvar : public CBaseAppSystem< ICvar >
{ 	// Implementation in engine.
public:
	unordered_map<string, ConCommandBase*> DumpToMap(void);

protected:
	enum ConVarSetType_t
	{
		CONVAR_SET_STRING = 0,
		CONVAR_SET_INT,
		CONVAR_SET_FLOAT,
	};

	struct QueuedConVarSet_t
	{
		ConVar* m_pConVar;
		ConVarSetType_t m_nType;
		int m_nInt;
		float m_flFloat;
		//CUtlString m_String; // !TODO:
	};

	class CCVarIteratorInternal : public ICVarIteratorInternal
	{
	public:
		virtual void            SetFirst(void) = 0; //0
		virtual void            Next(void) = 0; //1
		virtual	bool            IsValid(void) = 0; //2
		virtual ConCommandBase* Get(void) = 0; //3

		CCvar* const m_pOuter;
		CConCommandHash* const m_pHash;
		CConCommandHash::CCommandHashIterator_t m_hashIter;
	};

	virtual CCVarIteratorInternal* FactoryInternalIterator(void) = 0;

	friend class CCVarIteratorInternal;
	friend class CCvarUtilities;

private:
	CUtlVector< FnChangeCallback_t > m_GlobalChangeCallbacks;
	char pad0[30];           //!TODO:
	int m_nNextDLLIdentifier;
	ConCommandBase* m_pConCommandList;
	CConCommandHash m_CommandHash;
	CUtlVector<void*> m_Unknown;
	char pad2[32];
	void* m_pCallbackStub;
	void* m_pAllocFunc;
	char pad3[16];
	CUtlVector< QueuedConVarSet_t > m_QueuedConVarSets;
	bool m_bMaterialSystemThreadSetAllowed;
};

///////////////////////////////////////////////////////////////////////////////
extern CCvar* g_pCVar;

/* ==== CCVAR =========================================================================================================================================================== */
///////////////////////////////////////////////////////////////////////////////
class VCVar : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogVarAdr("g_pCVar", reinterpret_cast<uintptr_t>(g_pCVar));
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const
	{
		g_pCVar = g_GameDll.FindPatternSIMD("48 83 EC 28 48 8B 05 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ?? 48 85 C0 48 89 15")
			.FindPatternSelf("48 8D 0D", CMemory::Direction::DOWN, 40).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CCvar*>();

		//g_pCVar = g_GameDll.FindPatternSIMD("40 53 48 83 EC 20 48 83 3D ?? ?? ?? ?? ?? 48 8B D9 74 09") // Actual CCvar, above is the vtable ptr.
			//.FindPatternSelf("48 83 3D", CMemory::Direction::DOWN).ResolveRelativeAddressSelf(0x3, 0x8).RCast<CCvar*>();
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////
