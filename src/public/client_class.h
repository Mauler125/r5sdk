#pragma once
#include "public/iclientnetworkable.h"

typedef IClientNetworkable* (*CreateClientClassFn)(int entNum, int serialNum);
typedef IClientNetworkable* (*CreateEventFn)();
class CRecvTable;

//-----------------------------------------------------------------------------
// Purpose: Client side class definition
//-----------------------------------------------------------------------------
class ClientClass
{
public:
	const char* GetName(void) const
	{
		return m_pNetworkName;
	}

public:
	CreateClientClassFn m_pCreateFn;
	CreateEventFn       m_pCreateEventFn;
	char*               m_pNetworkName;
	CRecvTable*         m_pRecvTable;
	ClientClass*        m_pNext;
	int                 m_ClassID;
	int                 m_ClassSize;
};