//=============================================================================//
//
// Purpose: Runs the state machine for the host & server.
//
//=============================================================================//

#include "core/stdafx.h"
#include "tier0/cmd.h"
#include "tier0/cvar.h"
#include "tier0/commandline.h"
#include "tier0/fasttimer.h"
#include "tier1/NetAdr2.h"
#include "tier2/socketcreator.h"
#include "vpc/keyvalues.h"
#ifdef DEDICATED
#include "engine/sv_rcon.h"
#else // 
#include "engine/cl_rcon.h"
#endif // DEDICATED
#include "engine/gl_screen.h"
#include "engine/host_state.h"
#include "engine/net_chan.h"
#include "engine/sys_engine.h"
#include "engine/sys_utils.h"
#include "engine/cmodel_bsp.h"
#ifndef GAMECLIENTONLY
#include "engine/baseserver.h"
#endif // !GAMECLIENTONLY
#include "rtech/rtech_game.h"
#ifndef DEDICATED
#include "vgui/vgui_baseui_interface.h"
#endif // DEDICATED
#include "client/IVEngineClient.h"
#include "networksystem/pylon.h"
#include "public/include/bansystem.h"
#include "public/include/edict.h"
#ifndef GAMECLIENTONLY
#include "game/server/gameinterface.h"
#endif // !GAMECLIENTONLY

bool g_bLevelResourceInitialized = false;
//-----------------------------------------------------------------------------
// Purpose: state machine's main processing loop
//-----------------------------------------------------------------------------
FORCEINLINE void CHostState::FrameUpdate(void* rcx, void* rdx, float time)
{
	static bool bInitialized = false;
	static ConVar* single_frame_shutdown_for_reload = g_pCVar->FindVar("single_frame_shutdown_for_reload");
	if (!bInitialized)
	{
		g_pHostState->Setup();
		bInitialized = true;
	}
#ifdef DEDICATED
	g_pRConServer->RunFrame();
#else // 
	g_pRConClient->RunFrame();
#endif // DEDICATED

	HostStates_t oldState{};
	void* placeHolder = nullptr;
	if (setjmpFn(*host_abortserver, placeHolder))
	{
		CHostState_InitFn(g_pHostState);
		return;
	}
	else
	{
#ifndef GAMECLIENTONLY
		*g_ServerAbortServer = true;
#endif // !GAMECLIENTONLY
		do
		{
			Cbuf_Execute();
			oldState = g_pHostState->m_iCurrentState;

			switch (g_pHostState->m_iCurrentState)
			{
			case HostStates_t::HS_NEW_GAME:
			{
				DevMsg(eDLL_T::ENGINE, "%s - Loading level: '%s'\n", "CHostState::FrameUpdate", g_pHostState->m_levelName);
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
				State_RunFn(&g_pHostState->m_iCurrentState, nullptr, time);
				break;
			}
			case HostStates_t::HS_GAME_SHUTDOWN:
			{
				DevMsg(eDLL_T::ENGINE, "%s - Shutdown host game\n", "CHostState::FrameUpdate");

				g_bLevelResourceInitialized = false;
				Host_Game_ShutdownFn(g_pHostState);
				break;
			}
			case HostStates_t::HS_RESTART:
			{
				DevMsg(eDLL_T::ENGINE, "%s - Restarting state machine\n", "CHostState::FrameUpdate");
				g_bLevelResourceInitialized = false;
#ifndef DEDICATED
				CL_EndMovieFn();
#endif // !DEDICATED
				SendOfflineRequestToStryderFn(); // We have hostnames nulled anyway.
				g_pEngine->SetNextState(EngineState_t::DLL_RESTART);
				break;
			}
			case HostStates_t::HS_SHUTDOWN:
			{
				DevMsg(eDLL_T::ENGINE, "%s - Shutdown state machine\n", "CHostState::FrameUpdate");
				g_bLevelResourceInitialized = false;
#ifndef DEDICATED
				CL_EndMovieFn();
#endif // !DEDICATED
				SendOfflineRequestToStryderFn(); // We have hostnames nulled anyway.
				g_pEngine->SetNextState(EngineState_t::DLL_CLOSE);
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
// Purpose: setup state machine
//-----------------------------------------------------------------------------
FORCEINLINE void CHostState::Setup(void) const
{
	g_pHostState->LoadConfig();
	g_pConVar->ClearHostNames();
#ifdef DEDICATED
	g_pRConServer->Init();
#else // 
	g_pRConClient->Init();
#endif // DEDICATED

	std::thread t1(&CHostState::Think, this);
	t1.detach();

	*reinterpret_cast<bool*>(m_bRestrictServerCommands) = true; // Restrict commands.
	ConCommandBase* disconnect = g_pCVar->FindCommandBase("disconnect");
	disconnect->AddFlags(FCVAR_SERVER_CAN_EXECUTE); // Make sure server is not restricted to this.
	g_pCVar->FindVar("net_usesocketsforloopback")->SetValue(1);

	if (net_userandomkey->GetBool())
	{
		HNET_GenerateKey();
	}

#ifdef DEDICATED
	const char* szNoMap = "server_idle";
#else // DEDICATED
	const char* szNoMap = "main_menu";
#endif
	snprintf(const_cast<char*>(m_levelName), sizeof(m_levelName), szNoMap);
}

//-----------------------------------------------------------------------------
// Purpose: think
//-----------------------------------------------------------------------------
FORCEINLINE void CHostState::Think(void) const
{
	static bool bInitialized = false;
	static CFastTimer banListTimer;
	static CFastTimer pylonTimer;
	static CFastTimer statsTimer;
	static ConVar* hostname = g_pCVar->FindVar("hostname");

	for (;;) // Loop running at 20-tps.
	{
		if (!bInitialized) // Initialize clocks.
		{
			banListTimer.Start();
#ifdef DEDICATED
			pylonTimer.Start();
#endif // DEDICATED
			statsTimer.Start();
			bInitialized = true;
		}

		if (banListTimer.GetDurationInProgress().GetSeconds() > 1.0)
		{
			g_pBanSystem->BanListCheck();
			banListTimer.Start();
		}
#ifdef DEDICATED
		if (pylonTimer.GetDurationInProgress().GetSeconds() > 5.0)
		{
			KeepAliveToPylon();
			pylonTimer.Start();
		}
#endif // DEDICATED
		if (statsTimer.GetDurationInProgress().GetSeconds() > 1.0)
		{
			std::string svCurrentPlaylist = KeyValues_GetCurrentPlaylist();
			std::int64_t nPlayerCount = g_pServer->GetNumHumanPlayers();

			SetConsoleTitleA(fmt::format("{} - {}/{} Players ({} on {})",
				hostname->GetString(), nPlayerCount, g_ServerGlobalVariables->m_nMaxClients, svCurrentPlaylist.c_str(), m_levelName).c_str());
			statsTimer.Start();
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}
}

//-----------------------------------------------------------------------------
// Purpose: load and execute configuration files
//-----------------------------------------------------------------------------
FORCEINLINE void CHostState::LoadConfig(void) const
{
	if (!g_pCmdLine->CheckParm("-devsdk"))
	{
		Cbuf_AddText(Cbuf_GetCurrentPlayer(), "exec \"autoexec_server.cfg\"", cmd_source_t::kCommandSrcCode);
		Cbuf_AddText(Cbuf_GetCurrentPlayer(), "exec \"rcon_server.cfg\"", cmd_source_t::kCommandSrcCode);
#ifndef DEDICATED
		Cbuf_AddText(Cbuf_GetCurrentPlayer(), "exec \"autoexec_client.cfg\"", cmd_source_t::kCommandSrcCode);
		Cbuf_AddText(Cbuf_GetCurrentPlayer(), "exec \"rcon_client.cfg\"", cmd_source_t::kCommandSrcCode);
#endif // !DEDICATED
		Cbuf_AddText(Cbuf_GetCurrentPlayer(), "exec \"autoexec.cfg\"", cmd_source_t::kCommandSrcCode);
	}
	else // Development configs.
	{
		Cbuf_AddText(Cbuf_GetCurrentPlayer(), "exec \"autoexec_server_dev.cfg\"", cmd_source_t::kCommandSrcCode);
		Cbuf_AddText(Cbuf_GetCurrentPlayer(), "exec \"rcon_server_dev.cfg\"", cmd_source_t::kCommandSrcCode);
#ifndef DEDICATED
		Cbuf_AddText(Cbuf_GetCurrentPlayer(), "exec \"autoexec_client_dev.cfg\"", cmd_source_t::kCommandSrcCode);
		Cbuf_AddText(Cbuf_GetCurrentPlayer(), "exec \"rcon_client_dev.cfg\"", cmd_source_t::kCommandSrcCode);
#endif // !DEDICATED
		Cbuf_AddText(Cbuf_GetCurrentPlayer(), "exec \"autoexec_dev.cfg\"", cmd_source_t::kCommandSrcCode);
	}
	Cbuf_Execute();
}

//-----------------------------------------------------------------------------
// Purpose: shutdown active game
//-----------------------------------------------------------------------------
FORCEINLINE void CHostState::GameShutDown(void)
{
	g_bLevelResourceInitialized = false;
	if (m_bActiveGame)
	{
#ifndef GAMECLIENTONLY
		g_pServerGameDLL->GameShutdown();
#endif // !GAMECLIENTONLY
		m_bActiveGame = 0;
#ifdef DEDICATED
		const char* szNoMap = "server_idle";
#else // DEDICATED
		const char* szNoMap = "main_menu";
#endif
		snprintf(const_cast<char*>(m_levelName), sizeof(m_levelName), szNoMap);
	}
}

//-----------------------------------------------------------------------------
// Purpose: unloads all pakfiles loaded by the SDK
//-----------------------------------------------------------------------------
FORCEINLINE void CHostState::UnloadPakFile(void)
{
	for (int i = 0; i < sizeof(g_nLoadedPakFileId); i++)
	{
		if (g_nLoadedPakFileId[i] > 0)
		{
			RTech_UnloadPak(g_nLoadedPakFileId[i]);
		}
		else
		{
			memset(g_nLoadedPakFileId, '\0', sizeof(g_nLoadedPakFileId));
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: initialize new game
//-----------------------------------------------------------------------------
FORCEINLINE void CHostState::State_NewGame(void)
{
	g_bLevelResourceInitialized = false;
	m_bSplitScreenConnect = false;
	if (!g_ServerGameClients) // Init Game if it ain't valid.
	{
		SV_InitGameDLLFn();
	}

	if (!CModelLoader_Map_IsValidFn(g_CModelLoader, m_levelName) // Check if map is valid and if we can start a new game.
		|| !Host_NewGameFn(m_levelName, nullptr, m_bBackgroundLevel, m_bSplitScreenConnect, nullptr) || !g_ServerGameClients)
	{
		Error(eDLL_T::ENGINE, "%s - Error: Map not valid\n", "CHostState::State_NewGame");
#ifndef DEDICATED
		SCR_EndLoadingPlaque();
#endif // !DEDICATED
		GameShutDown();
	}

	m_iCurrentState = HostStates_t::HS_RUN; // Set current state to run.

	// If our next state isn't a shutdown or its a forced shutdown then set next state to run.
	if (m_iNextState != HostStates_t::HS_SHUTDOWN || !g_pCVar->FindVar("host_hasIrreversibleShutdown")->GetBool())
	{
		m_iNextState = HostStates_t::HS_RUN;
	}
}

//-----------------------------------------------------------------------------
// Purpose: change singleplayer level
//-----------------------------------------------------------------------------
FORCEINLINE void CHostState::State_ChangeLevelSP(void)
{
	DevMsg(eDLL_T::ENGINE, "%s - Changing singleplayer level to: '%s'\n", "CHostState::State_ChangeLevelSP", m_levelName);
	m_flShortFrameTime = 1.5; // Set frame time.
	g_bLevelResourceInitialized = false;

	if (CModelLoader_Map_IsValidFn(g_CModelLoader, m_levelName)) // Check if map is valid and if we can start a new game.
	{
		Host_ChangelevelFn(true, m_levelName, m_mapGroupName); // Call change level as singleplayer level.
	}
	else
	{
		Error(eDLL_T::ENGINE, "%s - Error: Unable to find map: '%s'\n", "CHostState::State_ChangeLevelSP", m_levelName);
	}

	m_iCurrentState = HostStates_t::HS_RUN; // Set current state to run.

	// If our next state isn't a shutdown or its a forced shutdown then set next state to run.
	if (m_iNextState != HostStates_t::HS_SHUTDOWN || !g_pCVar->FindVar("host_hasIrreversibleShutdown")->GetBool())
	{
		m_iNextState = HostStates_t::HS_RUN;
	}
}

//-----------------------------------------------------------------------------
// Purpose: change multiplayer level
//-----------------------------------------------------------------------------
FORCEINLINE void CHostState::State_ChangeLevelMP(void)
{
	DevMsg(eDLL_T::ENGINE, "%s - Changing multiplayer level to: '%s'\n", "CHostState::State_ChangeLevelMP", m_levelName);
	m_flShortFrameTime = 0.5; // Set frame time.
	g_bLevelResourceInitialized = false;

#ifndef GAMECLIENTONLY
	g_pServerGameDLL->LevelShutdown();
#endif // !GAMECLIENTONLY
	if (CModelLoader_Map_IsValidFn(g_CModelLoader, m_levelName)) // Check if map is valid and if we can start a new game.
	{
#ifndef DEDICATED
		using EnabledProgressBarForNextLoadFn = void(*)(void*);
		(*reinterpret_cast<EnabledProgressBarForNextLoadFn**>(g_pEngineVGui))[31](g_pEngineVGui); // EnabledProgressBarForNextLoad
#endif // !DEDICATED
		Host_ChangelevelFn(false, m_levelName, m_mapGroupName); // Call change level as multiplayer level.
	}
	else
	{
		Error(eDLL_T::ENGINE, "%s - Error: Unable to find map: '%s'\n", "CHostState::State_ChangeLevelMP", m_levelName);
	}

	m_iCurrentState = HostStates_t::HS_RUN; // Set current state to run.

	// If our next state isn't a shutdown or its a forced shutdown then set next state to run.
	if (m_iNextState != HostStates_t::HS_SHUTDOWN || !g_pCVar->FindVar("host_hasIrreversibleShutdown")->GetBool())
	{
		m_iNextState = HostStates_t::HS_RUN;
	}
}

///////////////////////////////////////////////////////////////////////////////
void CHostState_Attach()
{
	DetourAttach((LPVOID*)&CHostState_FrameUpdate, &CHostState::FrameUpdate);
}

void CHostState_Detach()
{
	DetourDetach((LPVOID*)&CHostState_FrameUpdate, &CHostState::FrameUpdate);
}

///////////////////////////////////////////////////////////////////////////////
CHostState* g_pHostState = reinterpret_cast<CHostState*>(p_CHostState_FrameUpdate.FindPatternSelf("48 8D ?? ?? ?? ?? 01", ADDRESS::Direction::DOWN, 100).ResolveRelativeAddressSelf(0x3, 0x7).GetPtr());
