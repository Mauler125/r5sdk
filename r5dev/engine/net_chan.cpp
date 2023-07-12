//=============================================================================//
//
// Purpose: Netchannel system utilities
//
//=============================================================================//

#include "core/stdafx.h"
#include "tier0/frametask.h"
#include "tier1/cvar.h"
#include "vpc/keyvalues.h"
#include "common/callback.h"
#include "engine/net.h"
#include "engine/net_chan.h"
#ifndef CLIENT_DLL
#include "engine/server/server.h"
#include "engine/client/client.h"
#include "server/vengineserver_impl.h"
#endif // !CLIENT_DLL


//-----------------------------------------------------------------------------
// Purpose: gets the netchannel network loss
// Output : float
//-----------------------------------------------------------------------------
float CNetChan::GetNetworkLoss() const
{
	float v1 = *&m_DataFlow[1].frames[0].one;
	if (!v1 && !m_nSequencesSkipped_MAYBE)
		return 0.0f;

	float v4 = (v1 + m_nSequencesSkipped_MAYBE);
	if (v1 + m_nSequencesSkipped_MAYBE < 0)
		v4 = v4 + float(2 ^ 64);

	return m_nSequencesSkipped_MAYBE / v4;
}

//-----------------------------------------------------------------------------
// Purpose: gets the netchannel sequence number
// Input  : flow - 
// Output : int
//-----------------------------------------------------------------------------
int CNetChan::GetSequenceNr(int flow) const
{
	if (flow == FLOW_OUTGOING)
	{
		return m_nOutSequenceNr;
	}
	else if (flow == FLOW_INCOMING)
	{
		return m_nInSequenceNr;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: gets the netchannel connect time
// Output : double
//-----------------------------------------------------------------------------
double CNetChan::GetTimeConnected(void) const
{
	double t = *g_pNetTime - connect_time;
	return (t > 0.0) ? t : 0.0;
}

//-----------------------------------------------------------------------------
// Purpose: shutdown netchannel
// Input  : *this - 
//			*szReason - 
//			bBadRep - 
//			bRemoveNow - 
//-----------------------------------------------------------------------------
void CNetChan::_Shutdown(CNetChan* pChan, const char* szReason, uint8_t bBadRep, bool bRemoveNow)
{
	v_NetChan_Shutdown(pChan, szReason, bBadRep, bRemoveNow);
}

//-----------------------------------------------------------------------------
// Purpose: process message
// Input  : *pChan - 
//			*pMsg - 
// Output : true on success, false on failure
//-----------------------------------------------------------------------------
bool CNetChan::_ProcessMessages(CNetChan* pChan, bf_read* pMsg)
{
#ifndef CLIENT_DLL
	if (!ThreadInServerFrameThread() || !net_processTimeBudget->GetInt())
		return v_NetChan_ProcessMessages(pChan, pMsg);

	const double flStartTime = Plat_FloatTime();
	const bool bResult = v_NetChan_ProcessMessages(pChan, pMsg);

	if (!pChan->m_MessageHandler) // NetChannel removed?
		return bResult;

	CClient* pClient = reinterpret_cast<CClient*>(pChan->m_MessageHandler);
	ServerPlayer_t* pSlot = &g_ServerPlayer[pClient->GetUserID()];

	if (flStartTime - pSlot->m_flLastNetProcessTime >= 1.0 ||
		pSlot->m_flLastNetProcessTime == -1.0)
	{
		pSlot->m_flLastNetProcessTime = flStartTime;
		pSlot->m_flCurrentNetProcessTime = 0.0;
	}
	pSlot->m_flCurrentNetProcessTime +=
		(Plat_FloatTime() * 1000) - (flStartTime * 1000);

	if (pSlot->m_flCurrentNetProcessTime >
		net_processTimeBudget->GetDouble())
	{
		Warning(eDLL_T::SERVER, "Removing netchannel '%s' ('%s' exceeded frame budget by '%3.1f'ms!)\n", 
			pChan->GetName(), pChan->GetAddress(), (pSlot->m_flCurrentNetProcessTime - net_processTimeBudget->GetDouble()));
		pClient->Disconnect(Reputation_t::REP_MARK_BAD, "#DISCONNECT_NETCHAN_OVERFLOW");

		return false;
	}

	return bResult;
#else // !CLIENT_DLL
	return v_NetChan_ProcessMessages(pChan, pMsg);
#endif
}

//-----------------------------------------------------------------------------
// Purpose: send message
// Input  : &msg - 
//			bForceReliable - 
//			bVoice - 
// Output : true on success, false on failure
//-----------------------------------------------------------------------------
bool CNetChan::SendNetMsg(INetMessage& msg, bool bForceReliable, bool bVoice)
{
	if (remote_address.GetType() == netadrtype_t::NA_NULL)
		return false;

	bf_write* pStream = &m_StreamUnreliable;

	if (msg.IsReliable() || bForceReliable)
		pStream = &m_StreamReliable;

	if (bVoice)
		pStream = &m_StreamVoice;

	if (pStream != &m_StreamUnreliable ||
		pStream->GetNumBytesLeft() >= NET_UNRELIABLE_STREAM_MINSIZE)
	{
		AcquireSRWLockExclusive(&LOCK);

		pStream->WriteUBitLong(msg.GetType(), NETMSG_TYPE_BITS);
		if (!pStream->IsOverflowed())
			msg.WriteToBuffer(pStream);

		ReleaseSRWLockExclusive(&LOCK);
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////
void VNetChan::Attach() const
{
	DetourAttach((PVOID*)&v_NetChan_Shutdown, &CNetChan::_Shutdown);
	DetourAttach((PVOID*)&v_NetChan_ProcessMessages, &CNetChan::_ProcessMessages);
}
void VNetChan::Detach() const
{
	DetourDetach((PVOID*)&v_NetChan_Shutdown, &CNetChan::_Shutdown);
	DetourDetach((PVOID*)&v_NetChan_ProcessMessages, &CNetChan::_ProcessMessages);
}
