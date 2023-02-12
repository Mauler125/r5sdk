//=============================================================================//
//
// Purpose: Runs the state machine for the host & server.
//
//=============================================================================//
// host_state.cpp: methods are declared inline to prevent stack corruption.
//
/////////////////////////////////////////////////////////////////////////////////
#include "core/stdafx.h"
#include "tier0/jobthread.h"
#include "tier0/commandline.h"
#include "tier0/fasttimer.h"
#include "tier1/cmd.h"
#include "tier1/cvar.h"
#include "tier1/NetAdr.h"
#include "tier2/socketcreator.h"
#include "vpc/keyvalues.h"
#include "datacache/mdlcache.h"
#ifndef CLIENT_DLL
#include "engine/server/sv_rcon.h"
#endif // !CLIENT_DLL
#ifndef DEDICATED
#include "engine/client/cl_rcon.h"
#include "engine/client/cl_main.h"
#include "engine/client/clientstate.h"
#endif // DEDICATED
#include "engine/net.h"
#include "engine/gl_screen.h"
#include "engine/host.h"
#include "engine/host_cmd.h"
#include "engine/host_state.h"
#include "engine/sys_engine.h"
#include "engine/modelloader.h"
#include "engine/cmodel_bsp.h"
#ifndef CLIENT_DLL
#include "engine/server/server.h"
#endif // !CLIENT_DLL
#include "rtech/rtech_game.h"
#include "rtech/rtech_utils.h"
#include "rtech/stryder/stryder.h"
#ifndef DEDICATED
#include "vgui/vgui_baseui_interface.h"
#include "client/vengineclient_impl.h"
#endif // DEDICATED
#include "networksystem/pylon.h"
#ifndef CLIENT_DLL
#include "networksystem/bansystem.h"
#endif // !CLIENT_DLL
#include "networksystem/listmanager.h"
#include "public/edict.h"
#ifndef CLIENT_DLL
#include "game/server/gameinterface.h"
#endif // !CLIENT_DLL
#include "squirrel/sqinit.h"

//-----------------------------------------------------------------------------
// Purpose: state machine's main processing loop
//-----------------------------------------------------------------------------
FORCEINLINE void CHostState::FrameUpdate(CHostState* pHostState, double flCurrentTime, float flFrameTime)
{
	static bool bInitialized = false;
	static bool bResetIdleName = false;
	if (!bInitialized)
	{
		g_pHostState->Setup();
		bInitialized = true;
	}

	g_pHostState->Think();
#ifndef CLIENT_DLL
	RCONServer()->RunFrame();
#endif // !CLIENT_DLL
#ifndef DEDICATED
	RCONClient()->RunFrame();
#endif // !DEDICATED

	HostStates_t oldState{};
	if (setjmp(*host_abortserver))
	{
		g_pHostState->Init();
		return;
	}
	else
	{
#ifndef CLIENT_DLL
		*g_bAbortServerSet = true;
#endif // !CLIENT_DLL
		do
		{
			Cbuf_Execute();
			oldState = g_pHostState->m_iCurrentState;

			switch (g_pHostState->m_iCurrentState)
			{
			case HostStates_t::HS_NEW_GAME:
			{
				g_pHostState->State_NewGame();
				break;
			}
			case HostStates_t::HS_CHANGE_LEVEL_SP:
			{
				g_pHostState->State_ChangeLevelSP();
				break;
			}
			case HostStates_t::HS_CHANGE_LEVEL_MP:
			{
				g_pHostState->State_ChangeLevelMP();
				break;
			}
			case HostStates_t::HS_RUN:
			{
				if (!g_pHostState->m_bActiveGame)
				{
					if (bResetIdleName)
					{
						g_pHostState->ResetLevelName();
						bResetIdleName = false;
					}
				}
				else // Reset idle name the next non-active frame.
				{
					bResetIdleName = true;
				}

#if !defined (DEDICATED) && !defined (CLIENT_DLL)
				// Parallel processing of 'C_BaseAnimating::SetupBones()' is not supported
				// on listen servers running the local client.
				if (g_pServer->IsActive())
				{
					if (cl_threaded_bone_setup->GetBool())
					{
						cl_threaded_bone_setup->SetValue(false);
					}
				}
#endif // !DEDICATED && !CLIENT_DLL

				CHostState_State_Run(&g_pHostState->m_iCurrentState, flCurrentTime, flFrameTime);
				break;
			}
			case HostStates_t::HS_GAME_SHUTDOWN:
			{
				DevMsg(eDLL_T::ENGINE, "%s: Shutdown host game\n", __FUNCTION__);
				CHostState_State_GameShutDown(g_pHostState);
				break;
			}
			case HostStates_t::HS_RESTART:
			{
				DevMsg(eDLL_T::ENGINE, "%s: Restarting state machine\n", __FUNCTION__);
#ifndef DEDICATED
				CL_EndMovie();
#endif // !DEDICATED
				Stryder_SendOfflineRequest(); // We have hostnames nulled anyway.
				g_pEngine->SetNextState(IEngine::DLL_RESTART);
				break;
			}
			case HostStates_t::HS_SHUTDOWN:
			{
				DevMsg(eDLL_T::ENGINE, "%s: Shutdown state machine\n", __FUNCTION__);
#ifndef DEDICATED
				CL_EndMovie();
#endif // !DEDICATED
				Stryder_SendOfflineRequest(); // We have hostnames nulled anyway.
				g_pEngine->SetNextState(IEngine::DLL_CLOSE);
				break;
			}
			default:
			{
				break;
			}
			}

		} while ((oldState != HostStates_t::HS_RUN || g_pHostState->m_iNextState == HostStates_t::HS_LOAD_GAME && single_frame_shutdown_for_reload->GetBool())
			&& oldState != HostStates_t::HS_SHUTDOWN
			&& oldState != HostStates_t::HS_RESTART);
	}
}

//-----------------------------------------------------------------------------
// Purpose: state machine initialization
//-----------------------------------------------------------------------------
FORCEINLINE void CHostState::Init(void)
{
	if (m_iNextState != HostStates_t::HS_SHUTDOWN)
	{
		if (m_iNextState == HostStates_t::HS_GAME_SHUTDOWN)
		{
			CHostState_State_GameShutDown(this);
		}
		else
		{
			m_iCurrentState = HostStates_t::HS_RUN;
			if (m_iNextState != HostStates_t::HS_SHUTDOWN || !single_frame_shutdown_for_reload->GetInt())
				m_iNextState = HostStates_t::HS_RUN;
		}
	}
	m_flShortFrameTime = 1.0f;
	m_levelName[0] = 0;
	m_landMarkName[0] = 0;
	m_mapGroupName[0] = 0;
	m_nSplitScreenPlayers = 256;
	m_vecLocation.Init();
	m_angLocation.Init();
	m_iCurrentState = HostStates_t::HS_NEW_GAME;
}

//-----------------------------------------------------------------------------
// Purpose: state machine setup
//-----------------------------------------------------------------------------
FORCEINLINE void CHostState::Setup(void) 
{
	g_pHostState->LoadConfig();
#ifndef CLIENT_DLL
	g_pBanSystem->Load();
#endif // !CLIENT_DLL
	ConVar::PurgeHostNames();

	net_usesocketsforloopback->SetValue(1);
	if (net_useRandomKey->GetBool())
	{
		NET_GenerateKey();
	}
	ResetLevelName();
}

//-----------------------------------------------------------------------------
// Purpose: think
//-----------------------------------------------------------------------------
FORCEINLINE void CHostState::Think(void) const
{
	static bool bInitialized = false;
	static CFastTimer banListTimer;
	static CFastTimer pylonTimer;
	static CFastTimer reloadTimer;
	static CFastTimer statsTimer;

	if (!bInitialized) // Initialize clocks.
	{
#ifndef CLIENT_DLL
		banListTimer.Start();
#ifdef DEDICATED
		pylonTimer.Start();
#endif // DEDICATED
		statsTimer.Start();
		reloadTimer.Start();
#endif // !CLIENT_DLL
		bInitialized = true;
	}
#ifndef CLIENT_DLL
	if (banListTimer.GetDurationInProgress().GetSeconds() > sv_banlistRefreshRate->GetDouble())
	{
		g_pBanSystem->BanListCheck();
		banListTimer.Start();
	}
#endif // !CLIENT_DLL
#ifdef DEDICATED
	if (pylonTimer.GetDurationInProgress().GetSeconds() > sv_pylonRefreshRate->GetDouble())
	{
		const NetGameServer_t netGameServer
		{
			hostname->GetString(),
			hostdesc->GetString(),
			sv_pylonVisibility->GetInt() == EServerVisibility_t::HIDDEN,
			g_pHostState->m_levelName,
			mp_gamemode->GetString(),
			hostip->GetString(),
			hostport->GetString(),
			g_pNetKey->GetBase64NetKey(),
			std::to_string(*g_nServerRemoteChecksum),
			SDK_VERSION,
			std::to_string(g_pServer->GetNumHumanPlayers() + g_pServer->GetNumFakeClients()),
			std::to_string(g_ServerGlobalVariables->m_nMaxClients),
			std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::system_clock::now().time_since_epoch()
				).count()
		};

		std::thread(&CPylon::KeepAlive, g_pMasterServer, netGameServer).detach();
		pylonTimer.Start();
	}
#endif // DEDICATED
#ifndef CLIENT_DLL
	if (sv_autoReloadRate->GetBool())
	{
		if (reloadTimer.GetDurationInProgress().GetSeconds() > sv_autoReloadRate->GetDouble())
		{
			Cbuf_AddText(Cbuf_GetCurrentPlayer(), "reload\n", cmd_source_t::kCommandSrcCode);
			reloadTimer.Start();
		}
	}
	if (statsTimer.GetDurationInProgress().GetSeconds() > sv_statusRefreshRate->GetDouble())
	{
		string svCurrentPlaylist = KeyValues_GetCurrentPlaylist();
		int32_t nPlayerCount = g_pServer->GetNumHumanPlayers();

		SetConsoleTitleA(fmt::format("{:s} - {:d}/{:d} Players ({:s} on {:s})",
			hostname->GetString(), nPlayerCount, g_ServerGlobalVariables->m_nMaxClients, svCurrentPlaylist, m_levelName).c_str());
		statsTimer.Start();
	}
#endif // !CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: load and execute configuration files
//-----------------------------------------------------------------------------
FORCEINLINE void CHostState::LoadConfig(void) const
{
	if (CommandLine()->ParmValue("-launcher", 0) < 1) // Launcher level 1 indicates everything is handled from the commandline/launcher.
	{
		if (!CommandLine()->CheckParm("-devsdk"))
		{
#ifndef CLIENT_DLL
			Cbuf_AddText(Cbuf_GetCurrentPlayer(), "exec \"autoexec_server.cfg\"", cmd_source_t::kCommandSrcCode);
			Cbuf_AddText(Cbuf_GetCurrentPlayer(), "exec \"rcon_server.cfg\"", cmd_source_t::kCommandSrcCode);
#endif //!CLIENT_DLL
#ifndef DEDICATED
			Cbuf_AddText(Cbuf_GetCurrentPlayer(), "exec \"autoexec_client.cfg\"", cmd_source_t::kCommandSrcCode);
			Cbuf_AddText(Cbuf_GetCurrentPlayer(), "exec \"rcon_client.cfg\"", cmd_source_t::kCommandSrcCode);
#endif // !DEDICATED
			Cbuf_AddText(Cbuf_GetCurrentPlayer(), "exec \"autoexec.cfg\"", cmd_source_t::kCommandSrcCode);
		}
		else // Development configs.
		{
#ifndef CLIENT_DLL
			Cbuf_AddText(Cbuf_GetCurrentPlayer(), "exec \"autoexec_server_dev.cfg\"", cmd_source_t::kCommandSrcCode);
			Cbuf_AddText(Cbuf_GetCurrentPlayer(), "exec \"rcon_server_dev.cfg\"", cmd_source_t::kCommandSrcCode);
#endif //!CLIENT_DLL
#ifndef DEDICATED
			Cbuf_AddText(Cbuf_GetCurrentPlayer(), "exec \"autoexec_client_dev.cfg\"", cmd_source_t::kCommandSrcCode);
			Cbuf_AddText(Cbuf_GetCurrentPlayer(), "exec \"rcon_client_dev.cfg\"", cmd_source_t::kCommandSrcCode);
#endif // !DEDICATED
			Cbuf_AddText(Cbuf_GetCurrentPlayer(), "exec \"autoexec_dev.cfg\"", cmd_source_t::kCommandSrcCode);
		}
#ifndef DEDICATED
		Cbuf_AddText(Cbuf_GetCurrentPlayer(), "exec \"bind.cfg\"", cmd_source_t::kCommandSrcCode);
#endif // !DEDICATED
		Cbuf_Execute();
	}
}

//-----------------------------------------------------------------------------
// Purpose: shutdown active game
//-----------------------------------------------------------------------------
FORCEINLINE void CHostState::GameShutDown(void)
{
	if (m_bActiveGame)
	{
#ifndef CLIENT_DLL
		g_pServerGameDLL->GameShutdown();
#endif // !CLIENT_DLL
		m_bActiveGame = false;
		ResetLevelName();
	}
}

//-----------------------------------------------------------------------------
// Purpose: initialize new game
//-----------------------------------------------------------------------------
FORCEINLINE void CHostState::State_NewGame(void)
{
	DevMsg(eDLL_T::ENGINE, "%s: Loading level: '%s'\n", __FUNCTION__, g_pHostState->m_levelName);

	LARGE_INTEGER time{};
	uint16_t nSplitScreenPlayers = m_nSplitScreenPlayers;
	m_nSplitScreenPlayers = 0;
#ifndef CLIENT_DLL
	if (!g_pServerGameClients) // Init Game if it ain't valid.
	{
		SV_InitGameDLL();
	}
#endif // !CLIENT_DLL

#ifndef CLIENT_DLL
	if (!CModelLoader__Map_IsValid(g_pModelLoader, m_levelName) // Check if map is valid and if we can start a new game.
		|| !Host_NewGame(m_levelName, nullptr, m_bBackgroundLevel, nSplitScreenPlayers, time) || !g_pServerGameClients)
	{
		Error(eDLL_T::ENGINE, NO_ERROR, "%s: Error: Level not valid\n", __FUNCTION__);
#ifndef DEDICATED
		SCR_EndLoadingPlaque();
#endif // !DEDICATED
		GameShutDown();
	}
#endif // !CLIENT_DLL

	m_iCurrentState = HostStates_t::HS_RUN; // Set current state to run.

	// If our next state isn't a shutdown or its a forced shutdown then set next state to run.
	if (m_iNextState != HostStates_t::HS_SHUTDOWN || !host_hasIrreversibleShutdown->GetBool())
	{
		m_iNextState = HostStates_t::HS_RUN;
	}
}

//-----------------------------------------------------------------------------
// Purpose: change singleplayer level
//-----------------------------------------------------------------------------
FORCEINLINE void CHostState::State_ChangeLevelSP(void)
{
	DevMsg(eDLL_T::ENGINE, "%s: Changing singleplayer level to: '%s'\n", __FUNCTION__, m_levelName);
	m_flShortFrameTime = 1.5; // Set frame time.

	if (CModelLoader__Map_IsValid(g_pModelLoader, m_levelName)) // Check if map is valid and if we can start a new game.
	{
		Host_ChangeLevel(true, m_levelName, m_mapGroupName); // Call change level as singleplayer level.
	}
	else
	{
		Error(eDLL_T::ENGINE, NO_ERROR, "%s: Error: unable to find level: '%s'\n", __FUNCTION__, m_levelName);
	}

	m_iCurrentState = HostStates_t::HS_RUN; // Set current state to run.

	// If our next state isn't a shutdown or its a forced shutdown then set next state to run.
	if (m_iNextState != HostStates_t::HS_SHUTDOWN || !host_hasIrreversibleShutdown->GetBool())
	{
		m_iNextState = HostStates_t::HS_RUN;
	}
}

//-----------------------------------------------------------------------------
// Purpose: change multiplayer level
//-----------------------------------------------------------------------------
FORCEINLINE void CHostState::State_ChangeLevelMP(void)
{
	DevMsg(eDLL_T::ENGINE, "%s: Changing multiplayer level to: '%s'\n", __FUNCTION__, m_levelName);
	m_flShortFrameTime = 0.5; // Set frame time.

#ifndef CLIENT_DLL
	g_pServerGameDLL->LevelShutdown();
#endif // !CLIENT_DLL
	if (CModelLoader__Map_IsValid(g_pModelLoader, m_levelName)) // Check if map is valid and if we can start a new game.
	{
#ifndef DEDICATED
		g_pEngineVGui->EnabledProgressBarForNextLoad();
#endif // !DEDICATED
		Host_ChangeLevel(false, m_levelName, m_mapGroupName); // Call change level as multiplayer level.
	}
	else
	{
		Error(eDLL_T::ENGINE, NO_ERROR, "%s: Error: unable to find level: '%s'\n", __FUNCTION__, m_levelName);
	}

	m_iCurrentState = HostStates_t::HS_RUN; // Set current state to run.

	// If our next state isn't a shutdown or its a forced shutdown then set next state to run.
	if (m_iNextState != HostStates_t::HS_SHUTDOWN || !host_hasIrreversibleShutdown->GetBool())
	{
		m_iNextState = HostStates_t::HS_RUN;
	}
}

//-----------------------------------------------------------------------------
// Purpose: resets the level name
//-----------------------------------------------------------------------------
FORCEINLINE void CHostState::ResetLevelName(void)
{
#ifdef DEDICATED
	static const char* szNoMap = "server_idle";
#else // DEDICATED
	static const char* szNoMap = "menu_main";
#endif
	Q_snprintf(const_cast<char*>(m_levelName), sizeof(m_levelName), szNoMap);
}

void VHostState::Attach(void) const
{
	DetourAttach(&CHostState_FrameUpdate, &CHostState::FrameUpdate);
}
void VHostState::Detach(void) const
{
	DetourDetach(&CHostState_FrameUpdate, &CHostState::FrameUpdate);
}

///////////////////////////////////////////////////////////////////////////////
CHostState* g_pHostState = nullptr;
