#include "core/stdafx.h"
#include "engine/sys_utils.h"
#include "engine/host_state.h"
#include "engine/net_chan.h"
#include "tier0/cvar.h"
#include "client/IVEngineClient.h"
#include "networksystem/r5net.h"
#include "squirrel/sqinit.h"

//-----------------------------------------------------------------------------
// Purpose: Send keep alive request to Pylon Master Server.
// NOTE: When Pylon update reaches indev remove this and implement properly.
//-----------------------------------------------------------------------------
void KeepAliveToPylon()
{
	if (g_pHostState->m_bActiveGame && sv_pylonvisibility->m_iValue == 1) // Check for active game.
	{
		std::string m_szHostToken = std::string();
		std::string m_szHostRequestMessage = std::string();
		DevMsg(eDLL_T::CLIENT, "Sending PostServerHost request\n");
		bool result = g_pR5net->PostServerHost(m_szHostRequestMessage, m_szHostToken,
			ServerListing{
				g_pCvar->FindVar("hostname")->m_pzsCurrentValue,
				std::string(g_pHostState->m_levelName),
				"",
				g_pCvar->FindVar("hostport")->m_pzsCurrentValue,
				g_pCvar->FindVar("mp_gamemode")->m_pzsCurrentValue,
				false,
				std::to_string(*g_nRemoteFunctionCallsChecksum), // BUG BUG: Checksum is null on dedi
				std::string(),
				g_szNetKey.c_str()
			}
		);
	}
}

//-----------------------------------------------------------------------------
// Purpose: state machine's main processing loop
//-----------------------------------------------------------------------------
void HCHostState_FrameUpdate(void* rcx, void* rdx, float time)
{
	static auto setjmpFn            = ADDRESS(0x141205460).RCast<std::int64_t(*)(jmp_buf, void*)>();
	static auto host_abortserver    = ADDRESS(0x14B37C700).RCast<jmp_buf*>();
	static auto CHostState_InitFn   = ADDRESS(0x14023E7D0).RCast<void(*)(CHostState*)>();
	static auto g_ServerAbortServer = ADDRESS(0x14B37CA22).RCast<char*>();
	static auto State_RunFn         = ADDRESS(0x14023E870).RCast<void(*)(HostStates_t*, void*, float)>();
	static auto Cbuf_ExecuteFn      = ADDRESS(0x14020D5C0).RCast<void(*)()>();
	static auto g_ServerGameClients = ADDRESS(0x14B383428).RCast<std::int64_t*>();
	static auto SV_InitGameDLLFn    = ADDRESS(0x140308B90).RCast<void(*)()>();
	static auto g_CModelLoader      = ADDRESS(0x14173B210).RCast<void*>();
	static auto CModelLoader_Map_IsValidFn = ADDRESS(0x1402562F0).RCast<bool(*)(void*, const char*)>();
	static auto Host_NewGameFn             = ADDRESS(0x140238DA0).RCast<bool(*)(char*, char*, bool, bool, void*)>();
	static auto Host_Game_ShutdownFn       = ADDRESS(0x14023EDA0).RCast<void(*)(CHostState*)>();
	static auto src_drawloading            = ADDRESS(0x14B37D96B).RCast<char*>();
	static auto scr_engineevent_loadingstarted = ADDRESS(0x1666ED024).RCast<char*>();
	static auto gfExtendedError                = ADDRESS(0x14B383391).RCast<char*>();
	static auto g_CEngineVGui                  = ADDRESS(0x141741310).RCast<void*>();
	static auto g_ServerDLL                    = ADDRESS(0x141732048).RCast<void**>();
	static auto Host_ChangelevelFn             = ADDRESS(0x1402387B0).RCast<void(*)(bool, const char*, const char*)>();
	static auto CL_EndMovieFn                  = ADDRESS(0x1402C03D0).RCast<void(*)()>();
	static auto SendOfflineRequestToStryderFn  = ADDRESS(0x14033D380).RCast<void(*)()>();
	static auto CEngine                        = ADDRESS(0X141741BA0).RCast<void*>();

	static bool bInitialized = false;
	if (!bInitialized)
	{
		IConVar_ClearHostNames();
		ConCommand_InitConCommand();

		IVEngineClient_CommandExecute(NULL, "exec autoexec.cfg");
		IVEngineClient_CommandExecute(NULL, "exec autoexec_server.cfg");
#ifndef DEDICATED
		IVEngineClient_CommandExecute(NULL, "exec autoexec_client.cfg");
#endif // !DEDICATED

		*(bool*)m_bRestrictServerCommands = true; // Restrict commands.
		ConCommandBase* disconnect = (ConCommandBase*)g_pCvar->FindCommand("disconnect");
		disconnect->AddFlags(FCVAR_SERVER_CAN_EXECUTE); // Make sure server is not restricted to this.

		static std::thread PylonThread([]() // Pylon request thread.
		{
				while (true)
				{
					KeepAliveToPylon();
					std::this_thread::sleep_for(std::chrono::milliseconds(5000));
				}
		});

		if (net_userandomkey->m_pParent->m_iValue == 1)
		{
			HNET_GenerateKey();
		}

		g_pCvar->FindVar("net_usesocketsforloopback")->m_pParent->m_iValue = 1;

		bInitialized = true;
	}

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
			Cbuf_ExecuteFn();
			oldState = g_pHostState->m_iCurrentState;
			switch (g_pHostState->m_iCurrentState)
			{
			case HostStates_t::HS_NEW_GAME:
			{
				DevMsg(eDLL_T::ENGINE, "CHostState::FrameUpdate | CASE:HS_NEW_GAME | Loading level: '%s'\n", g_pHostState->m_levelName);

				// Inlined CHostState::State_NewGame
				g_pHostState->m_bSplitScreenConnect = false;
				if (!g_ServerGameClients) // Init Game if it ain't valid.
				{
					SV_InitGameDLLFn();
				}

				if (!CModelLoader_Map_IsValidFn(g_CModelLoader, g_pHostState->m_levelName) // Check if map is valid and if we can start a new game.
					|| !Host_NewGameFn(g_pHostState->m_levelName, nullptr, g_pHostState->m_bBackgroundLevel, g_pHostState->m_bSplitScreenConnect, nullptr) || !g_ServerGameClients)
				{
					DevMsg(eDLL_T::ENGINE, "CHostState::FrameUpdate | CASE:HS_NEW_GAME | Error: Map not valid.\n");
					// Inlined SCR_EndLoadingPlaque
					if (*src_drawloading)
					{
						*scr_engineevent_loadingstarted = 0;
						using HideLoadingPlaqueFn = void(*)(void*);
						(*reinterpret_cast<HideLoadingPlaqueFn**>(g_CEngineVGui))[36](g_CEngineVGui); // (*((void(__fastcall**)(void**))g_CEngineVGui + 36))(&g_CEngineVGui);// HideLoadingPlaque
					}
					else if (*gfExtendedError)
					{
						using ShowErrorMessageFn = void(*)(void*);
						(*reinterpret_cast<ShowErrorMessageFn**>(g_CEngineVGui))[35](g_CEngineVGui); // (*((void(__fastcall**)(void**))g_CEngineVGui + 35))(&g_CEngineVGui);// ShowErrorMessage
					}
					// End Inline SCR_EndLoadingPlaque

					// Inlined CHostState::GameShutdown
					if (g_pHostState->m_bActiveGame)
					{
						using GameShutdownFn = void(*)(void*);
						(*reinterpret_cast<GameShutdownFn**>(g_ServerDLL))[9](g_ServerDLL); // (*(void(__fastcall**)(void*))(*(_QWORD*)g_ServerDLL + 72i64))(g_ServerDLL);// GameShutdown
						g_pHostState->m_bActiveGame = 0;
					}
					// End Inline CHostState::GameShutdown
				}

				//	Seems useless so nope.
				//	if (g_CHLClient)
				//		(*(void(__fastcall**)(__int64, _QWORD))(*(_QWORD*)g_CHLClient + 1000i64))(g_CHLClient, 0i64);

				g_pHostState->m_iCurrentState = HostStates_t::HS_RUN; // Set current state to run.

				// If our next state isn't a shutdown or its a forced shutdown then set next state to run.
				if (g_pHostState->m_iNextState != HostStates_t::HS_SHUTDOWN || !g_pCvar->FindVar("host_hasIrreversibleShutdown")->m_pParent->m_iValue)
				{
					g_pHostState->m_iNextState = HostStates_t::HS_RUN;
				}

				// End Inline CHostState::State_NewGame
				break;
			}
			case HostStates_t::HS_CHANGE_LEVEL_SP:
			{
				g_pHostState->m_flShortFrameTime = 1.5; // Set frame time.

				DevMsg(eDLL_T::ENGINE, "CHostState::FrameUpdate | CASE:HS_CHANGE_LEVEL_SP | Changing singleplayer level to: '%s'\n", g_pHostState->m_levelName);

				if (CModelLoader_Map_IsValidFn(g_CModelLoader, g_pHostState->m_levelName)) // Check if map is valid and if we can start a new game.
				{
					Host_ChangelevelFn(true, g_pHostState->m_levelName, g_pHostState->m_mapGroupName); // Call change level as singleplayer level.
				}
				else
				{
					DevMsg(eDLL_T::ENGINE, "CHostState::FrameUpdate | CASE:HS_CHANGE_LEVEL_SP | Error: Unable to find map: '%s'\n", g_pHostState->m_levelName);
				}

				//	Seems useless so nope.
				// 	if (g_CHLClient)
				//		(*(void(__fastcall**)(__int64, _QWORD))(*(_QWORD*)g_CHLClient + 1000i64))(g_CHLClient, 0i64);

				g_pHostState->m_iCurrentState = HostStates_t::HS_RUN; // Set current state to run.

				// If our next state isn't a shutdown or its a forced shutdown then set next state to run.
				if (g_pHostState->m_iNextState != HostStates_t::HS_SHUTDOWN || !g_pCvar->FindVar("host_hasIrreversibleShutdown")->m_pParent->m_iValue)
				{
					g_pHostState->m_iNextState = HostStates_t::HS_RUN;
				}

				break;
			}
			case HostStates_t::HS_CHANGE_LEVEL_MP:
			{
				g_pHostState->m_flShortFrameTime = 0.5; // Set frame time.
				using LevelShutdownFn = void(__thiscall*)(void*);
				(*reinterpret_cast<LevelShutdownFn**>(*g_ServerDLL))[8](g_ServerDLL); // (*(void (__fastcall **)(void *))(*(_QWORD *)server_dll_var + 64i64))(server_dll_var);// LevelShutdown

				DevMsg(eDLL_T::ENGINE, "CHostState::FrameUpdate | CASE:HS_CHANGE_LEVEL_MP | Changing multiplayer level to: '%s'\n", g_pHostState->m_levelName);

				if (CModelLoader_Map_IsValidFn(g_CModelLoader, g_pHostState->m_levelName)) // Check if map is valid and if we can start a new game.
				{
					using EnabledProgressBarForNextLoadFn = void(*)(void*);
					(*reinterpret_cast<EnabledProgressBarForNextLoadFn**>(g_CEngineVGui))[31](g_CEngineVGui); // (*((void(__fastcall**)(void**))g_CEngineVGUI + 31))(&g_CEngineVGUI);// EnabledProgressBarForNextLoad
					Host_ChangelevelFn(false, g_pHostState->m_levelName, g_pHostState->m_mapGroupName); // Call change level as multiplayer level.
				}
				else
				{
					DevMsg(eDLL_T::ENGINE, "CHostState::FrameUpdate | CASE:HS_CHANGE_LEVEL_MP | Error: Unable to find map: '%s'\n", g_pHostState->m_levelName);
				}

				//	Seems useless so nope.
				// // 	if (g_CHLClient)
				//		(*(void(__fastcall**)(__int64, _QWORD))(*(_QWORD*)g_CHLClient + 1000i64))(g_CHLClient, 0i64);

				g_pHostState->m_iCurrentState = HostStates_t::HS_RUN; // Set current state to run.

				// If our next state isn't a shutdown or its a forced shutdown then set next state to run.
				if (g_pHostState->m_iNextState != HostStates_t::HS_SHUTDOWN || !g_pCvar->FindVar("host_hasIrreversibleShutdown")->m_pParent->m_iValue)
				{
					g_pHostState->m_iNextState = HostStates_t::HS_RUN;
				}

				break;
			}
			case HostStates_t::HS_RUN:
			{
				State_RunFn(&g_pHostState->m_iCurrentState, nullptr, time);
				break;
			}
			case HostStates_t::HS_GAME_SHUTDOWN:
			{
				DevMsg(eDLL_T::ENGINE, "CHostState::FrameUpdate | CASE:HS_GAME_SHUTDOWN | Shutdown game\n");
				Host_Game_ShutdownFn(g_pHostState);
				break;
			}
			case HostStates_t::HS_RESTART:
			{
				DevMsg(eDLL_T::ENGINE, "CHostState::FrameUpdate | CASE:HS_RESTART | Restarting client\n");
				CL_EndMovieFn();
				SendOfflineRequestToStryderFn(); // We have hostnames nulled anyway.
				*(std::int32_t*)((std::uintptr_t)CEngine + 0xC) = 3; //g_CEngine.vtable->SetNextState(&g_CEngine, DLL_RESTART);
				break;
			}
			case HostStates_t::HS_SHUTDOWN:
			{
				DevMsg(eDLL_T::ENGINE, "CHostState::FrameUpdate | CASE:HS_SHUTDOWN | Shutdown client\n");
				CL_EndMovieFn();
				SendOfflineRequestToStryderFn(); // We have hostnames nulled anyway.
				*(std::int32_t*)((std::uintptr_t)CEngine + 0xC) = 2; //g_CEngine.vtable->SetNextState(&g_CEngine, DLL_CLOSE);
				break;
			}
			default:
			{
				break;
			}
			}

		} while ((oldState != HostStates_t::HS_RUN || g_pHostState->m_iNextState == HostStates_t::HS_LOAD_GAME && g_pCvar->FindVar("g_single_frame_shutdown_for_reload_cvar")->m_pParent->m_iValue)
			&& oldState != HostStates_t::HS_SHUTDOWN
			&& oldState != HostStates_t::HS_RESTART);

	}
}

void CHostState_Attach()
{
	DetourAttach((LPVOID*)&CHostState_FrameUpdate, &HCHostState_FrameUpdate);
}

void CHostState_Detach()
{
	DetourDetach((LPVOID*)&CHostState_FrameUpdate, &HCHostState_FrameUpdate);
}

CHostState* g_pHostState = reinterpret_cast<CHostState*>(p_CHostState_FrameUpdate.FindPatternSelf("48 8D ?? ?? ?? ?? 01", ADDRESS::Direction::DOWN, 100).ResolveRelativeAddressSelf(0x3, 0x7).GetPtr());;
