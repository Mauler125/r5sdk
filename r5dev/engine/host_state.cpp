//=============================================================================//
//
// Purpose: Runs the state machine for the host & server
//
//=============================================================================//

#include "core/stdafx.h"
#include "tier0/cvar.h"
#include "tier0/commandline.h"
#include "tier1/NetAdr2.h"
#include "tier2/socketcreator.h"
#include "engine/sys_utils.h"
#include "engine/host_state.h"
#include "engine/net_chan.h"
#include "client/IVEngineClient.h"
#include "networksystem/r5net.h"
#include "squirrel/sqinit.h"
#include "public/include/bansystem.h"
#include "engine/sys_engine.h"
#include "engine/sv_rcon.h"

//-----------------------------------------------------------------------------
// Purpose: Send keep alive request to Pylon Master Server.
// NOTE: When Pylon update reaches indev remove this and implement properly.
//-----------------------------------------------------------------------------
void KeepAliveToPylon()
{
	if (g_pHostState->m_bActiveGame && sv_pylonvisibility->GetBool()) // Check for active game.
	{
		std::string m_szHostToken = std::string();
		std::string m_szHostRequestMessage = std::string();
		DevMsg(eDLL_T::CLIENT, "Sending PostServerHost request\n");
		bool result = g_pR5net->PostServerHost(m_szHostRequestMessage, m_szHostToken,
			ServerListing{
				g_pCVar->FindVar("hostname")->GetString(),
				std::string(g_pHostState->m_levelName),
				"",
				g_pCVar->FindVar("hostport")->GetString(),
				g_pCVar->FindVar("mp_gamemode")->GetString(),
				false,
				std::to_string(*g_nServerRemoteChecksum),
				std::string(),
				g_szNetKey.c_str()
			}
		);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Check refuse list and kill netchan connection.
//-----------------------------------------------------------------------------
void BanListCheck()
{
	if (g_pBanSystem->IsRefuseListValid())
	{
		for (int i = 0; i < g_pBanSystem->vsvrefuseList.size(); i++) // Loop through vector.
		{
			for (int c = 0; c < MAX_PLAYERS; c++) // Loop through all possible client instances.
			{
				CClient* client = g_pClient->GetClientInstance(c); // Get client instance.
				if (!client)
				{
					continue;
				}

				if (!client->GetNetChan()) // Netchan valid?
				{
					continue;
				}

				if (g_pClient->m_iOriginID != g_pBanSystem->vsvrefuseList[i].second) // See if nucleus id matches entry.
				{
					continue;
				}

				std::string finalIpAddress = std::string();
				ADDRESS ipAddressField = ADDRESS(((std::uintptr_t)client->GetNetChan()) + 0x1AC0); // Get client ip from netchan.
				if (ipAddressField && ipAddressField.GetValue<int>() != 0x0)
				{
					std::stringstream ss;
					ss << std::to_string(ipAddressField.GetValue<std::uint8_t>()) << "."
						<< std::to_string(ipAddressField.Offset(0x1).GetValue<std::uint8_t>()) << "."
						<< std::to_string(ipAddressField.Offset(0x2).GetValue<std::uint8_t>()) << "."
						<< std::to_string(ipAddressField.Offset(0x3).GetValue<std::uint8_t>());

					finalIpAddress = ss.str();
				}

				DevMsg(eDLL_T::SERVER, "\n");
				DevMsg(eDLL_T::SERVER, "______________________________________________________________\n");
				DevMsg(eDLL_T::SERVER, "] PYLON NOTICE -----------------------------------------------\n");
				DevMsg(eDLL_T::SERVER, "] OriginID : | '%lld' IS GETTING DISCONNECTED.\n", g_pClient->m_iOriginID);
				if (finalIpAddress.empty())
					DevMsg(eDLL_T::SERVER, "] IP-ADDR  : | CLIENT MODIFIED PACKET.\n");
				else
					DevMsg(eDLL_T::SERVER, "] IP-ADDR  : | '%s'\n", finalIpAddress.c_str());
				DevMsg(eDLL_T::SERVER, "--------------------------------------------------------------\n");
				DevMsg(eDLL_T::SERVER, "\n");

				g_pBanSystem->AddEntry(finalIpAddress, g_pClient->m_iOriginID); // Add local entry to reserve a non needed request.
				g_pBanSystem->Save(); // Save list.
				NET_DisconnectClient(g_pClient, c, g_pBanSystem->vsvrefuseList[i].first.c_str(), 0, 1); // Disconnect client.
			}
		}
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

	static bool bInitialized = false;
	if (!bInitialized)
	{
		g_pRConServer = new CRConServer();

		if (!g_pCmdLine->CheckParm("-devsdk"))
		{
			IVEngineClient_CommandExecute(NULL, "exec autoexec_server.cfg");
			IVEngineClient_CommandExecute(NULL, "exec rcon_server.cfg");
#ifndef DEDICATED
			IVEngineClient_CommandExecute(NULL, "exec autoexec_client.cfg");
			IVEngineClient_CommandExecute(NULL, "exec rcon_client.cfg");
#endif // !DEDICATED
			IVEngineClient_CommandExecute(NULL, "exec autoexec.cfg");
		}
		else // Development configs.
		{
			IVEngineClient_CommandExecute(NULL, "exec autoexec_server_dev.cfg");
			IVEngineClient_CommandExecute(NULL, "exec rcon_server_dev.cfg");
#ifndef DEDICATED
			IVEngineClient_CommandExecute(NULL, "exec autoexec_client_dev.cfg");
			IVEngineClient_CommandExecute(NULL, "exec rcon_client_dev.cfg");
#endif // !DEDICATED
			IVEngineClient_CommandExecute(NULL, "exec autoexec_dev.cfg");
		}

		g_pConVar->ClearHostNames();
		g_pRConServer->Init();

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
					BanListCheck();
					std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				}
		});

		if (net_userandomkey->GetBool())
		{
			HNET_GenerateKey();
		}

		g_pCVar->FindVar("net_usesocketsforloopback")->SetValue(1);

		bInitialized = true;
	}

	g_pRConServer->RunFrame();

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
				if (g_pHostState->m_iNextState != HostStates_t::HS_SHUTDOWN || !g_pCVar->FindVar("host_hasIrreversibleShutdown")->GetBool())
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
				if (g_pHostState->m_iNextState != HostStates_t::HS_SHUTDOWN || !g_pCVar->FindVar("host_hasIrreversibleShutdown")->GetBool())
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
				if (g_pHostState->m_iNextState != HostStates_t::HS_SHUTDOWN || !g_pCVar->FindVar("host_hasIrreversibleShutdown")->GetBool())
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
				g_pEngine->SetNextState(EngineState_t::DLL_RESTART);
				break;
			}
			case HostStates_t::HS_SHUTDOWN:
			{
				DevMsg(eDLL_T::ENGINE, "CHostState::FrameUpdate | CASE:HS_SHUTDOWN | Shutdown client\n");
				CL_EndMovieFn();
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

void CHostState_Attach()
{
	DetourAttach((LPVOID*)&CHostState_FrameUpdate, &HCHostState_FrameUpdate);
}

void CHostState_Detach()
{
	DetourDetach((LPVOID*)&CHostState_FrameUpdate, &HCHostState_FrameUpdate);
}

CHostState* g_pHostState = reinterpret_cast<CHostState*>(p_CHostState_FrameUpdate.FindPatternSelf("48 8D ?? ?? ?? ?? 01", ADDRESS::Direction::DOWN, 100).ResolveRelativeAddressSelf(0x3, 0x7).GetPtr());;
