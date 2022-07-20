//===========================================================================//
//
// Purpose: Implementation of the CModule class.
//
//===========================================================================//
#include "core/stdafx.h"
#include "public/include/utility.h"
#include "public/include/memaddr.h"

//-----------------------------------------------------------------------------
// Purpose: constructor
// Input  : *svModuleName
//-----------------------------------------------------------------------------
CModule::CModule(const string& svModuleName) : m_svModuleName(svModuleName)
{
	const MODULEINFO mInfo = GetModuleInfo(svModuleName.c_str());
	m_nModuleSize = static_cast<size_t>(mInfo.SizeOfImage);
	m_pModuleBase = reinterpret_cast<uintptr_t>(mInfo.lpBaseOfDll);

	m_pDOSHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(m_pModuleBase);
	m_pNTHeaders = reinterpret_cast<IMAGE_NT_HEADERS64*>(m_pModuleBase + m_pDOSHeader->e_lfanew);

	const IMAGE_SECTION_HEADER* hSection = IMAGE_FIRST_SECTION(m_pNTHeaders); // Get first image section.

	for (WORD i = 0; i < m_pNTHeaders->FileHeader.NumberOfSections; i++) // Loop through the sections.
	{
		const IMAGE_SECTION_HEADER& hCurrentSection = hSection[i]; // Get current section.
		m_vModuleSections.push_back(ModuleSections_t(string(reinterpret_cast<const char*>(hCurrentSection.Name)),
			static_cast<uintptr_t>(m_pModuleBase + hCurrentSection.VirtualAddress), hCurrentSection.SizeOfRawData)); // Push back a struct with the section data.
	}

	m_ExecutableCode = GetSectionByName(".text");
	m_ExceptionTable = GetSectionByName(".pdata");
	m_RunTimeData = GetSectionByName(".data");
	m_ReadOnlyData = GetSectionByName(".rdata");
}

//-----------------------------------------------------------------------------
// Purpose: find array of bytes in process memory using SIMD instructions
// Input  : *szPattern - 
//          *szMask - 
// Output : CMemory
//-----------------------------------------------------------------------------
CMemory CModule::FindPatternSIMD(const uint8_t* szPattern, const char* szMask, ModuleSections_t moduleSection) const
{
	if (!m_ExecutableCode.IsSectionValid())
		return CMemory();

	uint64_t nBase = static_cast<uint64_t>(m_ExecutableCode.m_pSectionBase);
	uint64_t nSize = static_cast<uint64_t>(m_ExecutableCode.m_nSectionSize);

	if (moduleSection.IsSectionValid())
	{
		nBase = static_cast<uint64_t>(moduleSection.m_pSectionBase);
		nSize = static_cast<uint64_t>(moduleSection.m_nSectionSize);
	}

	const uint8_t* pData = reinterpret_cast<uint8_t*>(nBase);
	const uint8_t* pEnd = pData + static_cast<uint32_t>(nSize) - strlen(szMask);

	int nMasks[64]; // 64*16 = enough masks for 1024 bytes.
	const int iNumMasks = static_cast<int>(ceil(static_cast<float>(strlen(szMask)) / 16.f));

	memset(nMasks, '\0', iNumMasks * sizeof(int));
	for (intptr_t i = 0; i < iNumMasks; ++i)
	{
		for (intptr_t j = strnlen(szMask + i * 16, 16) - 1; j >= 0; --j)
		{
			if (szMask[i * 16 + j] == 'x')
			{
				_bittestandset(reinterpret_cast<LONG*>(&nMasks[i]), j);
			}
		}
	}
	const __m128i xmm1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(szPattern));
	__m128i xmm2, xmm3, msks;
	for (; pData != pEnd; _mm_prefetch(reinterpret_cast<const char*>(++pData + 64), _MM_HINT_NTA))
	{
		if (szPattern[0] == pData[0])
		{
			xmm2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pData));
			msks = _mm_cmpeq_epi8(xmm1, xmm2);
			if ((_mm_movemask_epi8(msks) & nMasks[0]) == nMasks[0])
			{
				for (uintptr_t i = 1; i < static_cast<uintptr_t>(iNumMasks); ++i)
				{
					xmm2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>((pData + i * 16)));
					xmm3 = _mm_loadu_si128(reinterpret_cast<const __m128i*>((szPattern + i * 16)));
					msks = _mm_cmpeq_epi8(xmm2, xmm3);
					if ((_mm_movemask_epi8(msks) & nMasks[i]) == nMasks[i])
					{
						if ((i + 1) == iNumMasks)
						{
							return static_cast<CMemory>(const_cast<uint8_t*>(pData));
						}
					}
					else
					{
						goto cont;
					}
				}
				return static_cast<CMemory>((&*(const_cast<uint8_t*>(pData))));
			}
		}cont:;
	}
	return CMemory();
}


//-----------------------------------------------------------------------------
// Purpose: find array of bytes in process memory using SIMD instructions
// Input  : *szPattern
// Output : CMemory
//-----------------------------------------------------------------------------
CMemory CModule::FindPatternSIMD(const string& svPattern, ModuleSections_t moduleSection) const
{
	const pair patternInfo = PatternToMaskedBytes(svPattern);
	return FindPatternSIMD(patternInfo.first.data(), patternInfo.second.c_str(), moduleSection);
}

//-----------------------------------------------------------------------------
// Purpose: find address of input string constant in read only memory
// Input  : *svString - 
//          bNullTerminator - 
// Output : CMemory
//-----------------------------------------------------------------------------
CMemory CModule::FindStringReadOnly(const string& svString, bool bNullTerminator) const
{
	if (!m_ReadOnlyData.IsSectionValid())
		return CMemory();

	const vector<int> vBytes = StringToBytes(svString, bNullTerminator); // Convert our string to a byte array.
	const pair bytesInfo = std::make_pair(vBytes.size(), vBytes.data()); // Get the size and data of our bytes.

	uint8_t* pBase = reinterpret_cast<uint8_t*>(m_ReadOnlyData.m_pSectionBase); // Get start of .rdata section.

	for (size_t i = 0ull; i < m_ReadOnlyData.m_nSectionSize - bytesInfo.first; i++)
	{
		bool bFound = true;

		// If either the current byte equals to the byte in our pattern or our current byte in the pattern is a wildcard
		// our if clause will be false.
		for (size_t j = 0ull; j < bytesInfo.first; j++)
		{
			if (pBase[i + j] != bytesInfo.second[j] && bytesInfo.second[j] != -1)
			{
				bFound = false;
				break;
			}
		}

		if (bFound)
		{
			return CMemory(&pBase[i]);
		}
	}

	return CMemory();
}

//-----------------------------------------------------------------------------
// Purpose: find address of reference to string constant in executable memory
// Input  : *svString - 
//          bNullTerminator - 
// Output : CMemory
//-----------------------------------------------------------------------------
CMemory CModule::FindString(const string& svString, const ptrdiff_t nOccurence, bool bNullTerminator) const
{
	if (!m_ExecutableCode.IsSectionValid())
		return CMemory();

	const CMemory stringAddress = FindStringReadOnly(svString, bNullTerminator); // Get Address for the string in the .rdata section.
	if (!stringAddress)
		return CMemory();

	uint8_t* pLatestOccurence = nullptr;
	uint8_t* pTextStart = reinterpret_cast<uint8_t*>(m_ExecutableCode.m_pSectionBase); // Get the start of the .text section.
	ptrdiff_t dOccurencesFound = 0;

	for (size_t i = 0ull; i < m_ExecutableCode.m_nSectionSize - 0x5; i++)
	{
		byte byte = pTextStart[i];
		if (byte == LEA)
		{
			const CMemory skipOpCode = CMemory(reinterpret_cast<uintptr_t>(&pTextStart[i])).OffsetSelf(0x2); // Skip next 2 opcodes, those being the instruction and the register.
			const int32_t relativeAddress = skipOpCode.GetValue<int32_t>();                                  // Get 4-byte long string relative Address
			const uintptr_t nextInstruction = skipOpCode.Offset(0x4).GetPtr();                               // Get location of next instruction.
			const CMemory potentialLocation = CMemory(nextInstruction + relativeAddress);                    // Get potential string location.

			if (potentialLocation == stringAddress)
			{
				dOccurencesFound++;
				if (nOccurence == dOccurencesFound)
					return CMemory(&pTextStart[i]);

				pLatestOccurence = &pTextStart[i]; // Stash latest occurence.
			}
		}
	}
	return CMemory(pLatestOccurence);
}

//-----------------------------------------------------------------------------
// Purpose: get address of exported function in this module
// Input  : *svFunctionName - 
//          bNullTerminator - 
// Output : CMemory
//-----------------------------------------------------------------------------
CMemory CModule::GetExportedFunction(const string& svFunctionName) const
{
	if (!m_pDOSHeader || m_pDOSHeader->e_magic != IMAGE_DOS_SIGNATURE) // Is dosHeader valid?
		return CMemory();

	if (!m_pNTHeaders || m_pNTHeaders->Signature != IMAGE_NT_SIGNATURE) // Is ntHeader valid?
		return CMemory();

	// Get the location of IMAGE_EXPORT_DIRECTORY for this module by adding the IMAGE_DIRECTORY_ENTRY_EXPORT relative virtual address onto our module base address.
	const IMAGE_EXPORT_DIRECTORY* pImageExportDirectory = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(m_pModuleBase + m_pNTHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
	if (!pImageExportDirectory)
		return CMemory();

	// Are there any exported functions?
	if (!pImageExportDirectory->NumberOfFunctions)
		return CMemory();

	// Get the location of the functions via adding the relative virtual address from the struct into our module base address.
	const DWORD* pAddressOfFunctions = reinterpret_cast<DWORD*>(m_pModuleBase + pImageExportDirectory->AddressOfFunctions);
	if (!pAddressOfFunctions)
		return CMemory();

	// Get the names of the functions via adding the relative virtual address from the struct into our module base Address.
	const DWORD* pAddressOfName = reinterpret_cast<DWORD*>(m_pModuleBase + pImageExportDirectory->AddressOfNames);
	if (!pAddressOfName)
		return CMemory();

	// Get the ordinals of the functions via adding the relative virtual Address from the struct into our module base address.
	DWORD* pAddressOfOrdinals = reinterpret_cast<DWORD*>(m_pModuleBase + pImageExportDirectory->AddressOfNameOrdinals);
	if (!pAddressOfOrdinals)
		return CMemory();

	for (DWORD i = 0; i < pImageExportDirectory->NumberOfFunctions; i++) // Iterate through all the functions.
	{
		// Get virtual relative Address of the function name. Then add module base Address to get the actual location.
		string ExportFunctionName = reinterpret_cast<char*>(reinterpret_cast<DWORD*>(m_pModuleBase + pAddressOfName[i]));

		if (ExportFunctionName.compare(svFunctionName) == 0) // Is this our wanted exported function?
		{
			// Get the function ordinal. Then grab the relative virtual address of our wanted function. Then add module base address so we get the actual location.
			return CMemory(m_pModuleBase + pAddressOfFunctions[reinterpret_cast<WORD*>(pAddressOfOrdinals)[i]]); // Return as CMemory class.
		}
	}
	return CMemory();
}

//-----------------------------------------------------------------------------
// Purpose: get address of a virtual method table by rtti type descriptor name.
// Input  : *tableName - 
// Output : CMemory
//-----------------------------------------------------------------------------
CMemory CModule::GetVirtualMethodTable(const std::string& tableName)
{
	const auto tableNameInfo = StringToMaskedBytes(tableName, false);
	CMemory rttiTypeDescriptor = FindPatternSIMD(tableNameInfo.first.data(), tableNameInfo.second.c_str(), {".data", m_RunTimeData.m_pSectionBase, m_RunTimeData.m_nSectionSize}).OffsetSelf(-0x10);
	if (!rttiTypeDescriptor)
		return CMemory();

	vector <CMemory> rttiTDRef = {};
	uintptr_t scanStart = m_ReadOnlyData.m_pSectionBase; // Get the start address of our scan.

	const uintptr_t scanEnd = (m_ReadOnlyData.m_pSectionBase + m_ReadOnlyData.m_nSectionSize) - 0x4; // Calculate the end of our scan.
	const uintptr_t rttiTDRva = rttiTypeDescriptor.GetPtr() - m_pModuleBase; // The RTTI gets referenced by a 4-Byte RVA address. We need to scan for that address.
	while (scanStart < scanEnd)
	{
		CMemory reference = FindPatternSIMD(reinterpret_cast<rsig_t>(&rttiTDRva), "xxxx", {".rdata", scanStart, m_ReadOnlyData.m_nSectionSize});
		if (!reference)
			break;

		rttiTDRef.push_back(reference);
		scanStart = reference.Offset(0x4).GetPtr(); // Set location to current reference + 0x4 so we avoid pushing it back again into the vector.
	}

	for (const CMemory& ref : rttiTDRef)
	{
		CMemory referenceOffset = ref.Offset(-0xC);
		if (referenceOffset.GetValue<int32_t>() != 1) // Check if we got a RTTI Object Locator for this reference by checking if -0xC is 1, which is the 'signature' field which is always 1 on x64.
			continue;

		return FindPatternSIMD(reinterpret_cast<rsig_t>(&referenceOffset), "xxxx", { ".rdata", m_ReadOnlyData.m_pSectionBase, m_ReadOnlyData.m_nSectionSize }).OffsetSelf(0x8);
	}

	return CMemory();
}

//-----------------------------------------------------------------------------
// Purpose: get the module section by name (example: '.rdata', '.text')
// Input  : *svModuleName - 
// Output : ModuleSections_t
//-----------------------------------------------------------------------------
CModule::ModuleSections_t CModule::GetSectionByName(const string& svSectionName) const
{
	for (size_t i = 0; i < m_vModuleSections.size(); i++)
	{
		if (m_vModuleSections[i].m_svSectionName.compare(svSectionName) == 0)
			return m_vModuleSections[i];
	}

	return ModuleSections_t();
}

//-----------------------------------------------------------------------------
// Purpose: returns the module base
//-----------------------------------------------------------------------------
uintptr_t CModule::GetModuleBase(void) const
{
	return m_pModuleBase;
}

//-----------------------------------------------------------------------------
// Purpose: returns the module size
//-----------------------------------------------------------------------------
DWORD CModule::GetModuleSize(void) const
{
	return m_nModuleSize;
}

//-----------------------------------------------------------------------------
// Purpose: returns the module name
//-----------------------------------------------------------------------------
string CModule::GetModuleName(void) const
{
	return m_svModuleName;
}
