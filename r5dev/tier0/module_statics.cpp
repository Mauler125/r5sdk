//===========================================================================//
//
// Purpose: Implementation of static methods in the CModule class.
//
// --------------------------------------------------------------------------
// Static methods are implemented here to avoid linker errors, as the CModule
// class relies on a protobuf based caching implementation. This allows us to
// not link unnecessary libraries to modules using these methods.
//===========================================================================//
#include "tier0/module.h"

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

	for (DWORD i = 0; i < pImageExportDirectory->NumberOfNames; i++)
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
