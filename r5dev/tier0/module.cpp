//===========================================================================//
//
// Purpose: Implementation of the CModule class.
//
//===========================================================================//
#include "tier0/memaddr.h"
#include "tier0/sigcache.h"

//-----------------------------------------------------------------------------
// Purpose: constructor
// Input  : *szModuleName - 
//			bDynamicInit  - set this to false if there is no memory allocator
//-----------------------------------------------------------------------------
CModule::CModule(const char* szModuleName, const bool bDynamicInit)
	: m_ModuleName(szModuleName)
{
	m_pModuleBase = reinterpret_cast<QWORD>(GetModuleHandleA(szModuleName));
	Init(bDynamicInit);
}

//-----------------------------------------------------------------------------
// Purpose: constructor
// Input  : *szModuleName - 
//			nModuleBase   - 
//-----------------------------------------------------------------------------
CModule::CModule(const char* szModuleName, const QWORD nModuleBase,
	const bool bDynamicInit)
	: m_ModuleName(szModuleName)
	, m_pModuleBase(nModuleBase)
{
	Init(bDynamicInit);
}

//-----------------------------------------------------------------------------
// Purpose: initializes module descriptors
//-----------------------------------------------------------------------------
void CModule::Init(const bool bInitSections)
{
	m_nModuleSize = static_cast<size_t>(GetNTHeaders()->OptionalHeader.SizeOfImage);

	if (bInitSections)
	{
		LoadSections();
	}
}

//-----------------------------------------------------------------------------
// Purpose: initializes the default executable segments
//-----------------------------------------------------------------------------
void CModule::LoadSections()
{
	const IMAGE_NT_HEADERS64* pNTHeaders = GetNTHeaders();
	const IMAGE_SECTION_HEADER* hSection = IMAGE_FIRST_SECTION(pNTHeaders);

	for (WORD i = 0; i < pNTHeaders->FileHeader.NumberOfSections; i++)
	{
		// Capture each section.
		const IMAGE_SECTION_HEADER& hCurrentSection = hSection[i];
		m_ModuleSections.emplace(reinterpret_cast<const char*>(hCurrentSection.Name),
			ModuleSections_t(static_cast<QWORD>
			(m_pModuleBase + hCurrentSection.VirtualAddress), hCurrentSection.SizeOfRawData));
	}
}

//-----------------------------------------------------------------------------
// Purpose: find array of bytes in process memory using SIMD instructions
// Input  : *pPattern      - 
//          *szMask        - 
//          *moduleSection - 
//          nOccurrence    - 
// Output : CMemory
//-----------------------------------------------------------------------------
CMemory CModule::FindPatternSIMD(const uint8_t* pPattern, const char* szMask,
	const ModuleSections_t* moduleSection, const size_t nOccurrence) const
{
	const ModuleSections_t& executableCode = GetSectionByName(".text");

	if (!executableCode.IsSectionValid())
		return nullptr;

	const bool bSectionValid = moduleSection ? moduleSection->IsSectionValid() : false;

	const QWORD nBase = bSectionValid ?
		moduleSection->m_pSectionBase : executableCode.m_pSectionBase;
	const QWORD nSize = bSectionValid ?
		moduleSection->m_nSectionSize : executableCode.m_nSectionSize;

	const size_t nMaskLen = strlen(szMask);
	const uint8_t* pData = reinterpret_cast<uint8_t*>(nBase);
	const uint8_t* pEnd = pData + nSize - nMaskLen;

	size_t nOccurrenceCount = 0;
	int nMasks[64]; // 64*16 = enough masks for 1024 bytes.
	const int iNumMasks = static_cast<int>(ceil(static_cast<float>(nMaskLen) / 16.f));

	memset(nMasks, '\0', iNumMasks * sizeof(int));
	for (intptr_t i = 0; i < iNumMasks; ++i)
	{
		for (intptr_t j = strnlen(szMask + i * 16, 16) - 1; j >= 0; --j)
		{
			if (szMask[i * 16 + j] == 'x')
			{
				_bittestandset(reinterpret_cast<LONG*>(&nMasks[i]), static_cast<LONG>(j));
			}
		}
	}
	const __m128i xmm1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pPattern));
	__m128i xmm2, xmm3, msks;
	for (; pData != pEnd; _mm_prefetch(reinterpret_cast<const char*>(++pData + 64), _MM_HINT_NTA))
	{
		if (pPattern[0] == pData[0])
		{
			xmm2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pData));
			msks = _mm_cmpeq_epi8(xmm1, xmm2);
			if ((_mm_movemask_epi8(msks) & nMasks[0]) == nMasks[0])
			{
				for (uintptr_t i = 1; i < static_cast<uintptr_t>(iNumMasks); ++i)
				{
					xmm2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>((pData + i * 16)));
					xmm3 = _mm_loadu_si128(reinterpret_cast<const __m128i*>((pPattern + i * 16)));
					msks = _mm_cmpeq_epi8(xmm2, xmm3);
					if ((_mm_movemask_epi8(msks) & nMasks[i]) == nMasks[i])
					{
						if ((i + 1) == iNumMasks)
						{
							if (nOccurrenceCount == nOccurrence)
							{
								return static_cast<CMemory>(const_cast<uint8_t*>(pData));
							}
							nOccurrenceCount++;
						}
					}
					else
					{
						goto cont;
					}
				}
				if (nOccurrenceCount == nOccurrence)
				{
					return static_cast<CMemory>((&*(const_cast<uint8_t*>(pData))));
				}
				nOccurrenceCount++;
			}
		}cont:;
	}
	return nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: find a string pattern in process memory using SIMD instructions
// Input  : *szPattern     - 
//          *moduleSection - 
// Output : CMemory
//-----------------------------------------------------------------------------
CMemory CModule::FindPatternSIMD(const char* szPattern,
	const ModuleSections_t* moduleSection) const
{
	uint64_t nRVA;
	if (g_SigCache.FindEntry(szPattern, nRVA))
	{
		return CMemory(nRVA + GetModuleBase());
	}

	const pair<vector<uint8_t>, string>
		patternInfo = PatternToMaskedBytes(szPattern);

	const CMemory memory = FindPatternSIMD(patternInfo.first.data(),
		patternInfo.second.c_str(), moduleSection);

	g_SigCache.AddEntry(szPattern, GetRVA(memory.GetPtr()));
	return memory;
}

//-----------------------------------------------------------------------------
// Purpose: find address of reference to string constant in executable memory
// Input  : *szString       - 
//          bNullTerminator - 
// Output : CMemory
//-----------------------------------------------------------------------------
CMemory CModule::FindString(const char* szString, const ptrdiff_t nOccurrence,
	bool bNullTerminator) const
{
	const ModuleSections_t& executableCode = GetSectionByName(".text");

	if (!executableCode.IsSectionValid())
		return nullptr;

	uint64_t nRVA;
	string svPackedString = szString + std::to_string(nOccurrence);

	if (g_SigCache.FindEntry(svPackedString.c_str(), nRVA))
	{
		return CMemory(nRVA + GetModuleBase());
	}

	// Get Address for the string in the .rdata section.
	const CMemory stringAddress = FindStringReadOnly(szString, bNullTerminator);

	if (!stringAddress)
		return nullptr;

	// Get the start of the .text section.
	uint8_t* pTextStart = reinterpret_cast<uint8_t*>(executableCode.m_pSectionBase);
	uint8_t* pLatestOccurrence = nullptr;
	ptrdiff_t dOccurrencesFound = 0;
	CMemory resultAddress;

	for (size_t i = 0ull; i < executableCode.m_nSectionSize - 0x5; i++)
	{
		byte byte = pTextStart[i];
		if (byte == LEA)
		{
			// Skip next 2 opcodes, those being the instruction and the register.
			const CMemory skipOpCode = CMemory(reinterpret_cast<
				QWORD>(&pTextStart[i])).OffsetSelf(0x2);

			// Get 4-byte long string relative Address
			const int32_t relativeAddress = skipOpCode.GetValue<int32_t>();
			// Get location of next instruction.
			const QWORD nextInstruction = skipOpCode.Offset(0x4).GetPtr();
			// Get potential string location.
			const CMemory potentialLocation = CMemory(nextInstruction + relativeAddress);

			if (potentialLocation == stringAddress)
			{
				dOccurrencesFound++;
				if (nOccurrence == dOccurrencesFound)
				{
					resultAddress = CMemory(&pTextStart[i]);
					g_SigCache.AddEntry(svPackedString.c_str(), GetRVA(resultAddress.GetPtr()));

					return resultAddress;
				}

				pLatestOccurrence = &pTextStart[i]; // Stash latest occurrence.
			}
		}
	}

	resultAddress = CMemory(pLatestOccurrence);
	g_SigCache.AddEntry(svPackedString.c_str(), GetRVA(resultAddress.GetPtr()));

	return resultAddress;
}

//-----------------------------------------------------------------------------
// Purpose: find address of input string constant in read only memory
// Input  : *szString       - 
//          bNullTerminator - 
// Output : CMemory
//-----------------------------------------------------------------------------
CMemory CModule::FindStringReadOnly(const char* szString, bool bNullTerminator) const
{
	const ModuleSections_t& readOnlyData = GetSectionByName(".rdata");

	if (!readOnlyData.IsSectionValid())
		return nullptr;

	uint64_t nRVA;
	if (g_SigCache.FindEntry(szString, nRVA))
	{
		return CMemory(nRVA + GetModuleBase());
	}

	// Convert our string to a byte array.
	const vector<int> vBytes = StringToBytes(szString, bNullTerminator);
	const pair<size_t, const int*> bytesInfo = std::make_pair<
		size_t, const int*>(vBytes.size(), vBytes.data()); // Get the size and data of our bytes.

	// Get start of .rdata section.
	const uint8_t* pBase = reinterpret_cast<uint8_t*>(readOnlyData.m_pSectionBase);

	for (size_t i = 0ull; i < readOnlyData.m_nSectionSize - bytesInfo.first; i++)
	{
		bool bFound = true;

		// If either the current byte equals to the byte in
		// our pattern or our current byte in the pattern is
		// a wildcard our if clause will be false.
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
			CMemory result = CMemory(&pBase[i]);
			g_SigCache.AddEntry(szString, GetRVA(result.GetPtr()));

			return result;
		}
	}

	return nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: find 'free' page in r/w/x sections
// Input  : nSize - 
// Output : CMemory
//-----------------------------------------------------------------------------
CMemory CModule::FindFreeDataPage(const size_t nSize) const
{
	auto checkDataSection = [](const void* address, const std::size_t size)
	{
		MEMORY_BASIC_INFORMATION membInfo = { 0 };

		VirtualQuery(address, &membInfo, sizeof(membInfo));

		if (membInfo.AllocationBase && membInfo.BaseAddress &&
			membInfo.State == MEM_COMMIT &&
			!(membInfo.Protect & PAGE_GUARD) &&
			membInfo.Protect != PAGE_NOACCESS)
		{
			if ((membInfo.Protect &
				(PAGE_EXECUTE_READWRITE | PAGE_READWRITE)) &&
				membInfo.RegionSize >= size)
			{
				return ((membInfo.Protect &
					(PAGE_EXECUTE_READWRITE | PAGE_READWRITE))
					&& membInfo.RegionSize >= size) ? true : false;
			}
		}
		return false;
	};

	// This is very unstable, this doesn't check
	// for the actual 'page' sizes. Also can be
	// optimized to search per 'section'.
	const QWORD endOfModule = m_pModuleBase
		+ GetNTHeaders()->OptionalHeader.SizeOfImage - sizeof(QWORD);

	for (QWORD currAddr = endOfModule;
		m_pModuleBase < currAddr; currAddr -= sizeof(QWORD))
	{
		if (*reinterpret_cast<QWORD*>(currAddr) == 0 &&
			checkDataSection(reinterpret_cast<void*>(currAddr), nSize))
		{
			bool bIsGoodPage = true;
			uint32_t nPageCount = 0;

			for (; nPageCount < nSize && bIsGoodPage;
				nPageCount += sizeof(QWORD))
			{
				const QWORD pageData =*reinterpret_cast<
					QWORD*>(currAddr + nPageCount);

				if (pageData != 0)
					bIsGoodPage = false;
			}

			if (bIsGoodPage && nPageCount >= nSize)
				return currAddr;
		}
	}

	return nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: get address of a virtual method table by rtti type descriptor name.
// Input  : *szTableName - 
//          nRefIndex    - 
// Output : address of virtual method table, null if not found.
//-----------------------------------------------------------------------------
CMemory CModule::GetVirtualMethodTable(const char* szTableName, const size_t nRefIndex)
{
	// Packed together as we can have multiple VFTable
	// searches, but with different ref indices.
	uint64_t nRVA;
	string svPackedTableName = szTableName + std::to_string(nRefIndex);

	if (g_SigCache.FindEntry(svPackedTableName.c_str(), nRVA))
	{
		return CMemory(nRVA + GetModuleBase());
	}

	const ModuleSections_t& readOnlyData = GetSectionByName(".rdata");
	const ModuleSections_t& dataSection = GetSectionByName(".data");

	const auto tableNameInfo = StringToMaskedBytes(szTableName, false);
	CMemory rttiTypeDescriptor = FindPatternSIMD(tableNameInfo.first.data(),
		tableNameInfo.second.c_str(), &dataSection).OffsetSelf(-0x10);

	if (!rttiTypeDescriptor)
		return nullptr;

	QWORD scanStart = readOnlyData.m_pSectionBase;
	const QWORD scanEnd = (readOnlyData.m_pSectionBase + readOnlyData.m_nSectionSize) - 0x4;

	// The RTTI gets referenced by a 4-Byte RVA
	// address, we need to scan for that address.
	const QWORD rttiTDRva = rttiTypeDescriptor.GetPtr() - m_pModuleBase;
	ModuleSections_t moduleSection;

	while (scanStart < scanEnd)
	{
		moduleSection = { scanStart, readOnlyData.m_nSectionSize };

		CMemory reference = FindPatternSIMD(reinterpret_cast<rsig_t>(
			&rttiTDRva), "xxxx", &moduleSection, nRefIndex);

		if (!reference)
			break;
		
		CMemory referenceOffset = reference.Offset(-0xC);

		// Check if we got a RTTI Object Locator for this
		// reference by checking if -0xC is 1, which is the
		// 'signature' field which is always 1 on x64.
		if (referenceOffset.GetValue<int32_t>() != 1)
		{
			// Set location to current reference + 0x4 so we
			// avoid pushing it back again into the vector.
			scanStart = reference.Offset(0x4).GetPtr();
			continue;
		}

		moduleSection = {readOnlyData.m_pSectionBase, readOnlyData.m_nSectionSize };

		CMemory vfTable = FindPatternSIMD(reinterpret_cast<rsig_t>(
			&referenceOffset), "xxxxxxxx", &moduleSection).OffsetSelf(0x8);

		g_SigCache.AddEntry(svPackedTableName.c_str(), GetRVA(vfTable.GetPtr()));
		return vfTable;
	}

	return CMemory();
}

//-----------------------------------------------------------------------------
// Purpose: get address of imported function in target module
// Input  : *szModuleName       - 
//          *szSymbolName       - 
//          bGetSymbolReference - 
// Output : CMemory
//-----------------------------------------------------------------------------
CMemory CModule::GetImportedSymbol(QWORD pModuleBase, const char* szModuleName,
	const char* szSymbolName, const bool bGetSymbolReference)
{
	IMAGE_DOS_HEADER* pDOSHeader = GetDOSHeader(pModuleBase);
	IMAGE_NT_HEADERS64* pNTHeaders = GetNTHeaders(pModuleBase);

	if (!pDOSHeader || pDOSHeader->e_magic != IMAGE_DOS_SIGNATURE)
		return nullptr;

	if (!pNTHeaders || pNTHeaders->Signature != IMAGE_NT_SIGNATURE)
		return nullptr;

	// Get the location of IMAGE_IMPORT_DESCRIPTOR for this
	// module by adding the IMAGE_DIRECTORY_ENTRY_IMPORT
	// relative virtual address onto our module base address.
	IMAGE_IMPORT_DESCRIPTOR* pImageImportDescriptors = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>
		(pModuleBase + pNTHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

	if (!pImageImportDescriptors)
		return nullptr;

	for (IMAGE_IMPORT_DESCRIPTOR* pIID = pImageImportDescriptors; pIID->Name != 0; pIID++)
	{
		// Get virtual relative Address of the imported module name.
		// Then add module base Address to get the actual location.
		const char* szImportedModuleName = reinterpret_cast<char*>(reinterpret_cast<DWORD*>(pModuleBase + pIID->Name));

		if (stricmp(szImportedModuleName, szModuleName) == NULL)
		{
			IMAGE_THUNK_DATA* pOgFirstThunk = reinterpret_cast<IMAGE_THUNK_DATA*>(pModuleBase + pIID->OriginalFirstThunk);

			// To get actual function address.
			IMAGE_THUNK_DATA* pFirstThunk = reinterpret_cast<IMAGE_THUNK_DATA*>(pModuleBase + pIID->FirstThunk);
			for (; pOgFirstThunk->u1.AddressOfData; ++pOgFirstThunk, ++pFirstThunk)
			{
				// Get image import by name.
				const IMAGE_IMPORT_BY_NAME* pImageImportByName = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(
					pModuleBase + pOgFirstThunk->u1.AddressOfData);

				if (strcmp(pImageImportByName->Name, szSymbolName) == NULL)
				{
					// Grab function address from firstThunk.
					QWORD* pFunctionAddress = &pFirstThunk->u1.Function;

					// Reference or address?
					return bGetSymbolReference ? CMemory(pFunctionAddress) : CMemory(*pFunctionAddress);
				}
			}
		}
	}
	return nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: get address of exported symbol in this module
// Input  : *pModuleBase - 
//          szSymbolName - 
// Output : CMemory
//-----------------------------------------------------------------------------
CMemory CModule::GetExportedSymbol(QWORD pModuleBase, const char* szSymbolName)
{
	IMAGE_DOS_HEADER* pDOSHeader = GetDOSHeader(pModuleBase);
	IMAGE_NT_HEADERS64* pNTHeaders = GetNTHeaders(pModuleBase);

	if (!pDOSHeader || pDOSHeader->e_magic != IMAGE_DOS_SIGNATURE)
		return nullptr;

	if (!pNTHeaders || pNTHeaders->Signature != IMAGE_NT_SIGNATURE)
		return nullptr;

	// Get the location of IMAGE_EXPORT_DIRECTORY for this
	// module by adding the IMAGE_DIRECTORY_ENTRY_EXPORT
	// relative virtual address onto our module base address.
	const IMAGE_EXPORT_DIRECTORY* pImageExportDirectory = 
		reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(pModuleBase
		+ pNTHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);

	if (!pImageExportDirectory)
		return nullptr;

	if (!pImageExportDirectory->NumberOfFunctions)
		return nullptr;

	// Get the location of the functions.
	const DWORD* pAddressOfFunctions = reinterpret_cast<DWORD*>(pModuleBase
		+ pImageExportDirectory->AddressOfFunctions);

	if (!pAddressOfFunctions)
		return nullptr;

	// Get the names of the functions.
	const DWORD* pAddressOfName = reinterpret_cast<DWORD*>(pModuleBase
		+ pImageExportDirectory->AddressOfNames);

	if (!pAddressOfName)
		return nullptr;

	// Get the ordinals of the functions.
	DWORD* pAddressOfOrdinals = reinterpret_cast<DWORD*>(pModuleBase
		+ pImageExportDirectory->AddressOfNameOrdinals);

	if (!pAddressOfOrdinals)
		return nullptr;

	for (DWORD i = 0; i < pImageExportDirectory->NumberOfFunctions; i++)
	{
		// Get virtual relative Address of the function name,
		// then add module base Address to get the actual location.
		const char* ExportFunctionName = 
			reinterpret_cast<char*>(reinterpret_cast<DWORD*>(
			pModuleBase + pAddressOfName[i]));

		if (strcmp(ExportFunctionName, szSymbolName) == NULL)
		{
			// Get the function ordinal, then grab the relative
			// virtual address of our wanted function. Then add
			// module base address so we get the actual location.
			return pModuleBase
			+ pAddressOfFunctions[reinterpret_cast<WORD*>(pAddressOfOrdinals)[i]];
		}
	}
	return nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: unlink module from peb
// Disclaimer: This does not bypass GetMappedFileName. That function calls
// NtQueryVirtualMemory which does a syscall to ntoskrnl for getting info
// on a section.
//-----------------------------------------------------------------------------
void CModule::UnlinkFromPEB() const
{
#define UNLINK_FROM_PEB(entry)    \
	(entry).Flink->Blink = (entry).Blink; \
	(entry).Blink->Flink = (entry).Flink;

	// https://en.wikipedia.org/wiki/Win32_Thread_Information_Block
	const PEB64* processEnvBlock = reinterpret_cast<PEB64*>(__readgsqword(0x60));
	const LIST_ENTRY* inLoadOrderList = &processEnvBlock->Ldr->InLoadOrderModuleList;

	for (LIST_ENTRY* entry = inLoadOrderList->Flink; entry != inLoadOrderList; entry = entry->Flink)
	{
		const PLDR_DATA_TABLE_ENTRY pldrEntry = reinterpret_cast<PLDR_DATA_TABLE_ENTRY>(entry->Flink);
		const QWORD baseAddr = reinterpret_cast<QWORD>(pldrEntry->DllBase);

		if (baseAddr != m_pModuleBase)
			continue;

		UNLINK_FROM_PEB(pldrEntry->InInitializationOrderLinks);
		UNLINK_FROM_PEB(pldrEntry->InMemoryOrderLinks);
		UNLINK_FROM_PEB(pldrEntry->InLoadOrderLinks);
		break;
	}
#undef UNLINK_FROM_PEB
}
