//=====================================================================================//
//
// Purpose: Implementation of the pylon server backend.
//
// $NoKeywords: $
//=====================================================================================//

#include <core/stdafx.h>
#include <tier1/cvar.h>
#include <engine/net.h>
#include <engine/host_state.h>
#include <engine/sys_utils.h>
#include <engine/server/server.h>
#include <squirrel/sqinit.h>
#include <networksystem/r5net.h>
#include <networksystem/pylon.h>
#include <public/include/edict.h>

//-----------------------------------------------------------------------------
// Purpose: Send keep alive request to Pylon Master Server.
// NOTE: When Pylon update reaches indev remove this and implement properly.
//-----------------------------------------------------------------------------
void KeepAliveToPylon()
{
#ifndef CLIENT_DLL
	if (g_pHostState->m_bActiveGame && sv_pylonVisibility->GetBool()) // Check for active game.
	{
		std::string m_szHostToken = std::string();
		std::string m_szHostRequestMessage = std::string();

		bool result = g_pR5net->PostServerHost(m_szHostRequestMessage, m_szHostToken,
            NetGameServer_t
            {
                hostname->GetString(),
                "", // description.
                "", // password.
                sv_pylonVisibility->GetInt() == 1,
                g_pHostState->m_levelName,
                mp_gamemode->GetString(),
                hostip->GetString(),
                hostport->GetInt(),
                g_svNetKey.c_str(),
                std::to_string(*g_nServerRemoteChecksum),
                SDK_VERSION,
                "",
                g_pServer->GetNumHumanPlayers() + g_pServer->GetNumFakeClients(),
                g_ServerGlobalVariables->m_nMaxClients
            }
		);
	}
#endif // !CLIENT_DLL
}