#pragma once

typedef int HKeySymbol;
#define INVALID_KEY_SYMBOL (-1)

class CKeyValuesSystem // VTABLE @ 0x1413AA1E8 in R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM
{
public:
	void RegisterSizeofKeyValues(int64_t size);
	void* AllocKeyValuesMemory(int64_t size);
	void FreeKeyValuesMemory(void* pMem);
	HKeySymbol GetSymbolForString(const char* name, bool bCreate = false);
	const char* GetStringForSymbol(HKeySymbol symbol);

	void* GetMemPool(void); // GetMemPool returns a global variable called m_pMemPool, it gets modified by AllocKeyValuesMemory and with FreeKeyValuesMemory you can see where to find it in FreeKeyValuesMemory.
	void SetKeyValuesExpressionSymbol(const char* name, bool bValue);
	bool GetKeyValuesExpressionSymbol(const char* name);
	HKeySymbol GetSymbolForStringCaseSensitive(HKeySymbol& hCaseInsensitiveSymbol, const char* name, bool bCreate = false);

	// Datatypes aren't accurate. But full fill the actual byte distance.
public:
	void* m_pVTable;                         // 0x0000
	int64_t m_iMaxKeyValuesSize;             // 0x0008
private:
	char         gap10[240];                 // 0x0010
public:
	int          m_KvConditionalSymbolTable; // 0x0100
private:
	char         gap104[4];                  // 0x0104
public:
	int64_t field_108;                       // 0x0108
private:
	char         gap110[32];                 // 0x0110
public:
	int          m_mutex;                    // 0x0130
};

CKeyValuesSystem* KeyValuesSystem();
/* ==== KEYVALUESSYSTEM ================================================================================================================================================= */
inline void* g_pKeyValuesMemPool = nullptr;
inline CKeyValuesSystem* g_pKeyValuesSystem = nullptr;

///////////////////////////////////////////////////////////////////////////////
class HKeyValuesSystem : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| VAR: g_pKeyValuesMemPool                  : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_pKeyValuesMemPool));
		spdlog::debug("| VAR: g_pKeyValuesSystem                   : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_pKeyValuesSystem));
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const
	{
		g_pKeyValuesSystem = g_GameDll.FindPatternSIMD(
			reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x56\x57\x41\x56\x48\x83\xEC\x40\x48\x8B\xF1"), "xxxx?xxxx?xxxxxxxxxxx")
			.FindPatternSelf("48 8D 0D ?? ?? ?? 01", CMemory::Direction::DOWN).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CKeyValuesSystem*>();

		g_pKeyValuesMemPool = g_GameDll.FindPatternSIMD(
			reinterpret_cast<rsig_t>("\x48\x8B\x05\x00\x00\x00\x00\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x48\x85\xD2"), "xxx????xxxxxxxxxxxx").
			ResolveRelativeAddressSelf(0x3, 0x7).RCast<void*>();
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HKeyValuesSystem);
