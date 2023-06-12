#ifndef MODULE_H
#define MODULE_H

class CModule
{
public:
	struct ModuleSections_t
	{
		ModuleSections_t(void) = default;
		ModuleSections_t(const char* sectionName, uintptr_t pSectionBase, size_t nSectionSize) :
			m_SectionName(sectionName), m_pSectionBase(pSectionBase), m_nSectionSize(nSectionSize) {}

		inline bool IsSectionValid(void) const { return m_nSectionSize != 0; }

		string    m_SectionName;           // Name of section.
		uintptr_t m_pSectionBase;          // Start address of section.
		size_t    m_nSectionSize;          // Size of section.
	};

	CModule(void) = default;
	CModule(const char* szModuleName);
	CModule(const char* szModuleName, const uintptr_t nModuleBase);

	void Init();
	void LoadSections();
#ifndef PLUGINSDK
	CMemory FindPatternSIMD(const char* szPattern, const ModuleSections_t* moduleSection = nullptr) const;
	CMemory FindString(const char* szString, const ptrdiff_t occurrence = 1, bool nullTerminator = false) const;
	CMemory FindStringReadOnly(const char* szString, bool nullTerminator) const;
	CMemory FindFreeDataPage(const size_t nSize) const;

	CMemory          GetVirtualMethodTable(const char* szTableName, const size_t nRefIndex = 0);
#endif // !PLUGINSDK
	CMemory          GetImportedFunction(const char* szModuleName, const char* szFunctionName, const bool bGetFunctionReference) const;
	CMemory          GetExportedFunction(const char* szFunctionName) const;
	ModuleSections_t GetSectionByName(const char* szSectionName) const;

	inline const vector<CModule::ModuleSections_t>& GetSections() const { return m_ModuleSections; }
	inline uintptr_t     GetModuleBase(void) const { return m_pModuleBase; }
	inline DWORD         GetModuleSize(void) const { return m_nModuleSize; }
	inline const string& GetModuleName(void) const { return m_ModuleName; }
	inline uintptr_t     GetRVA(const uintptr_t nAddress) const { return (nAddress - GetModuleBase()); }

	void                 UnlinkFromPEB(void) const;

	IMAGE_NT_HEADERS64*      m_pNTHeaders;
	IMAGE_DOS_HEADER*        m_pDOSHeader;

	ModuleSections_t         m_ExecutableCode;
	ModuleSections_t         m_ExceptionTable;
	ModuleSections_t         m_RunTimeData;
	ModuleSections_t         m_ReadOnlyData;

private:
	CMemory FindPatternSIMD(const uint8_t* pPattern, const char* szMask,
		const ModuleSections_t* moduleSection = nullptr, const size_t nOccurrence = 0) const;

	string                   m_ModuleName;
	uintptr_t                m_pModuleBase;
	DWORD                    m_nModuleSize;
	vector<ModuleSections_t> m_ModuleSections;
};

#endif // MODULE_H