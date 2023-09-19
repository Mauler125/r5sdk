//=============================================================================//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "core/stdafx.h"
#include "vstdlib/keyvaluessystem.h"

//-----------------------------------------------------------------------------
// Purpose: registers the size of the KeyValues in the specified instance
//			so it can build a properly sized memory pool for the KeyValues objects
//			the sizes will usually never differ but this is for versioning safety
// Input  : nSize - 
//-----------------------------------------------------------------------------
void CKeyValuesSystem::RegisterSizeofKeyValues(int64_t nSize)
{
	const static int index = 0;
	CallVFunc<void>(index, this, nSize);
}

//-----------------------------------------------------------------------------
// Purpose: allocates a KeyValues object from the shared mempool
// Input  : nSize - 
// Output : pointer to allocated pool
//-----------------------------------------------------------------------------
void* CKeyValuesSystem::AllocKeyValuesMemory(int64_t nSize)
{
	const static int index = 1;
	return CallVFunc<void*>(index, this, nSize);
}

//-----------------------------------------------------------------------------
// Purpose: frees a KeyValues object from the shared mempool
// Input  : *pMem - 
//-----------------------------------------------------------------------------
void CKeyValuesSystem::FreeKeyValuesMemory(void* pMem)
{
	const static int index = 2;
	CallVFunc<void>(index, this, pMem);
}

//-----------------------------------------------------------------------------
// Purpose: symbol table access (used for key names)
// Input  : *szName - 
//			bCreate - 
// Output : handle to KeyValue symbol on success, -1 on failure
//-----------------------------------------------------------------------------
HKeySymbol CKeyValuesSystem::GetSymbolForString(const char* szName, bool bCreate)
{
	const static int index = 3;
	return CallVFunc<HKeySymbol>(index, this, szName, bCreate);
}

//-----------------------------------------------------------------------------
// Purpose: symbol table access
// Input  : symbol - 
// Output : symbol string if found, empty if not found
//-----------------------------------------------------------------------------
const char* CKeyValuesSystem::GetStringForSymbol(HKeySymbol symbol)
{
	const static int index = 4;
	return CallVFunc<const char*>(index, this, symbol);
}

//-----------------------------------------------------------------------------
// Purpose: gets the global KeyValues memory pool
// Output : *g_pKeyValuesMemPool - 
//-----------------------------------------------------------------------------
void* CKeyValuesSystem::GetMemPool(void)
{
	const static int index = 7;
	return CallVFunc<void*>(index, this);
}

//-----------------------------------------------------------------------------
// Purpose: set a value for keyvalues resolution symbol
// e.g.: SetKeyValuesExpressionSymbol( "LOWVIOLENCE", true ) - enables [$LOWVIOLENCE]
// Input  : *szName - 
//			bValue - 
//-----------------------------------------------------------------------------
void CKeyValuesSystem::SetKeyValuesExpressionSymbol(const char* szName, bool bValue)
{
	const static int index = 8;
	CallVFunc<void>(index, this, szName, bValue);
}

//-----------------------------------------------------------------------------
// Purpose: get a value for keyvalues resolution symbol
// Input  : *szName - 
//-----------------------------------------------------------------------------
bool CKeyValuesSystem::GetKeyValuesExpressionSymbol(const char* szName)
{
	const static int index = 9;
	return CallVFunc<bool>(index, this, szName);
}

//-----------------------------------------------------------------------------
// Purpose: symbol table access (used for key names)
// Input  : *hCaseInsensitiveSymbol - 
//			*szName - 
//			bCreate - 
// Output : handle to KeyValue symbol on success, INVALID_KEY_SYMBOL on failure
//-----------------------------------------------------------------------------
HKeySymbol CKeyValuesSystem::GetSymbolForStringCaseSensitive(HKeySymbol& hCaseInsensitiveSymbol, const char* szName, bool bCreate)
{
	const static int index = 10;
	return CallVFunc<HKeySymbol>(index, this, hCaseInsensitiveSymbol, szName, bCreate);
}
