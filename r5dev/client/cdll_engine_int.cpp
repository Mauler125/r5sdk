#include "core/stdafx.h"
/*****************************************************************************/
#include "tier0/basetypes.h"
#include "tier0/IConVar.h"
#include "tier0/cvar.h"
#include "client/IVEngineClient.h"
#include "client/client.h"
#include "client/cdll_engine_int.h"
#include "public/include/bansystem.h"
#include "engine/net_chan.h"
#include "vpc/keyvalues.h"
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
				IConVar_ClearHostNames();
				ConCommand_InitConCommand();
				CKeyValueSystem_Init();

				IVEngineClient_CommandExecute(NULL, "exec autoexec.cfg");
				IVEngineClient_CommandExecute(NULL, "exec autoexec_server.cfg");
				IVEngineClient_CommandExecute(NULL, "exec autoexec_client.cfg");

				*(bool*)m_bRestrictServerCommands = true; // Restrict commands.
				ConCommandBase* disconnect = (ConCommandBase*)g_pCvar->FindCommand("disconnect");
				disconnect->AddFlags(FCVAR_SERVER_CAN_EXECUTE); // Make sure server is not restricted to this.

				if (net_userandomkey->m_pParent->m_iValue == 1)
				{
					HNET_GenerateKey();
				}
				g_pCvar->FindVar("net_usesocketsforloopback")->m_pParent->m_iValue = 1;

				bInitialized = true;
			}
			break;
		}
		case ClientFrameStage_t::FRAME_NET_UPDATE_POSTDATAUPDATE_END:
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

						int clientID = g_pClient->m_iUserID + 1; // Get UserID + 1.
						if (clientID != g_pBanSystem->vsvrefuseList[i].second) // See if they match.
						{
							continue;
						}

						NET_DisconnectClient(g_pClient, c, g_pBanSystem->vsvrefuseList[i].first.c_str(), 0, 1);
						g_pBanSystem->DeleteConnectionRefuse(clientID);
						break;
					}
				}
			}
			PatchNetVarConVar();
			break;
		}
		default:
		{
			break;
		}
	}

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
