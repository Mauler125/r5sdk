#include "core/stdafx.h"
/*****************************************************************************/
#include "tier0/IConVar.h"
#include "tier0/cvar.h"
#include "tier0/commandline.h"
#include "client/vengineclient_impl.h"
#include "client/cdll_engine_int.h"
#include "engine/net_chan.h"
#include "engine/cl_rcon.h"
#include "public/include/bansystem.h"
#include "vpc/keyvalues.h"
#include "gameui/IConsole.h"
/*****************************************************************************/

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void __fastcall HFrameStageNotify(CHLClient* rcx, ClientFrameStage_t frameStage)
{
	switch (frameStage)
	{
		case ClientFrameStage_t::FRAME_START: // FrameStageNotify gets called every frame by CEngine::Frame with the stage being FRAME_START. We can use this to check/set global variables.
		{
			static bool bInitialized = false;
			if (!bInitialized)
			{
#ifdef GAMEDLL_S3
				g_pConVar->ClearHostNames();
#endif // GAMEDLL_S3
				CKeyValueSystem_Init();
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1) || defined (GAMEDLL_S2) // !TEMP UNTIL CHOSTSTATE IS BUILD AGNOSTIC! //
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

				*(bool*)m_bRestrictServerCommands = true; // Restrict commands.
				ConCommandBase* disconnect = (ConCommandBase*)g_pCVar->FindCommand("disconnect");
				disconnect->AddFlags(FCVAR_SERVER_CAN_EXECUTE); // Make sure server is not restricted to this.

				if (net_userandomkey->GetBool())
				{
					HNET_GenerateKey();
				}
				g_pCVar->FindVar("net_usesocketsforloopback")->SetValue(1);
				g_pRConClient->Init();
#endif // GAMEDLL_S0 || GAMEDLL_S1 || GAMEDLL_S2
				bInitialized = true;
			}
			break;
		}
		case ClientFrameStage_t::FRAME_NET_UPDATE_POSTDATAUPDATE_END:
		{
			g_pBanSystem->BanListCheck();
			PatchNetVarConVar();
			break;
		}
		default:
		{
			break;
		}
	}
	g_pIConsole->Think();
	g_pRConClient->RunFrame();
	CHLClient_FrameStageNotify(rcx, (int)frameStage);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PatchNetVarConVar()
{
	CHAR sConvarPtr[] = "\x72\x3a\x73\x76\x72\x75\x73\x7a\x7a\x03\x04";
	PCHAR curr = sConvarPtr;
	while (*curr)
	{
		*curr ^= 'B';
		++curr;
	}

	std::int64_t nCvarAddr = 0;
	std::stringstream ss;
	ss << std::hex << std::string(sConvarPtr);
	ss >> nCvarAddr;
	void* pCvar = reinterpret_cast<void*>(nCvarAddr);

	if (*reinterpret_cast<std::uint8_t*>(pCvar) == 144)
	{
		std::uint8_t padding[] =
		{
			0x48, 0x8B, 0x45, 0x58, 0xC7, 0x00, 0x00, 0x00, 0x00, 0x00
		};

		void* pCallback = nullptr;
		VirtualAlloc(pCallback, 10, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		memcpy(pCallback, (void*)padding, 9);
		reinterpret_cast<void(*)()>(pCallback)();
	}
}

///////////////////////////////////////////////////////////////////////////////
void CHLClient_Attach()
{
	DetourAttach((LPVOID*)&CHLClient_FrameStageNotify, &HFrameStageNotify);
}

void CHLClient_Detach()
{
	DetourDetach((LPVOID*)&CHLClient_FrameStageNotify, &HFrameStageNotify);
}
