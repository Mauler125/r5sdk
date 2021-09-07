#include "pch.h"
#include "hooks.h"

namespace Hooks
{
	FrameStageNotifyFn originalFrameStageNotify = nullptr;
}

void __fastcall Hooks::FrameStageNotify(CHLClient* rcx, ClientFrameStage_t curStage)
{
	switch (curStage)
	{
	case FRAME_START: // FrameStageNotify gets called every frame by CEngine::Frame with the stage being FRAME_START. We can use this to check/set global variables.
	{
		if (!GameGlobals::IsInitialized)
			GameGlobals::InitGameGlobals();

		break;
	}
	case FRAME_NET_UPDATE_POSTDATAUPDATE_END:
	{
		if (GameGlobals::BanSystem->IsRefuseListValid())
		{
			for (int i = 0; i < GameGlobals::BanSystem->refuseList.size(); i++) // Loop through vector.
			{
				for (int c = 0; c < MAX_PLAYERS; c++) // Loop through all possible client instances.
				{
					CClient* client = GameGlobals::Client->GetClientInstance(c); // Get client instance.
					if (!client)
						continue;

					if (!client->GetNetChan()) // Netchan valid?
						continue;

					int clientID = client->m_iUserID + 1; // Get UserID + 1.
					if (clientID != GameGlobals::BanSystem->refuseList[i].second) // See if they match.
						continue;

					GameGlobals::DisconnectClient(client, GameGlobals::BanSystem->refuseList[i].first.c_str(), 0, 1);
					GameGlobals::BanSystem->DeleteConnectionRefuse(clientID);
					break;
				}
			}
		}
		PatchNetVarConVar();
		break;
	}
	default:
		break;
	}
	originalFrameStageNotify(rcx, curStage);
}