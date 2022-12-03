#pragma once

class SendTable;

//-----------------------------------------------------------------------------
// Purpose: Server side class definition
//-----------------------------------------------------------------------------
class ServerClass
{
public:
	const char* GetName(void) const
	{
		return m_pNetworkName;
	}

public:
	char* m_pNetworkName;
	SendTable* m_pSendTable;
	ServerClass* m_pNext;
	int m_Unknown1;
	int m_ClassID;
	int m_InstanceBaselineIndex;
};