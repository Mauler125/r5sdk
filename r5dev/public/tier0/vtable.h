#pragma once

class CVTableHelper
{
public:

	CVTableHelper(void) = default;
	CVTableHelper(CModule* module, const char* tableName = "", uint32_t refIndex = 0);
	CVTableHelper(uintptr_t virtualTable, const char* tableName = "");
	CVTableHelper(void* virtualTable, const char* tableName = "");

	inline operator uintptr_t(void) const
	{
		return m_pVirtualTable;
	}

	inline operator void* (void) const
	{
		return reinterpret_cast<void*>(m_pVirtualTable);
	}

	inline operator bool(void) const
	{
		return m_pVirtualTable != NULL && !m_vVirtualFunctions.empty();
	}

	uintptr_t GetVirtualFunctionTable()
	{
		return m_pVirtualTable;
	}

	ptrdiff_t GetVirtualFunctionCount()
	{
		return m_nVirtualFunctionCount;
	}

	const string& GetVTableName()
	{
		return m_svVirtualTableName;
	}

	template <typename ReturnType, typename ...Args>
	ReturnType Call(int index, void* thisPtr, Args... args);

private:
	void GetAllVTableFunctions();
	ptrdiff_t GetVTableLength();

	uintptr_t m_pVirtualTable;
	string m_svVirtualTableName;
	ptrdiff_t m_nVirtualFunctionCount;
	vector<uintptr_t> m_vVirtualFunctions;
};