#pragma once

//-----------------------------------------------------------------------------
// Purpose: Easier access networkable vars from memory.
//-----------------------------------------------------------------------------

class CRecvProp;
class CRecvTable;
class SendTable;

class CNetVarTables
{
public:
	struct Table
	{
		string m_svNetTableName;
		CRecvProp* m_pProp;
		int m_Offset;
		vector<CRecvProp*> m_vChildProps;
		vector<Table> m_vChildTables;
	};

	CNetVarTables(bool isServer);

#ifndef DEDICATED
	Table LoadTable(CRecvTable* recvTable);
#endif // !DEDICATED

#ifndef CLIENT_DLL
	Table LoadTable(SendTable* sendTable);
#endif // !CLIENT_DLL

	// DEDI still needs some work.
#ifndef DEDICATED
	int GetOffset(const string& tableName, const string& varName);
	int GetOffset(const Table& table, const string& varName);
#endif // !DEDICATED

	vector<Table> vTables;
};