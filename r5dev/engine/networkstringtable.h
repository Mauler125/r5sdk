//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef NETWORKSTRINGTABLE_H
#define NETWORKSTRINGTABLE_H

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
	bool        m_bAllowCreation;  // create guard
	int         m_nTickCount;      // current tick
	bool        m_bLocked;         // currently locked?
	bool        m_bEnableRollback; // enables rollback feature
	//CUtlVector < CNetworkStringTable* > m_Tables;	// the string tables
};

#endif // NETWORKSTRINGTABLE_H