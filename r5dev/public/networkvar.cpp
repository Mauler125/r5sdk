#include "core/stdafx.h"
#include "public/networkvar.h"

//-----------------------------------------------------------------------------
// Purpose: Easier access networkable vars from memory.
//-----------------------------------------------------------------------------

#ifndef CLIENT_DLL
//#include "game/server/gameinterface.h" // this doesn't wanna link properly and complains, gotta look at it later again.
#endif // !CLIENT_DLL

#ifndef DEDICATED
#include "engine/client/cdll_engine_int.h"
#include "public/dt_recv.h"
#endif // !DEDICATED

CNetVarTables::CNetVarTables(bool isServer)
{
	if (isServer)
	{
#ifndef CLIENT_DLL
		/*
		for (ServerClass* sc = g_pServerGameDLL->GetAllServerClasses(); sc != nullptr; sc = sc->m_pNext)
		{
			if (sc->m_pSendTable)
			{
				vTables.emplace_back(LoadTable(sc->m_pSendTable));
			}
		}
		*/
#endif // !CLIENT_DLL
	}
	else
	{
#ifndef DEDICATED
		for (ClientClass* cc = (*g_pHLClient)->GetAllClasses(); cc != nullptr; cc = cc->m_pNext)
		{
			if (cc->m_pRecvTable)
			{
				vTables.emplace_back(LoadTable(cc->m_pRecvTable));
			}
		}
#endif // !DEDICATED
	}
}

#ifndef DEDICATED
CNetVarTables::Table CNetVarTables::LoadTable(CRecvTable* recvTable)
{
	Table nvTable = Table{};

	nvTable.m_Offset = 0;
	nvTable.m_svNetTableName = recvTable->m_pNetTableName;

	for (int i = 0; i < recvTable->m_nProps; i++)
	{
		CRecvProp* prop = recvTable->m_pProps[i];

		if (!prop || std::isdigit(prop->m_pVarName[0])) // Weird edge cases. Need double checking.
			continue;

		if (strcmp(prop->m_pVarName, "baseclass") == NULL) // No weird baseclass. Need double checking.
			continue;

		if (prop->m_RecvType == SendPropType::DPT_DataTable && prop->m_pDataTable)
		{
			nvTable.m_vChildTables.emplace_back(LoadTable(prop->m_pDataTable));
			nvTable.m_vChildTables.back().m_Offset = prop->m_Offset;
			nvTable.m_vChildTables.back().m_pProp = prop;
		}
		else
		{
			nvTable.m_vChildProps.emplace_back(prop);
		}
	}

	return nvTable;
}
#endif // !DEDICATED

#ifndef CLIENT_DLL
CNetVarTables::Table CNetVarTables::LoadTable(SendTable* /*sendTable*/)
{
	Table nvTable = Table{};
	return nvTable;
}
#endif // !CLIENT_DLL


#ifndef DEDICATED
int CNetVarTables::GetOffset(const string& tableName, const string& varName)
{
	for (const auto& table : vTables)
	{
		if (table.m_svNetTableName.compare(tableName) == 0)
			return GetOffset(table, varName);
	}

	return 0;
}

int CNetVarTables::GetOffset(const Table& table, const string& varName)
{
	for (const auto& prop : table.m_vChildProps)
	{
		string svVarName = prop->m_pVarName;
		if (svVarName.compare(varName) == 0)
			return table.m_Offset + prop->m_Offset;
	}

	for (const auto& child : table.m_vChildTables)
	{
		string svVarName = child.m_pProp->m_pVarName;
		if (svVarName.compare(varName) == 0)
			return table.m_Offset + child.m_Offset;
	}

	// Try to loop through sub tables.
	for (const auto& child : table.m_vChildTables)
	{
		int propOffset = GetOffset(child, varName);
		if (propOffset != 0)
			return table.m_Offset + propOffset;
	}

	return 0;
}

#endif // !DEDICATED