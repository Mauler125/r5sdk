//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include <ikeyvaluessystem.h>
#include "keyvaluessystem.h"
#include "tier0/threadtools.h"
#include "tier1/keyvalues.h"
#include "tier1/mempool.h"
#include "tier1/utlsymbol.h"
#include "tier1/utlmap.h"
#include "tier1/memstack.h"
#include "tier1/convar.h"
#include "tier1/strtools.h"

#ifdef _PS3
#include "ps3/ps3_core.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

#ifdef NO_SBH // no need to pool if using tier0 small block heap
#define KEYVALUES_USE_POOL 1
#endif

//
// Defines platform-endian-specific macros:
// MEM_4BYTES_AS_0_AND_3BYTES :  present a 4 byte uint32 as a memory
//                               layout where first memory byte is zero
//                               and the other 3 bytes represent value
// MEM_4BYTES_FROM_0_AND_3BYTES: unpack from memory with first zero byte
//                               and 3 value bytes the original uint32 value
//
// used for efficiently reading/writing storing 3 byte values into memory
// region immediately following a null-byte-terminated string, essentially
// sharing the null-byte-terminator with the first memory byte
//
#if defined( PLAT_LITTLE_ENDIAN )
// Number in memory has lowest-byte in front, use shifts to make it zero
#define MEM_4BYTES_AS_0_AND_3BYTES( x4bytes ) ( ( (uint32) (x4bytes) ) << 8 )
#define MEM_4BYTES_FROM_0_AND_3BYTES( x03bytes ) ( ( (uint32) (x03bytes) ) >> 8 )
#endif
#if defined( PLAT_BIG_ENDIAN )
// Number in memory has highest-byte in front, use masking to make it zero
#define MEM_4BYTES_AS_0_AND_3BYTES( x4bytes ) ( ( (uint32) (x4bytes) ) & 0x00FFFFFF )
#define MEM_4BYTES_FROM_0_AND_3BYTES( x03bytes ) ( ( (uint32) (x03bytes) ) & 0x00FFFFFF )
#endif

CKeyValuesSystem* g_pKeyValuesSystem = nullptr;
void* g_pKeyValuesMemPool = nullptr;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CKeyValuesSystem::CKeyValuesSystem() :
	m_HashItemMemPool(sizeof(hash_item_t), 64, CUtlMemoryPool::GROW_FAST, "CKeyValuesSystem::m_HashItemMemPool"),
	m_KeyValuesTrackingList(0, 0, MemoryLeakTrackerLessFunc),
	m_KvConditionalSymbolTable(DefLessFunc(HKeySymbol))
{
	MEM_ALLOC_CREDIT();
	// initialize hash table
	m_HashTable.AddMultipleToTail(2047);
	for (int i = 0; i < m_HashTable.Count(); i++)
	{
		m_HashTable[i].stringIndex = 0;
		m_HashTable[i].next = NULL;
	}

	m_Strings.Init("CKeyValuesSystem::m_Strings", 4 * 1024 * 1024, 64 * 1024, 0, 4);
	// Make 0 stringIndex to never be returned, by allocating
	// and wasting minimal number of alignment bytes now:
	char* pszEmpty = ((char*)m_Strings.Alloc(1));
	*pszEmpty = 0;

#ifdef KEYVALUES_USE_POOL
	m_pMemPool = NULL;
#endif
	m_iMaxKeyValuesSize = sizeof(KeyValues);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CKeyValuesSystem::~CKeyValuesSystem()
{
#ifdef KEYVALUES_USE_POOL
#ifdef _DEBUG
	// display any memory leaks
	if (m_pMemPool && m_pMemPool->Count() > 0)
	{
		DevMsg("Leaked KeyValues blocks: %d\n", m_pMemPool->Count());
	}

	// iterate all the existing keyvalues displaying their names
	for (int i = 0; i < m_KeyValuesTrackingList.MaxElement(); i++)
	{
		if (m_KeyValuesTrackingList.IsValidIndex(i))
		{
			DevMsg("\tleaked KeyValues(%s)\n", &m_Strings[m_KeyValuesTrackingList[i].nameIndex]);
		}
	}
#endif

	delete m_pMemPool;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: registers the size of the KeyValues in the specified instance
//			so it can build a properly sized memory pool for the KeyValues objects
//			the sizes will usually never differ but this is for versioning safety
//-----------------------------------------------------------------------------
void CKeyValuesSystem::RegisterSizeofKeyValues(const ssize_t size)
{
	if (size > m_iMaxKeyValuesSize)
	{
		m_iMaxKeyValuesSize = size;
	}
}

#ifdef KEYVALUES_USE_POOL
static void KVLeak(char const* fmt, ...)
{
	va_list argptr;
	char data[1024];

	va_start(argptr, fmt);
	Q_vsnprintf(data, sizeof(data), fmt, argptr);
	va_end(argptr);

	Msg(eDLL_T::COMMON, "%s", data);
}
#endif

//-----------------------------------------------------------------------------
// Purpose: allocates a KeyValues object from the shared mempool
//-----------------------------------------------------------------------------
void* CKeyValuesSystem::AllocKeyValuesMemory(const ssize_t size)
{
#ifdef KEYVALUES_USE_POOL
	// allocate, if we don't have one yet
	if (!m_pMemPool)
	{
		m_pMemPool = new CUtlMemoryPool(m_iMaxKeyValuesSize, 1024, CUtlMemoryPool::GROW_FAST, "CKeyValuesSystem::m_pMemPool");
		m_pMemPool->SetErrorReportFunc(KVLeak);
	}

	return m_pMemPool->Alloc(size);
#else
	return malloc(size);
#endif
}

//-----------------------------------------------------------------------------
// Purpose: frees a KeyValues object from the shared mempool
//-----------------------------------------------------------------------------
void CKeyValuesSystem::FreeKeyValuesMemory(void* const pMem)
{
#ifdef KEYVALUES_USE_POOL
	m_pMemPool->Free(pMem);
#else
	free(pMem);
#endif
}

//-----------------------------------------------------------------------------
// Purpose: symbol table access (used for key names)
//-----------------------------------------------------------------------------
HKeySymbol CKeyValuesSystem::GetSymbolForString(const char* const name, const bool bCreate)
{
	if (!name)
	{
		return (-1);
	}

	AUTO_LOCK(m_Mutex);
	MEM_ALLOC_CREDIT();

	int hash = CaseInsensitiveHash(name, m_HashTable.Count());
	int i = 0;
	hash_item_t* item = &m_HashTable[hash];
	while (1)
	{
		if (!stricmp(name, (char*)m_Strings.GetBase() + item->stringIndex))
		{
			return (HKeySymbol)item->stringIndex;
		}

		i++;

		if (item->next == NULL)
		{
			if (!bCreate)
			{
				// not found
				return -1;
			}

			// we're not in the table
			if (item->stringIndex != 0)
			{
				// first item is used, an new item
				item->next = (hash_item_t*)m_HashItemMemPool.Alloc(sizeof(hash_item_t));
				item = item->next;
			}

			// build up the new item
			item->next = NULL;
			const size_t numStringBytes = strlen(name);
			char* pString = (char*)m_Strings.Alloc(numStringBytes + 1 + 3);
			if (!pString)
			{
				Error(eDLL_T::COMMON, EXIT_FAILURE, "Out of keyvalue string space");
				return -1;
			}
			item->stringIndex = pString - (char*)m_Strings.GetBase();
			memcpy(pString, name, numStringBytes);
			*reinterpret_cast<uint32*>(pString + numStringBytes) = 0;	// string null-terminator + 3 alternative spelling bytes
			return (HKeySymbol)item->stringIndex;
		}

		item = item->next;
	}

	// shouldn't be able to get here
	Assert(0);
	return (-1);
}

//-----------------------------------------------------------------------------
// Purpose: symbol table access (used for key names)
//-----------------------------------------------------------------------------
HKeySymbol CKeyValuesSystem::GetSymbolForStringCaseSensitive(HKeySymbol& hCaseInsensitiveSymbol, const char* const name, const bool bCreate)
{
	if (!name)
	{
		return (-1);
	}

	AUTO_LOCK(m_Mutex);
	MEM_ALLOC_CREDIT();

	const int hash = CaseInsensitiveHash(name, m_HashTable.Count());

	ssize_t numNameStringBytes = -1;
	ssize_t i = 0;
	hash_item_t* item = &m_HashTable[hash];

	while (1)
	{
		char* pCompareString = (char*)m_Strings.GetBase() + item->stringIndex;
		const int iResultNegative = _V_stricmp_NegativeForUnequal(name, pCompareString);
		if (iResultNegative == 0)
		{
			// strings are exactly equal matching every letter's case
			hCaseInsensitiveSymbol = (HKeySymbol)item->stringIndex;
			return (HKeySymbol)item->stringIndex;
		}
		else if (iResultNegative > 0)
		{
			// strings are equal in a case-insensitive compare, but have different case for some letters
			// Need to walk the case-resolving chain
			numNameStringBytes = Q_strlen(pCompareString);
			uint32* pnCaseResolveIndex = reinterpret_cast<uint32*>(pCompareString + numNameStringBytes);
			hCaseInsensitiveSymbol = (HKeySymbol)item->stringIndex;
			while (int nAlternativeStringIndex = MEM_4BYTES_FROM_0_AND_3BYTES(*pnCaseResolveIndex))
			{
				pCompareString = (char*)m_Strings.GetBase() + nAlternativeStringIndex;
				const int iResult = strcmp(name, pCompareString);
				if (!iResult)
				{
					// found an exact match
					return (HKeySymbol)nAlternativeStringIndex;
				}
				// Keep traversing alternative case-resolving chain
				pnCaseResolveIndex = reinterpret_cast<uint32*>(pCompareString + numNameStringBytes);
			}
			// Reached the end of alternative case-resolving chain, pnCaseResolveIndex is pointing at 0 bytes
			// indicating no further alternative stringIndex
			if (!bCreate)
			{
				// If we aren't interested in creating the actual string index,
				// then return symbol with default capitalization
				// NOTE: this is not correct value, but it cannot be used to create a new value anyway,
				// only for locating a pre-existing value and lookups are case-insensitive
				return (HKeySymbol)item->stringIndex;
			}
			else
			{
				char* pString = (char*)m_Strings.Alloc(numNameStringBytes + 1 + 3);
				if (!pString)
				{
					Error(eDLL_T::COMMON, EXIT_FAILURE, "Out of keyvalue string space");
					return -1;
				}
				int64_t nNewAlternativeStringIndex = pString - (char*)m_Strings.GetBase();
				memcpy(pString, name, numNameStringBytes);
				*reinterpret_cast<uint32*>(pString + numNameStringBytes) = 0;	// string null-terminator + 3 alternative spelling bytes
				*pnCaseResolveIndex = MEM_4BYTES_AS_0_AND_3BYTES(nNewAlternativeStringIndex);	// link previous spelling entry to the new entry
				return (HKeySymbol)nNewAlternativeStringIndex;
			}
		}

		i++;

		if (item->next == NULL)
		{
			if (!bCreate)
			{
				// not found
				return -1;
			}

			// we're not in the table
			if (item->stringIndex != 0)
			{
				// first item is used, an new item
				item->next = (hash_item_t*)m_HashItemMemPool.Alloc(sizeof(hash_item_t));
				item = item->next;
			}

			// build up the new item
			item->next = NULL;
			size_t numStringBytes = strlen(name);
			char* pString = (char*)m_Strings.Alloc(numStringBytes + 1 + 3);
			if (!pString)
			{
				Error(eDLL_T::COMMON, EXIT_FAILURE, "Out of keyvalue string space");
				return -1;
			}
			item->stringIndex = pString - (char*)m_Strings.GetBase();
			memcpy(pString, name, numStringBytes);
			*reinterpret_cast<uint32*>(pString + numStringBytes) = 0;	// string null-terminator + 3 alternative spelling bytes
			hCaseInsensitiveSymbol = (HKeySymbol)item->stringIndex;
			return (HKeySymbol)item->stringIndex;
		}

		item = item->next;
	}

	// shouldn't be able to get here
	Assert(0);
	return (-1);
}

//-----------------------------------------------------------------------------
// Purpose: symbol table access
//-----------------------------------------------------------------------------
const char* CKeyValuesSystem::GetStringForSymbol(const HKeySymbol symbol)
{
	if (symbol == -1)
	{
		return "";
	}
	return ((char*)m_Strings.GetBase() + (size_t)symbol);
}

//-----------------------------------------------------------------------------
// Purpose: adds KeyValues record into global list so we can track memory leaks
//-----------------------------------------------------------------------------
void CKeyValuesSystem::AddKeyValuesToMemoryLeakList(const void* const pMem, const HKeySymbol name)
{
#ifdef _DEBUG
	// only track the memory leaks in debug builds
	MemoryLeakTracker_t item = { name, pMem };
	m_KeyValuesTrackingList.Insert(item);
#endif
}

//-----------------------------------------------------------------------------
// Purpose: used to track memory leaks
//-----------------------------------------------------------------------------
void CKeyValuesSystem::RemoveKeyValuesFromMemoryLeakList(const void* const pMem)
{
#ifdef _DEBUG
	// only track the memory leaks in debug builds
	MemoryLeakTracker_t item = { 0, pMem };
	int index = m_KeyValuesTrackingList.Find(item);
	m_KeyValuesTrackingList.RemoveAt(index);
#endif
}

//-----------------------------------------------------------------------------
// Purpose: generates a simple hash value for a string
//-----------------------------------------------------------------------------
int CKeyValuesSystem::CaseInsensitiveHash(const char* const string, const int iBounds)
{
	unsigned int hash = 0;
	const char* iter = string;

	for (; *iter != 0; iter++)
	{
		if (*iter >= 'A' && *iter <= 'Z')
		{
			hash = (hash << 1) + (*iter - 'A' + 'a');
		}
		else
		{
			hash = (hash << 1) + *iter;
		}
	}

	return hash % iBounds;
}

//-----------------------------------------------------------------------------
// Purpose: set/get a value for keyvalues resolution symbol
// e.g.: SetKeyValuesExpressionSymbol( "LOWVIOLENCE", true ) - enables [$LOWVIOLENCE]
//-----------------------------------------------------------------------------
void CKeyValuesSystem::SetKeyValuesExpressionSymbol(const char* name, const bool bValue)
{
	if (!name)
		return;

	const char* pName = name;

	if (pName[0] == '$')
		++pName;

	HKeySymbol hSym = GetSymbolForString(pName, true);	// find or create symbol
	{
		AUTO_LOCK(m_Mutex);
		m_KvConditionalSymbolTable.InsertOrReplace(hSym, bValue);
	}
}

bool CKeyValuesSystem::GetKeyValuesExpressionSymbol(const char* const name)
{
	if (!name)
		return false;

	const char* pName = name;

	if (pName[0] == '$')
		++pName;

	const HKeySymbol hSym = GetSymbolForString(pName, false);	// find or create symbol
	if (hSym != -1)
	{
		AUTO_LOCK(m_Mutex);
		CUtlMap< HKeySymbol, bool >::IndexType_t idx = m_KvConditionalSymbolTable.Find(hSym);
		if (idx != m_KvConditionalSymbolTable.InvalidIndex())
		{
			// Found the symbol value in conditional symbol table
			return m_KvConditionalSymbolTable.Element(idx);
		}
	}

	//
	// Fallback conditionals
	//

//	if (!V_stricmp(pName, "GAMECONSOLESPLITSCREEN"))
//	{
//#if defined( _GAMECONSOLE )
//		return (XBX_GetNumGameUsers() > 1);
//#else
//		return false;
//#endif
//	}
//
//	if (!V_stricmp(pName, "GAMECONSOLEGUEST"))
//	{
//#if defined( _GAMECONSOLE )
//		return (XBX_GetPrimaryUserIsGuest() != 0);
//#else
//		return false;
//#endif
//	}
//
//	if (!V_stricmp(pName, "ENGLISH") ||
//		!V_stricmp(pName, "JAPANESE") ||
//		!V_stricmp(pName, "GERMAN") ||
//		!V_stricmp(pName, "FRENCH") ||
//		!V_stricmp(pName, "SPANISH") ||
//		!V_stricmp(pName, "ITALIAN") ||
//		!V_stricmp(pName, "KOREAN") ||
//		!V_stricmp(pName, "TCHINESE") ||
//		!V_stricmp(pName, "PORTUGUESE") ||
//		!V_stricmp(pName, "SCHINESE") ||
//		!V_stricmp(pName, "POLISH") ||
//		!V_stricmp(pName, "RUSSIAN") ||
//		!V_stricmp(pName, "TURKISH"))
//	{
//		// the language symbols are true if we are in that language
//		// english is assumed when no language is present
//		const char* pLanguageString;
//#ifdef _GAMECONSOLE
//		pLanguageString = XBX_GetLanguageString();
//#else
//		static ConVarRef cl_language("cl_language");
//		pLanguageString = cl_language.GetString();
//#endif
//		if (!pLanguageString || !pLanguageString[0])
//		{
//			pLanguageString = "english";
//		}
//		if (!V_stricmp(pName, pLanguageString))
//		{
//			return true;
//		}
//		else
//		{
//			return false;
//		}
//	}
//
//	// very expensive, back door for DLC updates
//	if (!V_strnicmp(pName, "CVAR_", 5))
//	{
//		ConVarRef cvRef(name + 5);
//		if (cvRef.IsValid())
//			return cvRef.GetBool();
//	}

	// purposely warn on these to prevent syntax errors
	// need to get these fixed asap, otherwise unintended false behavior
	Warning(eDLL_T::COMMON, "KV Conditional: Unknown symbol %s\n", pName);
	return false;
}
