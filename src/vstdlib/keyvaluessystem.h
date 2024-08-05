#pragma once

#include "public/ikeyvaluessystem.h"
#include "tier1/memstack.h"
#include "tier1/mempool.h"
#include "tier1/utlvector.h"
#include "tier1/utlmap.h"

class CKeyValuesSystem;

/* ==== KEYVALUESSYSTEM ================================================================================================================================================= */
extern CKeyValuesSystem* g_pKeyValuesSystem;
extern void* g_pKeyValuesMemPool;

class CKeyValuesSystem : public IKeyValuesSystem// VTABLE @ 0x1413AA1E8 in R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM
{
public:
	CKeyValuesSystem();
	~CKeyValuesSystem();

	// registers the size of the KeyValues in the specified instance
	// so it can build a properly sized memory pool for the KeyValues objects
	// the sizes will usually never differ but this is for versioning safety
	virtual void RegisterSizeofKeyValues(const ssize_t nSize);

	// allocates/frees a KeyValues object from the shared mempool
	virtual void* AllocKeyValuesMemory(const ssize_t nSize);
	virtual void FreeKeyValuesMemory(void*const pMem);

	// symbol table access (used for key names)
	virtual HKeySymbol GetSymbolForString(const char*const szName, const bool bCreate = true);
	virtual const char* GetStringForSymbol(const HKeySymbol symbol);

	// for debugging, adds KeyValues record into global list so we can track memory leaks
	virtual void AddKeyValuesToMemoryLeakList(const void*const pMem, const HKeySymbol name);
	virtual void RemoveKeyValuesFromMemoryLeakList(const void*const pMem);

	// GetMemPool returns a global variable called m_pMemPool, it gets modified by AllocKeyValuesMemory and with FreeKeyValuesMemory you can see where to find it in FreeKeyValuesMemory.
	virtual void* GetKeyValuesMemory(void) { return g_pKeyValuesMemPool; }

	// set/get a value for keyvalues resolution symbol
	// e.g.: SetKeyValuesExpressionSymbol( "LOWVIOLENCE", true ) - enables [$LOWVIOLENCE]
	virtual void SetKeyValuesExpressionSymbol(const char*const szName, const bool bValue);
	virtual bool GetKeyValuesExpressionSymbol(const char*const szName);

	// symbol table access from code with case-preserving requirements (used for key names)
	virtual HKeySymbol GetSymbolForStringCaseSensitive(HKeySymbol& hCaseInsensitiveSymbol, const char*const szName, const bool bCreate = true);

private:
	ssize_t m_iMaxKeyValuesSize;

	// string hash table
	/*
	Here's the way key values system data structures are laid out:
	hash table with 2047 hash buckets:
	[0] { hash_item_t }
	[1]
	[2]
	...
	each hash_item_t's stringIndex is an offset in m_Strings memory
	at that offset we store the actual null-terminated string followed
	by another 3 bytes for an alternative capitalization.
	These 3 trailing bytes are set to 0 if no alternative capitalization
	variants are present in the dictionary.
	These trailing 3 bytes are interpreted as stringIndex into m_Strings
	memory for the next	alternative capitalization

	Getting a string value by HKeySymbol : constant time access at the
	string memory represented by stringIndex

	Getting a symbol for a string value:
	1)	compute the hash
	2)	start walking the hash-bucket using special version of stricmp
		until a case insensitive match is found
	3a) for case-insensitive lookup return the found stringIndex
	3b) for case-sensitive lookup keep walking the list of alternative
		capitalizations using strcmp until exact case match is found
	*/
	CMemoryStack m_Strings;

	struct hash_item_t
	{
		int64_t stringIndex;
		hash_item_t* next;
	};

	CUtlMemoryPool m_HashItemMemPool;
	CUtlVector<hash_item_t> m_HashTable;
	int CaseInsensitiveHash(const char *const string, const int iBounds);

	struct MemoryLeakTracker_t
	{
		int64_t nameIndex;
		const void* pMem;
	};
	static bool MemoryLeakTrackerLessFunc(const MemoryLeakTracker_t& lhs, const MemoryLeakTracker_t& rhs)
	{
		return lhs.pMem < rhs.pMem;
	}

	// Unknown less func.
	void* m_pCompareFunc;

	CUtlRBTree<MemoryLeakTracker_t, int> m_KeyValuesTrackingList;
	CUtlMap<HKeySymbol, bool> m_KvConditionalSymbolTable;

	CThreadMutex m_Mutex;
};

///////////////////////////////////////////////////////////////////////////////
class HKeyValuesSystem : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogVarAdr("g_pKeyValuesMemPool", g_pKeyValuesMemPool);
		LogVarAdr("g_pKeyValuesSystem", g_pKeyValuesSystem);
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const
	{
		g_pKeyValuesSystem = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 56 57 41 56 48 83 EC 40 48 8B F1")
			.FindPatternSelf("48 8D 0D ?? ?? ?? 01", CMemory::Direction::DOWN).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CKeyValuesSystem*>();

		g_pKeyValuesMemPool = g_GameDll.FindPatternSIMD("48 8B 05 ?? ?? ?? ?? C3 CC CC CC CC CC CC CC CC 48 85 D2").ResolveRelativeAddressSelf(0x3, 0x7).RCast<void*>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const { }
};
///////////////////////////////////////////////////////////////////////////////
