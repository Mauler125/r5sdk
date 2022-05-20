//===============================================================================//
//
// Purpose: 
//
// $NoKeywords: $
//
//===============================================================================//
// client.cpp: implementation of the CClient class.
//
///////////////////////////////////////////////////////////////////////////////////
#include "core/stdafx.h"
#include "engine/client/client.h"
#include "engine/server/server.h"

//---------------------------------------------------------------------------------
// Purpose: gets the client from buffer by index
//---------------------------------------------------------------------------------
CClient* CClient::GetClient(int nIndex) const
{
	return (CClient*)(std::uintptr_t)(g_pClientBuffer.GetPtr() + (nIndex * sizeof(CClient)));
}

//---------------------------------------------------------------------------------
// Purpose: gets the userID of this client
//---------------------------------------------------------------------------------
std::int32_t CClient::GetUserID(void) const
{
	return m_nUserID;
}

//---------------------------------------------------------------------------------
// Purpose: gets the userID of this client
//---------------------------------------------------------------------------------
std::int64_t CClient::GetOriginID(void) const
{
	return m_nOriginID;
}

//---------------------------------------------------------------------------------
// Purpose: gets the signon state of this client
//---------------------------------------------------------------------------------
SIGNONSTATE CClient::GetSignonState(void) const
{
	return m_nSignonState;
}

//---------------------------------------------------------------------------------
// Purpose: gets the persistence state of this client
//---------------------------------------------------------------------------------
PERSISTENCE CClient::GetPersistenceState(void) const
{
	return m_nPersistenceState;
}

//---------------------------------------------------------------------------------
// Purpose: gets the net channel of this client
//---------------------------------------------------------------------------------
CNetChan* CClient::GetNetChan(void) const
{
	return m_NetChannel;
}

//---------------------------------------------------------------------------------
// Purpose: sets the userID of this client
//---------------------------------------------------------------------------------
void CClient::SetUserID(std::int32_t nUserID)
{
	m_nUserID = nUserID;
}

//---------------------------------------------------------------------------------
// Purpose: sets the originID of this client
//---------------------------------------------------------------------------------
void CClient::SetOriginID(std::int64_t nOriginID)
{
	m_nOriginID = nOriginID;
}

//---------------------------------------------------------------------------------
// Purpose: sets the signon state of this client
//---------------------------------------------------------------------------------
void CClient::SetSignonState(SIGNONSTATE nSignonState)
{
	m_nSignonState = nSignonState;
}

//---------------------------------------------------------------------------------
// Purpose: sets the persistence state of this client
//---------------------------------------------------------------------------------
void CClient::SetPersistenceState(PERSISTENCE nPersistenceState)
{
	m_nPersistenceState = nPersistenceState;
}

//---------------------------------------------------------------------------------
// Purpose: sets the net channel of this client
// !TODO  : Remove this and rebuild INetChannel
//---------------------------------------------------------------------------------
void CClient::SetNetChan(CNetChan* pNetChan)
{
	m_NetChannel = pNetChan;
}

//---------------------------------------------------------------------------------
// Purpose: checks if client is connected to server
// Output : true if connected, false otherwise
//---------------------------------------------------------------------------------
bool CClient::IsConnected(void) const
{
	return m_nSignonState >= SIGNONSTATE::SIGNONSTATE_CONNECTED;
}

//---------------------------------------------------------------------------------
// Purpose: checks if client is spawned to server
// Output : true if connected, false otherwise
//---------------------------------------------------------------------------------
bool CClient::IsSpawned(void) const
{
	return m_nSignonState >= SIGNONSTATE::SIGNONSTATE_NEW;
}

//---------------------------------------------------------------------------------
// Purpose: checks if client is active to server
// Output : true if connected, false otherwise
//---------------------------------------------------------------------------------
bool CClient::IsActive(void) const
{
	return m_nSignonState == SIGNONSTATE::SIGNONSTATE_FULL;
}

//---------------------------------------------------------------------------------
// Purpose: checks if client's persistence data is available
// Output : true if available, false otherwise
//---------------------------------------------------------------------------------
bool CClient::IsPersistenceAvailable(void) const
{
	return m_nPersistenceState >= PERSISTENCE::PERSISTENCE_AVAILABLE;
}

//---------------------------------------------------------------------------------
// Purpose: checks if client's persistence data is ready
// Output : true if ready, false otherwise
//---------------------------------------------------------------------------------
bool CClient::IsPersistenceReady(void) const
{
	return m_nPersistenceState == PERSISTENCE::PERSISTENCE_READY;
}

//---------------------------------------------------------------------------------
// Purpose: checks if client is a fake client
// Output : true if connected, false otherwise
//---------------------------------------------------------------------------------
bool CClient::IsFakeClient(void) const
{
	return m_bFakePlayer;
}

//---------------------------------------------------------------------------------
// Purpose: checks if this client is an actual human player
// Output : true if human, false otherwise
//---------------------------------------------------------------------------------
bool CClient::IsHumanPlayer(void) const
{
	if (!IsConnected())
		return false;

	if (IsFakeClient())
		return false;

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: throw away any residual garbage in the channel
// Input  : *pBaseClient - 
//---------------------------------------------------------------------------------
void CClient::Clear(CClient* pBaseClient)
{
	CBaseClient_Clear(pBaseClient);
}

//---------------------------------------------------------------------------------
// Purpose: connect new client
// Input  : *pClient - 
//			*szName - 
//			*pNetChannel - 
//			bFakePlayer - 
//			*a5 - 
//			*szMessage -
//			nMessageSize - 
// Output : true if connection was succesfull, false otherwise
//---------------------------------------------------------------------------------
bool CClient::Connect(CClient* pClient, const char* szName, void* pNetChannel, bool bFakePlayer, void* a5, char* szMessage, int nMessageSize)
{
	return CBaseClient_Connect(pClient, szName, pNetChannel, bFakePlayer, a5, szMessage, nMessageSize);
}

///////////////////////////////////////////////////////////////////////////////////
void CBaseClient_Attach()
{
	DetourAttach((LPVOID*)&CBaseClient_Clear, &CClient::Clear);
	DetourAttach((LPVOID*)&CBaseClient_Connect, &CClient::Connect);
}
void CBaseClient_Detach()
{
	DetourDetach((LPVOID*)&CBaseClient_Clear, &CClient::Clear);
	DetourDetach((LPVOID*)&CBaseClient_Connect, &CClient::Connect);
}

///////////////////////////////////////////////////////////////////////////////
CClient* g_pClient = nullptr;