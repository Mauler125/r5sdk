//=============================================================================//
//
// Purpose: Interface the engine exposes to the game DLL
//
//=============================================================================//

#include "core/stdafx.h"
#include "tier0/cvar.h"
#include "engine/sys_utils.h"
#include "server/IVEngineServer.h"
#include "client/client.h"

//-----------------------------------------------------------------------------
// Purpose: sets the persistence var in the CClient instance to 'ready'
//-----------------------------------------------------------------------------
bool HIVEngineServer_PersistenceAvailable(void* entidx, int clientidx)
{
	/* I don't know if permanently setting persistence to ready is a solution
	*  We might wanna re-think that and handle persistence properly by also removing it on client disconnect.
	*  Doing it for when we kick someone or ban someone is already a good start. Though we need to figure out how to handle timeouts?
	*  Maybe NetChan::Shutdown gets called then anyway.
	*  Then we might need to check ReturnAddress to make sure its not a local disconnect due to local server usage.
	*  A lot of things to be considered..
	*  - Pix
	*/
	CClient* pClient = g_pClient->GetClientInstance(clientidx);         // Get client instance.
	*(char*)((std::uintptr_t)pClient + g_dwPersistenceVar) = (char)0x5; // Set the client instance to 'ready'.

	if (!g_bIsPersistenceVarSet[clientidx] && sv_showconnecting->GetBool())
	{
		void* clientNamePtr = (void**)(((std::uintptr_t)pClient->GetNetChan()) + 0x1A8D); // Get client name from netchan.
		std::string clientName((char*)clientNamePtr, 32);                                // Get full name.
		std::int64_t originID = pClient->m_iOriginID;
		std::int64_t clientID = static_cast<std::int64_t>(pClient->m_iUserID + 1);

		std::string ipAddress = "null"; // If this stays null they modified the packet somehow.
		ADDRESS ipAddressField = ADDRESS(((std::uintptr_t)pClient->GetNetChan()) + 0x1AC0); // Get client ip from netchan.
		if (ipAddressField)
		{
			std::stringstream ss;
			ss  << std::to_string(ipAddressField.GetValue<std::uint8_t>()) << "."
				<< std::to_string(ipAddressField.Offset(0x1).GetValue<std::uint8_t>()) << "."
				<< std::to_string(ipAddressField.Offset(0x2).GetValue<std::uint8_t>()) << "."
				<< std::to_string(ipAddressField.Offset(0x3).GetValue<std::uint8_t>());

			ipAddress = ss.str();
		}

		DevMsg(eDLL_T::SERVER, "\n");
		DevMsg(eDLL_T::SERVER, "______________________________________________________________\n");
		DevMsg(eDLL_T::SERVER, "] CLIENT_INSTANCE_DETAILS ------------------------------------\n");
		DevMsg(eDLL_T::SERVER, "] INDEX: | '#%d'\n", clientidx);
		DevMsg(eDLL_T::SERVER, "] NAME : | '%s'\n", clientName.c_str());
		DevMsg(eDLL_T::SERVER, "] OID  : | '%lld'\n", originID);
		DevMsg(eDLL_T::SERVER, "] UID  : | '%lld'\n", clientID);
		DevMsg(eDLL_T::SERVER, "] IPADR: | '%s'\n", ipAddress.c_str());
		DevMsg(eDLL_T::SERVER, "--------------------------------------------------------------\n");
		DevMsg(eDLL_T::SERVER, "\n");

		g_bIsPersistenceVarSet[clientidx] = true;
	}
	///////////////////////////////////////////////////////////////////////////
	return IVEngineServer_PersistenceAvailable(entidx, clientidx);
}

void IVEngineServer_Attach()
{
	DetourAttach((LPVOID*)&IVEngineServer_PersistenceAvailable, &HIVEngineServer_PersistenceAvailable);
}

void IVEngineServer_Detach()
{
	DetourDetach((LPVOID*)&IVEngineServer_PersistenceAvailable, &HIVEngineServer_PersistenceAvailable);
}

///////////////////////////////////////////////////////////////////////////////
bool g_bIsPersistenceVarSet[MAX_PLAYERS];
