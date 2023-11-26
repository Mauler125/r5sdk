//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//
#include "core/stdafx.h"
#include "tier1/cvar.h"
#ifndef CLIENT_DLL
#include "server/server.h"
#include "host.h"
#endif // !CLIENT_DLL
#include "net.h"
#include "networkstringtable.h"

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : i - 
// Output : CNetworkStringTableItem
//-----------------------------------------------------------------------------
//CNetworkStringTableItem* CNetworkStringTable::GetItem(int i)
//{
//	if (i >= 0)
//	{
//		return &m_pItems->Element(i);
//	}
//
//	Assert(m_pItemsClientSide);
//	return &m_pItemsClientSide->Element(-i);
//}

//-----------------------------------------------------------------------------
// Purpose: Returns the table identifier
// Output : TABLEID
//-----------------------------------------------------------------------------
TABLEID CNetworkStringTable::GetTableId(void) const
{
	return m_id;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the max size of the table
// Output : int
//-----------------------------------------------------------------------------
int CNetworkStringTable::GetMaxStrings(void) const
{
	return m_nMaxEntries;
}

//-----------------------------------------------------------------------------
// Purpose: Returns a table, by name
// Output : const char
//-----------------------------------------------------------------------------
const char* CNetworkStringTable::GetTableName(void) const
{
	return m_pszTableName;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the number of bits needed to encode an entry index
// Output : int
//-----------------------------------------------------------------------------
int CNetworkStringTable::GetEntryBits(void) const
{
	return m_nEntryBits;
}

//-----------------------------------------------------------------------------
// Purpose: Sets the tick count
//-----------------------------------------------------------------------------
void CNetworkStringTable::SetTick(int tick_count)
{
	Assert(tick_count >= m_nTickCount);
	m_nTickCount = tick_count;
}

//-----------------------------------------------------------------------------
// Purpose: Locks the string table
//-----------------------------------------------------------------------------
bool CNetworkStringTable::Lock(bool bLock)
{
	bool bState = m_bLocked;
	m_bLocked = bLock;
	return bState;
}

//-----------------------------------------------------------------------------
// Purpose: Writes network string table delta's to snapshot buffer
// Input  : *pClient - 
//			nTickAck - 
//			*pMsg - 
//-----------------------------------------------------------------------------
void CNetworkStringTableContainer::WriteUpdateMessage(CNetworkStringTableContainer* thisp, CClient* pClient, unsigned int nTickAck, bf_write* pMsg)
{
#ifndef CLIENT_DLL
	if (sv_stats->GetBool())
	{
		uint8_t nCPUPercentage = static_cast<uint8_t>(g_pServer->GetCPUUsage() * 100.0f);
		SVC_ServerTick serverTick(g_pServer->GetTick(), *host_frametime_unbounded, *host_frametime_stddeviation, nCPUPercentage);

		serverTick.m_nGroup = 0;
		serverTick.m_bReliable = true;
		serverTick.m_NetTick.m_nCommandTick = -1; // Statistics only, see 'CClientState::VProcessServerTick'.

		pMsg->WriteUBitLong(serverTick.GetType(), NETMSG_TYPE_BITS);
		if (!pMsg->IsOverflowed())
		{
			serverTick.WriteToBuffer(pMsg);
		}
	}
#endif // !CLIENT_DLL
	v_CNetworkStringTableContainer__WriteUpdateMessage(thisp, pClient, nTickAck, pMsg);
}

void VNetworkStringTableContainer::Detour(const bool bAttach) const
{
#if !defined (CLIENT_DLL) && !defined (GAMEDLL_S0) && !defined (GAMEDLL_S1) // TODO: doesn't work properly for S0/S1 yet.
	DetourSetup(&v_CNetworkStringTableContainer__WriteUpdateMessage, &CNetworkStringTableContainer::WriteUpdateMessage, bAttach);
#endif // !CLIENT_DLL && !GAMEDLL_S0 && !GAMEDLL_S1
}
