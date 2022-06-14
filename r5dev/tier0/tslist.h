#ifndef TSLIST_H
#define TSLIST_H

inline CMemory p_MemAlloc_Internal;
inline auto v_MemAlloc_Internal = p_MemAlloc_Internal.RCast<void* (*)(void* pPool, size_t nSize)>();

inline CMemory p_MemAlloc_Wrapper;
inline auto v_MemAlloc_Wrapper = p_MemAlloc_Wrapper.RCast<void* (*)(size_t nSize)>();

inline CMemory p_CTSListBase_Wrapper;
inline auto CTSListBase_Wrapper = p_CTSListBase_Wrapper.RCast<void* (*)(void)>();

inline CMemory g_pMallocPool;

void TSList_Attach();
void TSList_Detach();
///////////////////////////////////////////////////////////////////////////////
class VTSListBase : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: MemAlloc_Internal                    : {:#18x} |\n", p_MemAlloc_Internal.GetPtr());
		spdlog::debug("| FUN: MemAlloc_Wrapper                     : {:#18x} |\n", p_MemAlloc_Wrapper.GetPtr());
		spdlog::debug("| FUN: CTSListBase_Wrapper                  : {:#18x} |\n", p_CTSListBase_Wrapper.GetPtr());
		spdlog::debug("| VAR: g_pMallocPool                        : {:#18x} |\n", g_pMallocPool.GetPtr());
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
		p_MemAlloc_Internal = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\xE9\x00\x00\x00\x00\xCC\xCC\xCC\x40\x53\x48\x83\xEC\x20\x48\x8D\x05\x00\x00\x00\x00"), "x????xxxxxxxxxxxx????");
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_MemAlloc_Wrapper = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x20\x48\x8B\x05\x00\x00\x00\x00\x48\x8B\xD9\x48\x85\xC0\x75\x0C\xE8\x16"), "xxxxxxxxx????xxxxxxxxxx");
#elif defined (GAMEDLL_S2)
		p_MemAlloc_Wrapper = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x20\x48\x8B\x05\x00\x00\x00\x00\x48\x8B\xD9\x48\x85\xC0\x75\x0C\xE8\x00\x00\x00\x00\x48\x89\x05\x00\x00\x00\x00\x4C\x8B\x00\x48\x8B\xD3\x48\x8B\xC8\x48\x83\xC4\x20\x5B\x49\xFF\x60\x08"), "xxxxxxxxx????xxxxxxxxx????xxx????xxxxxxxxxxxxxxxxxx");
#elif defined (GAMEDLL_S3)
		p_MemAlloc_Wrapper = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x20\x48\x8B\x05\x6B\x83\x25\x0D\x48\x8B\xD9"), "xxxxxxxxxxxxxxxx");
#endif
		p_CTSListBase_Wrapper = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x20\xBB\x00\x00\x00\x00\x33\xC0"), "xxxxxxx????xx");

		CTSListBase_Wrapper = p_CTSListBase_Wrapper.RCast<void* (*)(void)>();        /*40 53 48 83 EC 20 BB ?? ?? ?? ?? 33 C0*/
		v_MemAlloc_Wrapper = p_MemAlloc_Wrapper.RCast<void* (*)(size_t)>();          /*40 53 48 83 EC 20 48 8B 05 6B 83 25 0D 48 8B D9*/
		v_MemAlloc_Internal = p_MemAlloc_Internal.RCast<void* (*)(void*, size_t)>(); /*E9 ?? ?? ?? ?? CC CC CC 40 53 48 83 EC 20 48 8D 05 ?? ?? ?? ??*/
	}
	virtual void GetVar(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		g_pMallocPool = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>(
			"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\xD9\xFF\x15\x00\x00\x00\x00"),
			"xxxx?xxxx?xxxx?xxxxxxxxxxxx????xxxxx????").Offset(0x600).FindPatternSelf("48 8D 15 ?? ?? ?? 01", CMemory::Direction::DOWN, 100).ResolveRelativeAddressSelf(0x3, 0x7);
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		g_pMallocPool = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>(
			"\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\xAC\x24\x00\x00\x00\x00\xB8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x2B\xE0\x48\x8B\xD9"),
			"xxxx?xxxx?xxxx?xxxxxxxxxxxxx????x????x????xxxxxx").Offset(0x130).FindPatternSelf("48 8D 15 ?? ?? ?? 01", CMemory::Direction::DOWN, 100).ResolveRelativeAddressSelf(0x3, 0x7);
#endif
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VTSListBase);

#endif // TSLIST_H
