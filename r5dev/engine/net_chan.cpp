//=============================================================================//
//
// Purpose: Netchannel system utilities
//
//=============================================================================//

#include "core/stdafx.h"
#include "tier1/cvar.h"
#include "engine/sys_utils.h"
#include "engine/net.h"
#include "engine/net_chan.h"

//-----------------------------------------------------------------------------
// Purpose: gets the netchannel name
// Output : const char*
//-----------------------------------------------------------------------------
string CNetChan::GetName(void) const
{
	// [0x1A8D + 0x1] (first char in array is a null character!).
	const char* pszName = this->m_Name + 1;
	return string(pszName, NET_CHANNELNAME_MAXLEN);
}

//-----------------------------------------------------------------------------
// Purpose: gets the netchannel address
// Output : const char*
//-----------------------------------------------------------------------------
string CNetChan::GetAddress(void) const
{
	char szAdr[INET6_ADDRSTRLEN]{};
	if (!inet_ntop(AF_INET6, &this->remote_address.adr, szAdr, INET6_ADDRSTRLEN))
	{
		if (sv_showconnecting->GetBool())
		{
			Warning(eDLL_T::ENGINE, "%s - Address conversion failed: %s", __FUNCTION__, NET_ErrorString(WSAGetLastError()));
		}
	}
	return szAdr;
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
// Output : int
//-----------------------------------------------------------------------------
int CNetChan::GetTotalData(int flow) const
{
	return this->m_DataFlow[flow].totalbytes;
}

//-----------------------------------------------------------------------------
// Purpose: gets the netchannel total packets
// Input  : flow - 
// Output : int
//-----------------------------------------------------------------------------
int CNetChan::GetTotalPackets(int flow) const
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
double CNetChan::GetConnectTime(void) const
{
	return this->connect_time;
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
