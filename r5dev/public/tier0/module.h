#ifndef MODULE_H
#define MODULE_H
#include "windows/tebpeb64.h"

class CModule
{
public:
	struct ModuleSections_t
	{
		ModuleSections_t(void) = default;
		ModuleSections_t(QWORD pSectionBase, size_t nSectionSize) :
			m_pSectionBase(pSectionBase), m_nSectionSize(nSectionSize) {}

		inline bool IsSectionValid(void) const { return m_nSectionSize != 0; }

		QWORD       m_pSectionBase; // Start address of section.
		size_t      m_nSectionSize; // Size of section.
	};
	typedef unordered_map<string, ModuleSections_t> ModuleSectionsMap_t;

	CModule(void) = default;
	CModule(const char* szModuleName);
	CModule(const QWORD nModuleBase);

	void InitFromName(const char* szModuleName);
	void InitFromBase(const QWORD nModuleBase);

	void LoadSections();

	CMemory FindPatternSIMD(const char* szPattern, const ModuleSections_t* moduleSection = nullptr) const;
	CMemory FindString(const char* szString, const ptrdiff_t occurrence = 1, bool nullTerminator = false) const;
	CMemory FindStringReadOnly(const char* szString, bool nullTerminator) const;
	CMemory FindFreeDataPage(const size_t nSize) const;

	CMemory          GetVirtualMethodTable(const char* szTableName, const size_t nRefIndex = 0);

	static CMemory   GetImportedSymbol(QWORD pModuleBase, const char* szModuleName, const char* szSymbolName, const bool bGetSymbolReference);
	inline CMemory   GetImportedSymbol(const char* szModuleName, const char* szSymbolName, const bool bGetSymbolReference) const
	{ return GetImportedSymbol(m_pModuleBase, szModuleName, szSymbolName, bGetSymbolReference); }

	static CMemory   GetExportedSymbol(QWORD pModuleBase, const char* szSymbolName);
	inline CMemory   GetExportedSymbol(const char* szSymbolName) const
	{ return GetExportedSymbol(m_pModuleBase, szSymbolName); }

	inline const CModule::ModuleSections_t& GetSectionByName(const char* szSectionName) const
	{ return m_ModuleSections.at(szSectionName); }

	inline const ModuleSectionsMap_t& GetSections() const { return m_ModuleSections; }
	inline QWORD         GetModuleBase(void) const { return m_pModuleBase; }
	inline DWORD         GetModuleSize(void) const { return m_nModuleSize; }
	inline const string& GetModuleName(void) const { return m_ModuleName; }
	inline QWORD         GetRVA(const QWORD nAddress) const { return (nAddress - GetModuleBase()); }


	inline        IMAGE_DOS_HEADER* GetDOSHeader() const { return GetDOSHeader(m_pModuleBase); }
	inline static IMAGE_DOS_HEADER* GetDOSHeader(QWORD pModuleBase)
	{ return reinterpret_cast<IMAGE_DOS_HEADER*>(pModuleBase); }

	inline        IMAGE_NT_HEADERS64* GetNTHeaders() const { return GetNTHeaders(m_pModuleBase); }
	inline static IMAGE_NT_HEADERS64* GetNTHeaders(QWORD pModuleBase)
	{ return reinterpret_cast<IMAGE_NT_HEADERS64*>(pModuleBase + GetDOSHeader(pModuleBase)->e_lfanew); }

	// https://en.wikipedia.org/wiki/Win32_Thread_Information_Block
	inline static PEB64* GetProcessEnvironmentBlock()
	{ return reinterpret_cast<PEB64*>(__readgsqword(0x60)); }

	inline static TEB64* GetThreadEnvironmentBlock()
	{ return reinterpret_cast<TEB64*>(NtCurrentTeb()); }

	void                 UnlinkFromPEB(void) const;

private:
	CMemory FindPatternSIMD(const uint8_t* pPattern, const char* szMask,
		const ModuleSections_t* moduleSection = nullptr, const size_t nOccurrence = 0) const;

	QWORD                m_pModuleBase;
	DWORD                m_nModuleSize;
	string               m_ModuleName;
	ModuleSectionsMap_t  m_ModuleSections;
};

#endif // MODULE_H