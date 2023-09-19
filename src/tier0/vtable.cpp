//===========================================================================//
//
// Purpose: Implementation of the CVTableHelper class, used to assist in
//          function rebuilding and reverse engineering.
//          DO NOT USE FOR SHIPPING CODE!!!!!!!!!!
//
//===========================================================================//
#include "tier0/vtable.h"

//-----------------------------------------------------------------------------
// Purpose: create class instance from passed module and virtual table name
// Input  : CModule* -
//          const std::string& -
//          uint32_t
//-----------------------------------------------------------------------------
CVTableHelper::CVTableHelper(CModule* module, const char* tableName, uint32_t refIndex)
	: m_svVirtualTableName(tableName)
{
	m_pVirtualTable = module->GetVirtualMethodTable(tableName, refIndex);
	m_nVirtualFunctionCount = GetVTableLength();
	GetAllVTableFunctions();
}


//-----------------------------------------------------------------------------
// Purpose: create class instance from passed pointer
// Input  : uintptr_t -
//          const std::string& -
//-----------------------------------------------------------------------------
CVTableHelper::CVTableHelper(uintptr_t virtualTable, const char* tableName)
	: m_pVirtualTable(virtualTable), m_svVirtualTableName(tableName)
{
	m_nVirtualFunctionCount = GetVTableLength();
	GetAllVTableFunctions();
}


//-----------------------------------------------------------------------------
// Purpose: create class instance from passed pointer
// Input  : void* -
//          const std::string& -
//-----------------------------------------------------------------------------
CVTableHelper::CVTableHelper(void* virtualTable, const char* tableName)
	: m_pVirtualTable(uintptr_t(virtualTable)), m_svVirtualTableName(tableName)
{
	m_nVirtualFunctionCount = GetVTableLength();
	GetAllVTableFunctions();
}

//-----------------------------------------------------------------------------
// Purpose: gets function count of m_pVirtualTable
// Output : ptrdiff_t
//-----------------------------------------------------------------------------
ptrdiff_t CVTableHelper::GetVTableLength()
{
	uintptr_t* pStartOfVTable = reinterpret_cast<uintptr_t*>(m_pVirtualTable);
	MEMORY_BASIC_INFORMATION memInfo = { NULL };
	ptrdiff_t vtSize = -1;

	do {
		vtSize++;
		VirtualQuery(reinterpret_cast<void*>(pStartOfVTable[vtSize]), &memInfo, sizeof(memInfo));
	} while (memInfo.Protect == PAGE_EXECUTE_READ || memInfo.Protect == PAGE_EXECUTE_READWRITE);

	return vtSize;
}

//-----------------------------------------------------------------------------
// Purpose: populate m_vVirtualFunctions with all virtual function pointers
//-----------------------------------------------------------------------------
void CVTableHelper::GetAllVTableFunctions()
{
	for (ptrdiff_t i = 0; i < m_nVirtualFunctionCount; i++)
	{
		m_vVirtualFunctions.push_back(*reinterpret_cast<uintptr_t*>(m_pVirtualTable + (8 * i)));
	}
}

//-----------------------------------------------------------------------------
// Purpose: call function from m_vVirtualFunctions with passed index
// Input  : int -
//          void* -
//          arg_list -
// Output : Assigned template return type
//-----------------------------------------------------------------------------
template <typename ReturnType, typename ...Args>
ReturnType CVTableHelper::Call(int index, void* thisPtr, Args... args)
{
	return reinterpret_cast<ReturnType(__fastcall*)(void*, Args...)>(m_vVirtualFunctions.at(index))(thisPtr, args...);
}