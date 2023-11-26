//=============================================================================//
//
// Purpose: Interface the engine exposes to the game DLL
//
//=============================================================================//

#include "vengineserver_impl.h"

//-----------------------------------------------------------------------------
// Purpose: sets the persistence var in the CClient instance to 'ready'
//-----------------------------------------------------------------------------
bool CVEngineServer::PersistenceAvailable(void* entidx, int clientidx)
{
	///////////////////////////////////////////////////////////////////////////
	return IVEngineServer__PersistenceAvailable(entidx, clientidx);
}

void HVEngineServer::Detour(const bool bAttach) const
{
	DetourSetup(&IVEngineServer__PersistenceAvailable, &CVEngineServer::PersistenceAvailable, bAttach);
}

///////////////////////////////////////////////////////////////////////////////
ServerPlayer_t g_ServerPlayer[MAX_PLAYERS];

IVEngineServer* g_pEngineServerVFTable = nullptr;
CVEngineServer* g_pEngineServer = reinterpret_cast<CVEngineServer*>(&g_pEngineServerVFTable);