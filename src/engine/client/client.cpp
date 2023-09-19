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
#include "tier1/strtools.h"
#include "engine/server/server.h"
#include "engine/client/client.h"

// 128+1 so that the client still receives the 'console command too long' message.
#define STRINGCMD_MAX_LEN 129

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
	pClient->Clear();
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
#ifndef CLIENT_DLL
	g_ServerPlayer[GetUserID()].Reset(); // Reset ServerPlayer slot.
#endif // !CLIENT_DLL
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
	return pClient->Connect(szName, pNetChannel, bFakePlayer, a5, szMessage, nMessageSize);;
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
			va_list vArgs;
			va_start(vArgs, szReason);

			vsnprintf(szBuf, sizeof(szBuf), szReason, vArgs);

			szBuf[sizeof(szBuf) - 1] = '\0';
			va_end(vArgs);
		}/////////////////////////////
		v_CClient_Disconnect(this, nRepLvl, szBuf);
	}
}

//---------------------------------------------------------------------------------
// Purpose: activate player
// Input  : *pClient - 
//---------------------------------------------------------------------------------
void CClient::VActivatePlayer(CClient* pClient)
{
	// Set the client instance to 'ready' before calling ActivatePlayer.
	pClient->SetPersistenceState(PERSISTENCE::PERSISTENCE_READY);
	v_CClient_ActivatePlayer(pClient);

#ifndef CLIENT_DLL
	const CNetChan* pNetChan = pClient->GetNetChan();

	if (pNetChan && sv_showconnecting->GetBool())
	{
		Msg(eDLL_T::SERVER, "Activated player #%d; channel %s(%s) ('%llu')\n",
			pClient->GetUserID(), pNetChan->GetName(), pNetChan->GetAddress(), pClient->GetNucleusID());
	}
#endif // !CLIENT_DLL
}

//---------------------------------------------------------------------------------
// Purpose: send a net message with replay.
//			set 'CNetMessage::m_nGroup' to 'NoReplay' to disable replay.
// Input  : *pMsg - 
//			bLocal - 
//			bForceReliable - 
//			bVoice - 
//---------------------------------------------------------------------------------
bool CClient::SendNetMsgEx(CNetMessage* pMsg, char bLocal, bool bForceReliable, bool bVoice)
{
	if (!ShouldReplayMessage(pMsg))
	{
		// Don't copy the message into the replay buffer.
		pMsg->m_nGroup = NetMessageGroup::NoReplay;
	}

	return v_CClient_SendNetMsgEx(this, pMsg, bLocal, bForceReliable, bVoice);
}

//---------------------------------------------------------------------------------
// Purpose: send a snapshot
// Input  : *pClient - 
//			*pFrame - 
//			nTick - 
//			nTickAck - 
//---------------------------------------------------------------------------------
void* CClient::VSendSnapshot(CClient* pClient, CClientFrame* pFrame, int nTick, int nTickAck)
{
	return v_CClient_SendSnapshot(pClient, pFrame, nTick, nTickAck);
}

//---------------------------------------------------------------------------------
// Purpose: internal hook to 'CClient::SendNetMsgEx'
// Input  : *pClient - 
//			*pMsg - 
//			bLocal - 
//			bForceReliable - 
//			bVoice - 
//---------------------------------------------------------------------------------
bool CClient::VSendNetMsgEx(CClient* pClient, CNetMessage* pMsg, char bLocal, bool bForceReliable, bool bVoice)
{
	return pClient->SendNetMsgEx(pMsg, bLocal, bForceReliable, bVoice);
}

//---------------------------------------------------------------------------------
// Purpose: some versions of the binary have an optimization that shifts the 'this'
// pointer of the CClient structure by 8 bytes to avoid having to cache the vftable
// pointer if it never get used. Here we shift it back so it aligns again.
//---------------------------------------------------------------------------------
CClient* AdjustShiftedThisPointer(CClient* shiftedPointer)
{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	return shiftedPointer;
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	/* Original function called method "CClient::ExecuteStringCommand" with an optimization
	 * that shifted the 'this' pointer with 8 bytes.
	 * Since this has been inlined with "CClient::ProcessStringCmd" as of S2, the shifting
	 * happens directly to anything calling this function. */
	char* pShifted = reinterpret_cast<char*>(shiftedPointer) - 8;
	return reinterpret_cast<CClient*>(pShifted);
#endif // !GAMEDLL_S0 || !GAMEDLL_S1
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
	CClient* pClient_Adj = AdjustShiftedThisPointer(pClient);

	// Jettison the cmd if the client isn't active.
	if (!pClient_Adj->IsActive())
		return true;

	int nUserID = pClient_Adj->GetUserID();
	ServerPlayer_t* pSlot = &g_ServerPlayer[nUserID];

	double flStartTime = Plat_FloatTime();
	int nCmdQuotaLimit = sv_quota_stringCmdsPerSecond->GetInt();

	if (!nCmdQuotaLimit)
		return true;

	const char* pCmd = pMsg->cmd;
	// Just skip if the cmd pointer is null, we still check if the
	// client sent too many commands and take appropriate actions.
	// The internal function discards the command if it's null.
	if (pCmd)
	{
		// If the string length exceeds 128, the engine will return a 'command
		// string too long' message back to the client that issued it and
		// subsequently jettison the string cmd. Before this routine gets hit,
		// the entire string gets parsed (up to 512 bytes). There is an issue
		// in CUtlBuffer::ParseToken() that causes it to read past its buffer;
		// mostly seems to happen on 32bit, but a carefully crafted string
		// should work on 64bit too). The fix is to just null everything past
		// the maximum allowed length. The second 'theoretical' fix would be to
		// properly fix CUtlBuffer::ParseToken() by computing the UTF8 character
		// size each iteration and check if it still doesn't exceed bounds.
		memset(&pMsg->buffer[STRINGCMD_MAX_LEN],
			'\0', sizeof(pMsg->buffer) - (STRINGCMD_MAX_LEN));

		if (!V_IsValidUTF8(pCmd))
		{
			Warning(eDLL_T::SERVER, "Removing client '%s' from slot #%i ('%llu' sent invalid string command!)\n",
				pClient_Adj->GetNetChan()->GetAddress(), pClient_Adj->GetUserID(), pClient_Adj->GetNucleusID());

			pClient_Adj->Disconnect(Reputation_t::REP_MARK_BAD, "#DISCONNECT_INVALID_STRINGCMD");
			return true;
		}
	}

	if (flStartTime - pSlot->m_flStringCommandQuotaTimeStart >= 1.0)
	{
		pSlot->m_flStringCommandQuotaTimeStart = flStartTime;
		pSlot->m_nStringCommandQuotaCount = 0;
	}
	++pSlot->m_nStringCommandQuotaCount;

	if (pSlot->m_nStringCommandQuotaCount > nCmdQuotaLimit)
	{
		Warning(eDLL_T::SERVER, "Removing client '%s' from slot #%i ('%llu' exceeded string command quota!)\n",
			pClient_Adj->GetNetChan()->GetAddress(), pClient_Adj->GetUserID(), pClient_Adj->GetNucleusID());

		pClient_Adj->Disconnect(Reputation_t::REP_MARK_BAD, "#DISCONNECT_STRINGCMD_OVERFLOW");
		return true;
	}
#endif // !CLIENT_DLL

	return v_CClient_ProcessStringCmd(pClient, pMsg);
}

//---------------------------------------------------------------------------------
// Purpose: process set convar
// Input  : *pClient - (ADJ)
//			*pMsg - 
// Output : 
//---------------------------------------------------------------------------------
bool CClient::VProcessSetConVar(CClient* pClient, NET_SetConVar* pMsg)
{
#ifndef CLIENT_DLL
	CClient* pAdj = AdjustShiftedThisPointer(pClient);
	ServerPlayer_t* pSlot = &g_ServerPlayer[pAdj->GetUserID()];

	// This loop never exceeds 255 iterations, NET_SetConVar::ReadFromBuffer(...)
	// reads and inserts up to 255 entries in the vector (reads a byte for size).
	FOR_EACH_VEC(pMsg->m_ConVars, i)
	{
		const NET_SetConVar::cvar_t& entry = pMsg->m_ConVars[i];
		const char* const name = entry.name;
		const char* const value = entry.value;

		// Discard any ConVar change request if it contains funky characters.
		bool bFunky = false;
		for (const char* s = name; *s != '\0'; ++s)
		{
			if (!isalnum(*s) && *s != '_')
			{
				bFunky = true;
				break;
			}
		}
		if (bFunky)
		{
			DevWarning(eDLL_T::SERVER, "Ignoring ConVar change request for variable '%s' from client '%s'; invalid characters in the variable name\n",
				name, pAdj->GetClientName());
			continue;
		}

		// The initial set of ConVars must contain all client ConVars that are
		// flagged UserInfo. This is a simple fix to exploits that send bogus
		// data later, and catches bugs, such as new UserInfo ConVars appearing
		// later, which shouldn't happen.
		if (pSlot->m_bInitialConVarsSet && !pAdj->m_ConVars->FindKey(name))
		{
			DevWarning(eDLL_T::SERVER, "UserInfo update from \"%s\" ignored: %s = %s\n",
				pAdj->GetClientName(), name, value);
			continue;
		}

		pAdj->m_ConVars->SetString(name, value);
		DevMsg(eDLL_T::SERVER, "UserInfo update from \"%s\": %s = %s\n", pAdj->GetClientName(), name, value);
	}

	pSlot->m_bInitialConVarsSet = true;
	pAdj->m_bConVarsChanged = true;
#endif // !CLIENT_DLL

	return true;
}

void VClient::Attach(void) const
{
#ifndef CLIENT_DLL
	DetourAttach((LPVOID*)&v_CClient_Clear, &CClient::VClear);
	DetourAttach((LPVOID*)&v_CClient_Connect, &CClient::VConnect);
	DetourAttach((LPVOID*)&v_CClient_ActivatePlayer, &CClient::VActivatePlayer);
	DetourAttach((LPVOID*)&v_CClient_SendNetMsgEx, &CClient::VSendNetMsgEx);
	//DetourAttach((LPVOID*)&p_CClient_SendSnapshot, &CClient::VSendSnapshot);

	DetourAttach((LPVOID*)&v_CClient_ProcessStringCmd, &CClient::VProcessStringCmd);
	DetourAttach((LPVOID*)&v_CClient_ProcessSetConVar, &CClient::VProcessSetConVar);
#endif // !CLIENT_DLL
}
void VClient::Detach(void) const
{
#ifndef CLIENT_DLL
	DetourDetach((LPVOID*)&v_CClient_Clear, &CClient::VClear);
	DetourDetach((LPVOID*)&v_CClient_Connect, &CClient::VConnect);
	DetourDetach((LPVOID*)&v_CClient_ActivatePlayer, &CClient::VActivatePlayer);
	DetourDetach((LPVOID*)&v_CClient_SendNetMsgEx, &CClient::VSendNetMsgEx);
	//DetourDetach((LPVOID*)&p_CClient_SendSnapshot, &CClient::VSendSnapshot);

	DetourDetach((LPVOID*)&v_CClient_ProcessStringCmd, &CClient::VProcessStringCmd);
	DetourDetach((LPVOID*)&v_CClient_ProcessSetConVar, &CClient::VProcessSetConVar);
#endif // !CLIENT_DLL
}
