//=============================================================================//
//
// Purpose: Runs the state machine for the host & server.
//
//=============================================================================//

#include "core/stdafx.h"
#include "tier0/cmd.h"
#include "tier0/cvar.h"
#include "tier0/commandline.h"
#include "tier1/NetAdr2.h"
#include "tier2/socketcreator.h"
#ifdef DEDICATED
#include "engine/sv_rcon.h"
#else // 
#include "engine/cl_rcon.h"
#endif // DEDICATED
#include "engine/sys_engine.h"
#include "engine/sys_utils.h"
#include "engine/host_state.h"
#include "engine/net_chan.h"
#include "engine/gl_screen.h"
#ifndef DEDICATED
#include "vgui/vgui_baseui_interface.h"
#endif // DEDICATED
#include "client/IVEngineClient.h"
#include "networksystem/pylon.h"
#include "public/include/bansystem.h"
#include "game/server/gameinterface.h"

//-----------------------------------------------------------------------------
// Purpose: state machine's main processing loop
//-----------------------------------------------------------------------------
FORCEINLINE void CHostState::FrameUpdate(void* rcx, void* rdx, float time)
{
	static bool bInitialized = false;
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
		*g_ServerAbortServer = true;

		do
		{
			Cbuf_Execute();
			oldState = g_pHostState->m_iCurrentState;

			switch (g_pHostState->m_iCurrentState)
			{
			case HostStates_t::HS_NEW_GAME:
			{
				DevMsg(eDLL_T::ENGINE, "Loading level: '%s'\n", g_pHostState->m_levelName);

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
				DevMsg(eDLL_T::ENGINE, "Shutdown host game\n");
				Host_Game_ShutdownFn(g_pHostState);
				break;
			}
			case HostStates_t::HS_RESTART:
			{
				DevMsg(eDLL_T::ENGINE, "Restarting state machine\n");
#ifndef DEDICATED
				CL_EndMovieFn();
#endif // !DEDICATED
				SendOfflineRequestToStryderFn(); // We have hostnames nulled anyway.
				g_pEngine->SetNextState(EngineState_t::DLL_RESTART);
				break;
			}
			case HostStates_t::HS_SHUTDOWN:
			{
				DevMsg(eDLL_T::ENGINE, "Shutdown state machine\n");
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

		} while ((oldState != HostStates_t::HS_RUN || g_pHostState->m_iNextState == HostStates_t::HS_LOAD_GAME && g_pCVar->FindVar("single_frame_shutdown_for_reload")->GetBool())
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

	*(bool*)m_bRestrictServerCommands = true; // Restrict commands.
	ConCommandBase* disconnect = (ConCommandBase*)g_pCVar->FindCommand("disconnect");
	disconnect->AddFlags(FCVAR_SERVER_CAN_EXECUTE); // Make sure server is not restricted to this.

	static std::thread PylonThread([]() // Pylon request thread.
		{
			while (true)
			{
				KeepAliveToPylon();
				std::this_thread::sleep_for(std::chrono::milliseconds(5000));
			}
		});

	static std::thread BanlistThread([]()
		{
			while (true)
			{
				g_pBanSystem->BanListCheck();
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			}
		});

	if (net_userandomkey->GetBool())
	{
		HNET_GenerateKey();
	}

	g_pCVar->FindVar("net_usesocketsforloopback")->SetValue(1);
}

//-----------------------------------------------------------------------------
// Purpose: load and execute configuration files
//-----------------------------------------------------------------------------
FORCEINLINE void CHostState::LoadConfig(void) const
{
	if (!g_pCmdLine->CheckParm("-devsdk"))
	{
		IVEngineClient_CommandExecute(NULL, "exec \"autoexec_server.cfg\"");
		IVEngineClient_CommandExecute(NULL, "exec \"rcon_server.cfg\"");
#ifndef DEDICATED
		IVEngineClient_CommandExecute(NULL, "exec \"autoexec_client.cfg\"");
		IVEngineClient_CommandExecute(NULL, "exec \"rcon_client.cfg\"");
#endif // !DEDICATED
		IVEngineClient_CommandExecute(NULL, "exec \"autoexec.cfg\"");
	}
	else // Development configs.
	{
		IVEngineClient_CommandExecute(NULL, "exec \"autoexec_server_dev.cfg\"");
		IVEngineClient_CommandExecute(NULL, "exec \"rcon_server_dev.cfg\"");
#ifndef DEDICATED
		IVEngineClient_CommandExecute(NULL, "exec \"autoexec_client_dev.cfg\"");
		IVEngineClient_CommandExecute(NULL, "exec \"rcon_client_dev.cfg\"");
#endif // !DEDICATED
		IVEngineClient_CommandExecute(NULL, "exec \"autoexec_dev.cfg\"");
	}
}

//-----------------------------------------------------------------------------
// Purpose: initialize new game
//-----------------------------------------------------------------------------
FORCEINLINE void CHostState::State_NewGame(void)
{
	m_bSplitScreenConnect = false;
	if (!g_ServerGameClients) // Init Game if it ain't valid.
	{
		SV_InitGameDLLFn();
	}

	if (!CModelLoader_Map_IsValidFn(g_CModelLoader, m_levelName) // Check if map is valid and if we can start a new game.
		|| !Host_NewGameFn(m_levelName, nullptr, m_bBackgroundLevel, m_bSplitScreenConnect, nullptr) || !g_ServerGameClients)
	{
		DevMsg(eDLL_T::ENGINE, "Error: Map not valid\n");
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
// Purpose: shutdown active game
//-----------------------------------------------------------------------------
FORCEINLINE void CHostState::GameShutDown(void)
{
	if (m_bActiveGame)
	{
		g_pServerGameDLL->GameShutdown();
		m_bActiveGame = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: change singleplayer level
//-----------------------------------------------------------------------------
FORCEINLINE void CHostState::State_ChangeLevelSP(void)
{
	DevMsg(eDLL_T::ENGINE, "Changing singleplayer level to: '%s'\n", m_levelName);
	m_flShortFrameTime = 1.5; // Set frame time.

	if (CModelLoader_Map_IsValidFn(g_CModelLoader, m_levelName)) // Check if map is valid and if we can start a new game.
	{
		Host_ChangelevelFn(true, m_levelName, m_mapGroupName); // Call change level as singleplayer level.
	}
	else
	{
		DevMsg(eDLL_T::ENGINE, "Error: Unable to find map: '%s'\n", m_levelName);
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
	DevMsg(eDLL_T::ENGINE, "Changing multiplayer level to: '%s'\n", m_levelName);
	m_flShortFrameTime = 0.5; // Set frame time.

	g_pServerGameDLL->LevelShutdown();
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
		DevMsg(eDLL_T::ENGINE, "Error: Unable to find map: '%s'\n", m_levelName);
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
CHostState* g_pHostState = reinterpret_cast<CHostState*>(p_CHostState_FrameUpdate.FindPatternSelf("48 8D ?? ?? ?? ?? 01", ADDRESS::Direction::DOWN, 100).ResolveRelativeAddressSelf(0x3, 0x7).GetPtr());;
