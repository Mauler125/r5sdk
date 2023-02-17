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
#include "tier1/cvar.h"
#include "engine/server/server.h"
#include "engine/client/client.h"

//---------------------------------------------------------------------------------
// Purpose: gets the client from buffer by index
//---------------------------------------------------------------------------------
CClient* CClient::GetClient(int nIndex) const
{
	return reinterpret_cast<CClient*>(
		(reinterpret_cast<uintptr_t>(g_pClient) + (nIndex * sizeof(CClient))));
}

//---------------------------------------------------------------------------------
// Purpose: gets the handle of this client
//---------------------------------------------------------------------------------
edict_t CClient::GetHandle(void) const
{
	return m_nHandle;
}

//---------------------------------------------------------------------------------
// Purpose: gets the userID of this client
//---------------------------------------------------------------------------------
uint32_t CClient::GetUserID(void) const
{
	return m_nUserID;
}

//---------------------------------------------------------------------------------
// Purpose: gets the nucleusID of this client
//---------------------------------------------------------------------------------
uint64_t CClient::GetNucleusID(void) const
{
	return m_nNucleusID;
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
// Purpose: gets the pointer to the server object
//---------------------------------------------------------------------------------
CServer* CClient::GetServer(void) const
{
	return m_pServer;
}

//---------------------------------------------------------------------------------
// Purpose: gets the command tick
//---------------------------------------------------------------------------------
int CClient::GetCommandTick(void) const
{
	return m_nCommandTick;
}

//---------------------------------------------------------------------------------
// Purpose: gets the name of this client (managed by server)
//---------------------------------------------------------------------------------
const char* CClient::GetServerName(void) const
{
	return m_szServerName;
}

//---------------------------------------------------------------------------------
// Purpose: gets the name of this client (obtained from connectionless packet)
//---------------------------------------------------------------------------------
const char* CClient::GetClientName(void) const
{
	return m_szClientName;
}

//---------------------------------------------------------------------------------
// Purpose: sets the handle of this client
//---------------------------------------------------------------------------------
void CClient::SetHandle(edict_t nHandle)
{
	m_nHandle = nHandle;
}

//---------------------------------------------------------------------------------
// Purpose: sets the userID of this client
//---------------------------------------------------------------------------------
void CClient::SetUserID(uint32_t nUserID)
{
	m_nUserID = nUserID;
}

//---------------------------------------------------------------------------------
// Purpose: sets the nucleusID of this client
//---------------------------------------------------------------------------------
void CClient::SetNucleusID(uint64_t nNucleusID)
{
	m_nNucleusID = nNucleusID;
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
//---------------------------------------------------------------------------------
void CClient::Clear(void)
{
#ifndef CLIENT_DLL
	g_ServerPlayer[GetUserID()].Reset(); // Reset ServerPlayer slot.
#endif // !CLIENT_DLL
	v_CClient_Clear(this);
}

//---------------------------------------------------------------------------------
// Purpose: throw away any residual garbage in the channel
// Input  : *pClient - 
//---------------------------------------------------------------------------------
void CClient::VClear(CClient* pClient)
{
#ifndef CLIENT_DLL
	g_ServerPlayer[pClient->GetUserID()].Reset(); // Reset ServerPlayer slot.
#endif // !CLIENT_DLL
	v_CClient_Clear(pClient);
}

//---------------------------------------------------------------------------------
// Purpose: connect new client
// Input  : *szName - 
//			*pNetChannel - 
//			bFakePlayer - 
//			*a5 - 
//			*szMessage -
//			nMessageSize - 
// Output : true if connection was successful, false otherwise
//---------------------------------------------------------------------------------
bool CClient::Connect(const char* szName, void* pNetChannel, bool bFakePlayer, void* a5, char* szMessage, int nMessageSize)
{
	return v_CClient_Connect(this, szName, pNetChannel, bFakePlayer, a5, szMessage, nMessageSize);
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
// Output : true if connection was successful, false otherwise
//---------------------------------------------------------------------------------
bool CClient::VConnect(CClient* pClient, const char* szName, void* pNetChannel, bool bFakePlayer, void* a5, char* szMessage, int nMessageSize)
{
	bool bResult = v_CClient_Connect(pClient, szName, pNetChannel, bFakePlayer, a5, szMessage, nMessageSize);
#ifndef CLIENT_DLL
	g_ServerPlayer[pClient->GetUserID()].Reset(); // Reset ServerPlayer slot.
#endif // !CLIENT_DLL
	return bResult;
}

//---------------------------------------------------------------------------------
// Purpose: disconnect client
// Input  : nRepLvl - 
//			*szReason - 
//			... - 
//---------------------------------------------------------------------------------
void CClient::Disconnect(const Reputation_t nRepLvl, const char* szReason, ...)
{
	if (m_nSignonState != SIGNONSTATE::SIGNONSTATE_NONE)
	{
		char szBuf[1024];
		{/////////////////////////////
			va_list vArgs{};
			va_start(vArgs, szReason);

			vsnprintf(szBuf, sizeof(szBuf), szReason, vArgs);

			szBuf[sizeof(szBuf) - 1] = '\0';
			va_end(vArgs);
		}/////////////////////////////
		v_CClient_Disconnect(this, nRepLvl, szBuf);
	}
}

bool CClient::SendNetMsg(CNetMessage* pMsg, char bLocal, bool bForceReliable, bool bVoice)
{
	return v_CClient_SendNetMsg(this, pMsg, bLocal, bForceReliable, bVoice);
}

void* CClient::VSendSnapshot(CClient* pClient, CClientFrame* pFrame, int nTick, int nTickAck)
{
	return v_CClient_SendSnapshot(pClient, pFrame, nTick, nTickAck);
}

//---------------------------------------------------------------------------------
// Purpose: process string commands (kicking anyone attempting to DOS)
// Input  : *pClient - (ADJ)
//			*pMsg - 
// Output : false if cmd should be passed to CServerGameClients
//---------------------------------------------------------------------------------
bool CClient::VProcessStringCmd(CClient* pClient, NET_StringCmd* pMsg)
{
#ifndef CLIENT_DLL
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	CClient* pClient_Adj = pClient;
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	/* Original function called method "CClient::ExecuteStringCommand" with an optimization
	 * that shifted the 'this' pointer with 8 bytes.
	 * Since this has been inlined with "CClient::ProcessStringCmd" as of S2, the shifting
	 * happens directly to anything calling this function. */
	char* pShifted = reinterpret_cast<char*>(pClient) - 8;
	CClient* pClient_Adj = reinterpret_cast<CClient*>(pShifted);
#endif // !GAMEDLL_S0 || !GAMEDLL_S1
	ServerPlayer_t* pSlot = &g_ServerPlayer[pClient_Adj->GetUserID()];
	double flStartTime = Plat_FloatTime();
	int nCmdQuotaLimit = sv_quota_stringCmdsPerSecond->GetInt();

	if (!nCmdQuotaLimit)
		return true;

	if (flStartTime - pSlot->m_flStringCommandQuotaTimeStart >= 1.0)
	{
		pSlot->m_flStringCommandQuotaTimeStart = flStartTime;
		pSlot->m_nStringCommandQuotaCount = 0;
	}
	++pSlot->m_nStringCommandQuotaCount;

	if (pSlot->m_nStringCommandQuotaCount > nCmdQuotaLimit)
	{
		Warning(eDLL_T::SERVER, "Removing client '%s' from slot '%i' ('%llu' exceeded string command quota!)\n", 
			pClient_Adj->GetNetChan()->GetAddress(), pClient_Adj->GetUserID(), pClient_Adj->GetNucleusID());

		pClient_Adj->Disconnect(Reputation_t::REP_MARK_BAD, "#DISCONNECT_STRINGCMD_OVERFLOW");
		return true;
	}
#endif // !CLIENT_DLL

	return v_CClient_ProcessStringCmd(pClient, pMsg);
}

///////////////////////////////////////////////////////////////////////////////
CClient* g_pClient = nullptr;