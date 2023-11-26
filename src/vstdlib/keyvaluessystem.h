#pragma once

#include "public/ikeyvaluessystem.h"
#include "tier1/memstack.h"
#include "tier1/mempool.h"
#include "tier1/utlvector.h"
#include "tier1/utlmap.h"

class CKeyValuesSystem : public IKeyValuesSystem// VTABLE @ 0x1413AA1E8 in R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM
{
public:
	void RegisterSizeofKeyValues(int64_t nSize);
	void* AllocKeyValuesMemory(int64_t nSize);
	void FreeKeyValuesMemory(void* pMem);
	HKeySymbol GetSymbolForString(const char* szName, bool bCreate = true);
	const char* GetStringForSymbol(HKeySymbol symbol);

	void* GetMemPool(void); // GetMemPool returns a global variable called m_pMemPool, it gets modified by AllocKeyValuesMemory and with FreeKeyValuesMemory you can see where to find it in FreeKeyValuesMemory.
	void SetKeyValuesExpressionSymbol(const char* szName, bool bValue);
	bool GetKeyValuesExpressionSymbol(const char* szName);
	HKeySymbol GetSymbolForStringCaseSensitive(HKeySymbol& hCaseInsensitiveSymbol, const char* szName, bool bCreate = true);

private:
	int64 m_iMaxKeyValuesSize;
	CMemoryStack m_Strings;

	struct hash_item_t
	{
		int stringIndex;
		hash_item_t* next;
	};

	CUtlMemoryPool m_HashItemMemPool;
	CUtlVector<hash_item_t> m_HashTable;

	struct MemoryLeakTracker_t
	{
		int nameIndex;
		void* pMem;
	};

	// Unknown less func.
	void* m_pCompareFunc;

	CUtlRBTree<MemoryLeakTracker_t, int> m_KeyValuesTrackingList;
	CUtlMap<HKeySymbol, bool> m_KvConditionalSymbolTable;

	CThreadFastMutex m_Mutex;
};

/* ==== KEYVALUESSYSTEM ================================================================================================================================================= */
inline void* g_pKeyValuesMemPool = nullptr;
inline CKeyValuesSystem* g_pKeyValuesSystem = nullptr;

//-----------------------------------------------------------------------------
// Instance singleton and expose interface to rest of code
//-----------------------------------------------------------------------------
FORCEINLINE CKeyValuesSystem* KeyValuesSystem()
{
	return g_pKeyValuesSystem;
}

///////////////////////////////////////////////////////////////////////////////
class HKeyValuesSystem : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogVarAdr("g_pKeyValuesMemPool", reinterpret_cast<uintptr_t>(g_pKeyValuesMemPool));
		LogVarAdr("g_pKeyValuesSystem", reinterpret_cast<uintptr_t>(g_pKeyValuesSystem));
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
