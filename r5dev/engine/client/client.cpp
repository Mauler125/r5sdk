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
#include "mathlib/sha256.h"
#include "engine/server/server.h"
#include "engine/client/client.h"
#ifndef CLIENT_DLL
#include "jwt/include/decode.h"
#endif

// Absolute max string cmd length, any character past this will be NULLED.
#define STRINGCMD_MAX_LEN 512

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

static const char JWT_PUBLIC_KEY[] = 
"-----BEGIN PUBLIC KEY-----\n"
"MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA2/335exIZ6LE8pYi6e50\n"
"7tH19tXaeeEJVF5XXpTCXpndXIIWVimvg6xQ381eajySDw93wvG1DzW3U/6LHzyt\n"
"Q++N8w7N+FwnXyoDUD5Y8hheTZv6jjLoYT8ZtsMl20k9UosrbFBTMUhgmIT2dVth\n"
"LH+rT9ohpUNwQXHJvTOs9eY74GyfFw93+32LANBPZ8b+S8S3oZnKFVeCxRkYKsV0\n"
"b34POHVBbXNw6Kt163gR5zaiCfJJtRto9AA7MV2t9pfy8CChs3uJ+Xn7QVHD5cqt\n"
"Msg9MBac2Pvs2j+8wJ/igAVL5L81z3FXVt04id59TfPMUbYhRfY8pk7FB0MCigOH\n"
"dwIDAQAB\n"
"-----END PUBLIC KEY-----\n";

//---------------------------------------------------------------------------------
// Purpose: check whether this client is authorized to join this server
// Input  : *playerName  - 
//			*reasonBuf   - 
//			reasonBufLen - 
// Output : true if authorized, false otherwise
//---------------------------------------------------------------------------------
bool CClient::Authenticate(const char* const playerName, char* const reasonBuf, const size_t reasonBufLen)
{
#ifndef CLIENT_DLL
	// don't bother checking origin auth on bots or local clients
	if (IsFakeClient() || GetNetChan()->GetRemoteAddress().IsLoopback())
		return true;

#define FORMAT_ERROR_REASON(fmt, ...) V_snprintf(reasonBuf, reasonBufLen, fmt, ##__VA_ARGS__);

	KeyValues* const cl_onlineAuthTokenKv = this->m_ConVars->FindKey("cl_onlineAuthToken");
	KeyValues* const cl_onlineAuthTokenSignature1Kv = this->m_ConVars->FindKey("cl_onlineAuthTokenSignature1");
	KeyValues* const cl_onlineAuthTokenSignature2Kv = this->m_ConVars->FindKey("cl_onlineAuthTokenSignature2");

	if (!cl_onlineAuthTokenKv || !cl_onlineAuthTokenSignature1Kv)
	{
		FORMAT_ERROR_REASON("Missing token");
		return false;
	}

	const char* const onlineAuthToken = cl_onlineAuthTokenKv->GetString();
	const char* const onlineAuthTokenSignature1 = cl_onlineAuthTokenSignature1Kv->GetString();
	const char* const onlineAuthTokenSignature2 = cl_onlineAuthTokenSignature2Kv->GetString();

	const std::string fullToken = Format("%s.%s%s", onlineAuthToken, onlineAuthTokenSignature1, onlineAuthTokenSignature2);

	struct l8w8jwt_decoding_params params;
	l8w8jwt_decoding_params_init(&params);

	params.alg = L8W8JWT_ALG_RS256;

	params.jwt = (char*)fullToken.c_str();
	params.jwt_length = fullToken.length();

	params.verification_key = (unsigned char*)JWT_PUBLIC_KEY;
	params.verification_key_length = strlen(JWT_PUBLIC_KEY);

	params.validate_exp = sv_onlineAuthValidateExpiry->GetBool();
	params.exp_tolerance_seconds = (uint8_t)sv_onlineAuthExpiryTolerance->GetInt();

	params.validate_iat = sv_onlineAuthValidateIssuedAt->GetBool();
	params.iat_tolerance_seconds = (uint8_t)sv_onlineAuthIssuedAtTolerance->GetInt();

	l8w8jwt_claim* claims = nullptr;
	size_t numClaims = 0;

	enum l8w8jwt_validation_result validation_result;
	const int r = l8w8jwt_decode(&params, &validation_result, &claims, &numClaims);

	if (r != L8W8JWT_SUCCESS)
	{
		FORMAT_ERROR_REASON("Code %i", r);
		l8w8jwt_free_claims(claims, numClaims);

		return false;
	}

	if (validation_result != L8W8JWT_VALID)
	{
		char reasonBuffer[256];
		l8w8jwt_get_validation_result_desc(validation_result, reasonBuffer, sizeof(reasonBuffer));

		FORMAT_ERROR_REASON("%s", reasonBuffer);
		l8w8jwt_free_claims(claims, numClaims);

		return false;
	}

	bool foundSessionId = false;
	for (size_t i = 0; i < numClaims; ++i)
	{
		// session id
		if (!strcmp(claims[i].key, "sessionId"))
		{
			const char* const sessionId = claims[i].value;

			const std::string newId = Format(
				"%lld-%s-%s",
				this->m_DataBlock.userData,
				playerName,
				g_pMasterServer->GetHostIP().c_str()
			);

			DevMsg(eDLL_T::SERVER, "%s: newId=%s\n", __FUNCTION__, newId.c_str());
			const std::string hashedNewId = sha256(newId);

			if (hashedNewId.compare(sessionId) != 0)
			{
				FORMAT_ERROR_REASON("Token is not authorized for the connecting client");
				l8w8jwt_free_claims(claims, numClaims);

				return false;
			}

			foundSessionId = true;
		}
	}

	if (!foundSessionId)
	{
		FORMAT_ERROR_REASON("No session ID");
		l8w8jwt_free_claims(claims, numClaims);

		return false;
	}

	l8w8jwt_free_claims(claims, numClaims);

#undef REJECT_CONNECTION
#endif // !CLIENT_DLL

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: connect new client
// Input  : *szName - 
//			*pNetChan - 
//			bFakePlayer - 
//			*conVars - 
//			*szMessage -
//			nMessageSize - 
// Output : true if connection was successful, false otherwise
//---------------------------------------------------------------------------------
bool CClient::Connect(const char* szName, CNetChan* pNetChan, bool bFakePlayer,
	CUtlVector<NET_SetConVar::cvar_t>* conVars, char* szMessage, int nMessageSize)
{
#ifndef CLIENT_DLL
	g_ServerPlayer[GetUserID()].Reset(); // Reset ServerPlayer slot.
#endif

	if (!v_CClient_Connect(this, szName, pNetChan, bFakePlayer, conVars, szMessage, nMessageSize))
		return false;

#ifndef CLIENT_DLL

#define REJECT_CONNECTION(fmt, ...) V_snprintf(szMessage, nMessageSize, fmt, ##__VA_ARGS__);

	if (sv_onlineAuthEnable->GetBool())
	{
		char authFailReason[512];
		if (!Authenticate(szName, authFailReason, sizeof(authFailReason)))
		{
			REJECT_CONNECTION("Failed to verify authentication token [%s]", authFailReason);

			const bool bEnableLogging = sv_showconnecting->GetBool();
			if (bEnableLogging)
			{
				const char* const netAdr = pNetChan ? pNetChan->GetAddress() : "<unknown>";

				Warning(eDLL_T::SERVER, "Connection rejected for '%s' ('%llu' failed online authentication!)\n",
					netAdr, m_nNucleusID);
			}

			return false;
		}
	}

#undef REJECT_CONNECTION
#endif // !CLIENT_DLL

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: connect new client
// Input  : *pClient - 
//			*szName - 
//			*pNetChan - 
//			bFakePlayer - 
//			*a5 - 
//			*szMessage -
//			nMessageSize - 
// Output : true if connection was successful, false otherwise
//---------------------------------------------------------------------------------
bool CClient::VConnect(CClient* pClient, const char* szName, CNetChan* pNetChan, bool bFakePlayer,
	CUtlVector<NET_SetConVar::cvar_t>* conVars, char* szMessage, int nMessageSize)
{
	return pClient->Connect(szName, pNetChan, bFakePlayer, conVars, szMessage, nMessageSize);;
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
bool CClient::SendNetMsgEx(CNetMessage* pMsg, bool bLocal, bool bForceReliable, bool bVoice)
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
bool CClient::VSendNetMsgEx(CClient* pClient, CNetMessage* pMsg, bool bLocal, bool bForceReliable, bool bVoice)
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
	CClient* const pClient_Adj = AdjustShiftedThisPointer(pClient);

	// Jettison the cmd if the client isn't active.
	if (!pClient_Adj->IsActive())
		return true;

	const int nUserID = pClient_Adj->GetUserID();
	ServerPlayer_t* const pSlot = &g_ServerPlayer[nUserID];

	const double flStartTime = Plat_FloatTime();
	const int nCmdQuotaLimit = sv_quota_stringCmdsPerSecond->GetInt();

	if (!nCmdQuotaLimit)
		return true;

	const char* pCmd = pMsg->cmd;
	// Just skip if the cmd pointer is null, we still check if the
	// client sent too many commands and take appropriate actions.
	// The internal function discards the command if it's null.
	if (pCmd)
	{
		// There is an issue in CUtlBuffer::ParseToken() that causes it to read
		// past its buffer; mostly seems to happen on 32bit, but a carefully
		// crafted string should work on 64bit too). The fix is to just null 
		// everything past the maximum allowed length. The second 'theoretical'
		// fix would be to properly fix CUtlBuffer::ParseToken() by computing
		// the UTF8 character size each iteration and check if it still doesn't
		// exceed bounds.
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
	CClient* const pAdj = AdjustShiftedThisPointer(pClient);
	ServerPlayer_t* const pSlot = &g_ServerPlayer[pAdj->GetUserID()];

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

		// Add ConVar to list and set string.
		pAdj->m_ConVars->SetString(name, value);
		DevMsg(eDLL_T::SERVER, "UserInfo update from \"%s\": %s = %s\n", pAdj->GetClientName(), name, value);
	}

	pSlot->m_bInitialConVarsSet = true;
	pAdj->m_bConVarsChanged = true;
#endif // !CLIENT_DLL

	return true;
}

void VClient::Detour(const bool bAttach) const
{
#ifndef CLIENT_DLL
	DetourSetup(&v_CClient_Clear, &CClient::VClear, bAttach);
	DetourSetup(&v_CClient_Connect, &CClient::VConnect, bAttach);
	DetourSetup(&v_CClient_ActivatePlayer, &CClient::VActivatePlayer, bAttach);
	DetourSetup(&v_CClient_SendNetMsgEx, &CClient::VSendNetMsgEx, bAttach);
	//DetourSetup(&p_CClient_SendSnapshot, &CClient::VSendSnapshot, bAttach);

	DetourSetup(&v_CClient_ProcessStringCmd, &CClient::VProcessStringCmd, bAttach);
	DetourSetup(&v_CClient_ProcessSetConVar, &CClient::VProcessSetConVar, bAttach);
#endif // !CLIENT_DLL
}
