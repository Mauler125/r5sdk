//=============================================================================//
//
// Purpose: Interface the engine exposes to the game DLL
//
//=============================================================================//

#include "core/stdafx.h"
#include "tier1/cvar.h"
#include "common/protocol.h"
#include "engine/client/client.h"
#include "vengineserver_impl.h"

//-----------------------------------------------------------------------------
// Purpose: sets the persistence var in the CClient instance to 'ready'
//-----------------------------------------------------------------------------
bool CVEngineServer::PersistenceAvailable(void* entidx, int clientidx)
{
	CClient* pClient = g_pClient->GetClient(clientidx);           // Get client instance.
	pClient->SetPersistenceState(PERSISTENCE::PERSISTENCE_READY); // Set the client instance to 'ready'.

	if (!g_ServerPlayer[clientidx].m_bPersistenceEnabled && sv_showconnecting->GetBool())
	{
		g_ServerPlayer[clientidx].m_bPersistenceEnabled = true;
		CNetChan* pNetChan = pClient->GetNetChan();

		DevMsg(eDLL_T::SERVER, "Enabled persistence for client #%d; channel %s(%s) ('%llu')\n",
			clientidx, pNetChan->GetName(), pNetChan->GetAddress(), pClient->GetNucleusID());
	}
	///////////////////////////////////////////////////////////////////////////
	return IVEngineServer__PersistenceAvailable(entidx, clientidx);
}

void HVEngineServer::Attach() const
{
	DetourAttach(&IVEngineServer__PersistenceAvailable, &CVEngineServer::PersistenceAvailable);
}

void HVEngineServer::Detach() const
{
	DetourDetach(&IVEngineServer__PersistenceAvailable, &CVEngineServer::PersistenceAvailable);
}

///////////////////////////////////////////////////////////////////////////////
ServerPlayer_t g_ServerPlayer[MAX_PLAYERS];

IVEngineServer* g_pEngineServerVFTable = nullptr;
CVEngineServer* g_pEngineServer = reinterpret_cast<CVEngineServer*>(&g_pEngineServerVFTable);