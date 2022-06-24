#ifndef MODULE_H
#define MODULE_H

class CModule
{
public:
	struct ModuleSections_t
	{
		ModuleSections_t(void) = default;
		ModuleSections_t(const string& svSectionName, uintptr_t pSectionBase, size_t nSectionSize) :
			m_svSectionName(svSectionName), m_pSectionBase(pSectionBase), m_nSectionSize(nSectionSize) {}

		bool IsSectionValid(void) const
		{
			return m_nSectionSize != 0;
		}

		string    m_svSectionName;           // Name of section.
		uintptr_t m_pSectionBase{};          // Start address of section.
		size_t    m_nSectionSize{};          // Size of section.
	};

	CModule(void) = default;
	CModule(const string& moduleName);
	CMemory FindPatternSIMD(const uint8_t* szPattern, const char* szMask) const;
	CMemory FindPatternSIMD(const string& svPattern) const;
	CMemory FindString(const string& string, const ptrdiff_t occurence = 1, bool nullTerminator = false) const;
	CMemory FindStringReadOnly(const string& svString, bool nullTerminator) const;

	CMemory          GetExportedFunction(const string& svFunctionName) const;
	ModuleSections_t GetSectionByName(const string& svSectionName) const;
	uintptr_t        GetModuleBase(void) const;
	DWORD            GetModuleSize(void) const;
	string           GetModuleName(void) const;

	ModuleSections_t         m_ExecutableCode;
	ModuleSections_t         m_ExceptionTable;
	ModuleSections_t         m_RunTimeData;
	ModuleSections_t         m_ReadOnlyData;

private:
	string                   m_svModuleName;
	uintptr_t                m_pModuleBase{};
	DWORD                    m_nModuleSize{};
	IMAGE_NT_HEADERS64*      m_pNTHeaders = nullptr;
	IMAGE_DOS_HEADER*        m_pDOSHeader = nullptr;
	vector<ModuleSections_t> m_vModuleSections;
};

#endif // MODULE_H