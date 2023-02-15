//=============================================================================//
//
// Purpose: Netchannel system utilities
//
//=============================================================================//

#include "core/stdafx.h"
#include "tier1/cvar.h"
#include "engine/net.h"
#include "engine/net_chan.h"
#ifndef CLIENT_DLL
#include "engine/server/server.h"
#include "engine/client/client.h"
#include "server/vengineserver_impl.h"
#endif // !CLIENT_DLL


//-----------------------------------------------------------------------------
// Purpose: gets the netchannel name
// Output : const char*
//-----------------------------------------------------------------------------
const char* CNetChan::GetName(void) const
{
	return this->m_Name;
}

//-----------------------------------------------------------------------------
// Purpose: gets the netchannel address
// Output : const char*
//-----------------------------------------------------------------------------
const char* CNetChan::GetAddress(void) const
{
	return this->remote_address.ToString();
}

//-----------------------------------------------------------------------------
// Purpose: gets the netchannel data rate
// Output : int
//-----------------------------------------------------------------------------
int CNetChan::GetDataRate(void) const
{
	return this->m_Rate;
}

//-----------------------------------------------------------------------------
// Purpose: gets the netchannel buffer size (NET_FRAMES_BACKUP)
// Output : int
//-----------------------------------------------------------------------------
int CNetChan::GetBufferSize(void) const
{
	return NET_FRAMES_BACKUP;
}

//-----------------------------------------------------------------------------
// Purpose: gets the netchannel latency
// Input  : flow - 
// Output : float
//-----------------------------------------------------------------------------
float CNetChan::GetLatency(int flow) const
{
	return this->m_DataFlow[flow].latency;
}

//-----------------------------------------------------------------------------
// Purpose: gets the netchannel average choke
// Input  : flow - 
// Output : float
//-----------------------------------------------------------------------------
float CNetChan::GetAvgChoke(int flow) const
{
	return this->m_DataFlow[flow].avgchoke;
}

//-----------------------------------------------------------------------------
// Purpose: gets the netchannel average latency
// Input  : flow - 
// Output : float
//-----------------------------------------------------------------------------
float CNetChan::GetAvgLatency(int flow) const
{
	return this->m_DataFlow[flow].avglatency;
}

//-----------------------------------------------------------------------------
// Purpose: gets the netchannel average loss
// Input  : flow - 
// Output : float
//-----------------------------------------------------------------------------
float CNetChan::GetAvgLoss(int flow) const
{
	return this->m_DataFlow[flow].avgloss;
}

//-----------------------------------------------------------------------------
// Purpose: gets the netchannel average packets
// Input  : flow - 
// Output : float
//-----------------------------------------------------------------------------
float CNetChan::GetAvgPackets(int flow) const
{
	return this->m_DataFlow[flow].avgpacketspersec;
}

//-----------------------------------------------------------------------------
// Purpose: gets the netchannel average data
// Input  : flow - 
// Output : float
//-----------------------------------------------------------------------------
float CNetChan::GetAvgData(int flow) const
{
	return this->m_DataFlow[flow].avgbytespersec;
}

//-----------------------------------------------------------------------------
// Purpose: gets the netchannel total data
// Input  : flow - 
// Output : int64_t
//-----------------------------------------------------------------------------
int64_t CNetChan::GetTotalData(int flow) const
{
	return this->m_DataFlow[flow].totalbytes;
}

//-----------------------------------------------------------------------------
// Purpose: gets the netchannel total packets
// Input  : flow - 
// Output : int64_t
//-----------------------------------------------------------------------------
int64_t CNetChan::GetTotalPackets(int flow) const
{
	return this->m_DataFlow[flow].totalpackets;
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
		return this->m_nOutSequenceNr;
	}
	else if (flow == FLOW_INCOMING)
	{
		return this->m_nInSequenceNr;
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
// Purpose: gets the netchannel timeout
// Output : float
//-----------------------------------------------------------------------------
float CNetChan::GetTimeoutSeconds(void) const
{
	return this->m_Timeout;
}

//-----------------------------------------------------------------------------
// Purpose: gets the netchannel socket
// Output : int
//-----------------------------------------------------------------------------
int CNetChan::GetSocket(void) const
{
	return this->m_Socket;
}

//-----------------------------------------------------------------------------
// Purpose: checks if the reliable stream is overflowed
// Output : true if overflowed, false otherwise
//-----------------------------------------------------------------------------
bool CNetChan::IsOverflowed(void) const
{
	return this->m_StreamReliable.IsOverflowed();
}

//-----------------------------------------------------------------------------
// Purpose: clears the netchannel
//-----------------------------------------------------------------------------
void CNetChan::Clear(bool bStopProcessing)
{
	v_NetChan_Clear(this, bStopProcessing);
}

//-----------------------------------------------------------------------------
// Purpose: process message
// Input  : *pChan - 
//			*pMsg - 
// Output : true on success, false on failure
//-----------------------------------------------------------------------------
bool CNetChan::ProcessMessages(CNetChan* pChan, bf_read* pMsg)
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

///////////////////////////////////////////////////////////////////////////////
void VNetChannel::Attach() const
{
	DetourAttach(&v_NetChan_ProcessMessages, &CNetChan::ProcessMessages);
}
void VNetChannel::Detach() const
{
	DetourDetach(&v_NetChan_ProcessMessages, &CNetChan::ProcessMessages);
}
