//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef NETWORKSTRINGTABLE_H
#define NETWORKSTRINGTABLE_H
#include "tier0/fasttimer.h"
#include "tier1/utlvector.h"
#include "tier1/bitbuf.h"
#include "client/client.h"

typedef int TABLEID;

class INetworkStringTable
{
	INetworkStringTable* m_pVTable;
};

class CNetworkStringTable : public INetworkStringTable
{
public:
	TABLEID GetTableId(void) const;
	int GetMaxStrings(void) const;
	const char* GetTableName(void) const;
	int GetEntryBits(void) const;
	void SetTick(int tick_count);
	bool Lock(bool bLock);

private:
	TABLEID         m_id;
	bool            m_bLocked; // Might be wrong!
	char*           m_pszTableName;
	int             m_nMaxEntries;
	int             m_nEntryBits;
	int             m_nTickCount;
	int             m_nLastChangedTick;
	uint32_t        m_nFlags;
	// !TODO
};

class CNetworkStringTableContainer : public INetworkStringTable
{
public:
	static void WriteUpdateMessage(CNetworkStringTableContainer* thisp, CClient* client, unsigned int tick_ack, bf_write* msg);

private:
	bool        m_bAllowCreation;  // create guard
	int         m_nTickCount;      // current tick
	bool        m_bLocked;         // currently locked?
	bool        m_bEnableRollback; // enables rollback feature
	CUtlVector < CNetworkStringTable* > m_Tables; // the string tables
};

inline CMemory p_CNetworkStringTableContainer__WriteUpdateMessage;
inline void (*v_CNetworkStringTableContainer__WriteUpdateMessage)(CNetworkStringTableContainer* thisp, CClient* client, unsigned int tick_ack, bf_write* msg);

class VNetworkStringTableContainer : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CNetworkStringTableContainer::WriteUpdateMessage", p_CNetworkStringTableContainer__WriteUpdateMessage.GetPtr());
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_CNetworkStringTableContainer__WriteUpdateMessage = g_GameDll.FindPatternSIMD("48 89 74 24 ?? 55 57 41 54 41 55 41 56 48 8D AC 24 ?? ?? ?? ?? B8 ?? ?? ?? ??");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_CNetworkStringTableContainer__WriteUpdateMessage = g_GameDll.FindPatternSIMD("48 89 74 24 ?? 48 89 7C 24 ?? 55 41 54 41 55 41 56 41 57 48 8D AC 24 ?? ?? ?? ?? B8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 2B E0 45 33 ED");
#endif
		v_CNetworkStringTableContainer__WriteUpdateMessage =
			p_CNetworkStringTableContainer__WriteUpdateMessage.RCast<void (*)(CNetworkStringTableContainer*, CClient*, unsigned int, bf_write*)>();
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////

#endif // NETWORKSTRINGTABLE_H