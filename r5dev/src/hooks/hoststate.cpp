#include "pch.h"
#include "hooks.h"

namespace Hooks
{
	FrameUpdateFn originalFrameUpdate = nullptr;
}

void Hooks::FrameUpdate(void* rcx, void* rdx, float time)
{
	static auto setjmpFn = MemoryAddress(0x141205460).RCast<__int64(*)(jmp_buf, void*)>();
	static auto host_abortserver = MemoryAddress(0x14B37C700).RCast<jmp_buf*>();
	static auto CHostState_InitFn = MemoryAddress(0x14023E7D0).RCast<void(*)(CHostState*)>();
	static auto g_ServerAbortServer = MemoryAddress(0x14B37CA22).RCast<char*>();
	static auto State_RunFn = MemoryAddress(0x14023E870).RCast<void(*)(HostStates_t*,void*,float)>();
	static auto Cbuf_ExecuteFn = MemoryAddress(0x14020D5C0).RCast<void(*)()>();
	static auto g_ServerGameClients = MemoryAddress(0x14B383428).RCast<__int64*>();
	static auto SV_InitGameDLLFn = MemoryAddress(0x140308B90).RCast<void(*)()>();
	static auto g_CModelLoader = MemoryAddress(0x14173B210).RCast<void*>();
	static auto CModelLoader_Map_IsValidFn = MemoryAddress(0x1402562F0).RCast<bool(*)(void*, const char*)>();
	static auto Host_NewGameFn = MemoryAddress(0x140238DA0).RCast<bool(*)(char*, char*, bool, bool, void*)>();
	static auto Host_Game_ShutdownFn = MemoryAddress(0x14023EDA0).RCast<void(*)(CHostState*)>();
	static auto src_drawloading = MemoryAddress(0x14B37D96B).RCast<char*>();
	static auto scr_engineevent_loadingstarted = MemoryAddress(0x1666ED024).RCast<char*>();
	static auto gfExtendedError = MemoryAddress(0x14B383391).RCast<char*>();
	static auto g_CEngineVGui = MemoryAddress(0x141741310).RCast<void*>();
	static auto g_ServerDLL = MemoryAddress(0x141732048).RCast<void**>();
	static auto Host_ChangelevelFn = MemoryAddress(0x1402387B0).RCast<void(*)(bool, const char*, const char*)>();

	void* placeHolder = nullptr;
	if (setjmpFn(*host_abortserver, placeHolder))
	{
		CHostState_InitFn(GameGlobals::HostState);
		return;
	}
	else
	{
		*g_ServerAbortServer = true;

		while (true)
		{
			Cbuf_ExecuteFn();
			HostStates_t oldState = GameGlobals::HostState->m_iCurrentState;
			switch (GameGlobals::HostState->m_iCurrentState)
			{
			case HostStates_t::HS_NEW_GAME:
			{
				// Inlined CHostState::State_NewGame
				GameGlobals::HostState->m_bSplitScreenConnect = false;
				if (!g_ServerGameClients) // Init Game if it ain't valid.
				{
					SV_InitGameDLLFn();
				}

				if (!g_ServerGameClients) // SV_InitGameDLL failed its still null.
				{
					std::cout << "Fatal CHostState::FrameUpdate Error. SV_InitGameDLL() failed." << std::endl;
					abort(); // Placeholder.
				}

				if ( !CModelLoader_Map_IsValidFn(g_CModelLoader, GameGlobals::HostState->m_levelName) // Check if map is valid and if we can start a new game.
					|| !Host_NewGameFn(GameGlobals::HostState->m_levelName, nullptr, GameGlobals::HostState->m_bBackgroundLevel, GameGlobals::HostState->m_bSplitScreenConnect, nullptr) )
				{
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
					if (GameGlobals::HostState->m_bActiveGame)
					{
						using GameShutdownFn = void(*)(void*);
						(*reinterpret_cast<GameShutdownFn**>(g_ServerDLL))[9](g_ServerDLL); // (*(void(__fastcall**)(void*))(*(_QWORD*)g_ServerDLL + 72i64))(g_ServerDLL);// GameShutdown
						GameGlobals::HostState->m_bActiveGame = 0;
					} 
					// End Inline CHostState::GameShutdown
				}

			//	Seems useless so nope.
			//	if (g_CHLClient)
			//		(*(void(__fastcall**)(__int64, _QWORD))(*(_QWORD*)g_CHLClient + 1000i64))(g_CHLClient, 0i64);

				GameGlobals::HostState->m_iCurrentState = HostStates_t::HS_RUN; // Set current state to run.

				// If our next state isn't a shutdown or its a forced shutdown then set next state to run.
				if (GameGlobals::HostState->m_iNextState != HostStates_t::HS_SHUTDOWN || !GameGlobals::Cvar->FindVar("host_hasIrreversibleShutdown")->m_iValue)
					GameGlobals::HostState->m_iNextState = HostStates_t::HS_RUN;

				// End Inline CHostState::State_NewGame
				break;
			}
			case HostStates_t::HS_CHANGE_LEVEL_SP:
			{
				GameGlobals::HostState->m_flShortFrameTime = 1.5; // Set frame time.
				if (CModelLoader_Map_IsValidFn(g_CModelLoader, GameGlobals::HostState->m_levelName)) // Check if map is valid and if we can start a new game.
				{
					Host_ChangelevelFn(true, GameGlobals::HostState->m_levelName, GameGlobals::HostState->m_mapGroupName); // Call change level as singleplayer level.
				}
				else
				{
					std::cout << "Fatal CHostState::FrameUpdate Error. Map is not valid.";
				}

				//	Seems useless so nope.
				// 	if (g_CHLClient)
				//		(*(void(__fastcall**)(__int64, _QWORD))(*(_QWORD*)g_CHLClient + 1000i64))(g_CHLClient, 0i64);

				// If our next state isn't a shutdown or its a forced shutdown then set next state to run.
				if (GameGlobals::HostState->m_iNextState != HostStates_t::HS_SHUTDOWN || !GameGlobals::Cvar->FindVar("host_hasIrreversibleShutdown")->m_iValue)
					GameGlobals::HostState->m_iNextState = HostStates_t::HS_RUN;

				break;
			}
			case HostStates_t::HS_CHANGE_LEVEL_MP:
			{
				GameGlobals::HostState->m_flShortFrameTime = 1.5; // Set frame time.
				using LevelShutdownFn = void(__thiscall*)(void*);
				(*reinterpret_cast<LevelShutdownFn**>(*g_ServerDLL))[8](g_ServerDLL); // (*(void (__fastcall **)(void *))(*(_QWORD *)server_dll_var + 64i64))(server_dll_var);// LevelShutdown

				if (CModelLoader_Map_IsValidFn(g_CModelLoader, GameGlobals::HostState->m_levelName)) // Check if map is valid and if we can start a new game.
				{
					using ShowErrorMessageFn = void(*)(void*);
					(*reinterpret_cast<ShowErrorMessageFn**>(g_CEngineVGui))[31](g_CEngineVGui); // (*((void(__fastcall**)(void**))g_CEngineVGUI + 31))(&g_CEngineVGUI);// EnabledProgressBarForNextLoad
					Host_ChangelevelFn(false, GameGlobals::HostState->m_levelName, GameGlobals::HostState->m_mapGroupName); // Call change level as singleplayer level.
				}

				//	Seems useless so nope.
				// // 	if (g_CHLClient)
				//		(*(void(__fastcall**)(__int64, _QWORD))(*(_QWORD*)g_CHLClient + 1000i64))(g_CHLClient, 0i64);

				// If our next state isn't a shutdown or its a forced shutdown then set next state to run.
				if (GameGlobals::HostState->m_iNextState != HostStates_t::HS_SHUTDOWN || !GameGlobals::Cvar->FindVar("host_hasIrreversibleShutdown")->m_iValue)
					GameGlobals::HostState->m_iNextState = HostStates_t::HS_RUN;

				break;
			}
			case HostStates_t::HS_RUN:
			{
 				State_RunFn(&GameGlobals::HostState->m_iCurrentState, nullptr, time);
				break;
			}
			case HostStates_t::HS_GAME_SHUTDOWN:
			{
				Host_Game_ShutdownFn(GameGlobals::HostState);
				break;
			}
			case HostStates_t::HS_RESTART:
			{

				break;
			}
			default:
			{
				break;
			}
			}

			if (oldState == HostStates_t::HS_RUN)
				break;

			if (oldState == HostStates_t::HS_SHUTDOWN || oldState == HostStates_t::HS_RESTART)
				break;

			if (oldState == HostStates_t::HS_GAME_SHUTDOWN)
				break;
		}

	}
//	originalFrameUpdate(rcx, rdx, time);
}