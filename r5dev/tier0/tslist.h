#ifndef TSLIST_H
#define TSLIST_H

inline CMemory p_MemAlloc_Internal;
inline auto malloc_internal = p_MemAlloc_Internal.RCast<void* (*)(void* pBool, int64_t nSize)>();

inline CMemory p_MemAlloc_Wrapper;
inline auto MemAlloc_Wrapper = p_MemAlloc_Wrapper.RCast<void* (*)(size_t)>();

inline CMemory p_CTSListBase_Wrapper;
inline auto CTSListBase_Wrapper = p_CTSListBase_Wrapper.RCast<void* (*)(void)>();

///////////////////////////////////////////////////////////////////////////////
class HTSListBase : public IDetour
{
	virtual void GetAdr(void) const
	{
		std::cout << "| FUN: MemAlloc_Internal                    : 0x" << std::hex << std::uppercase << p_MemAlloc_Internal.GetPtr()   << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: MemAlloc_Wrapper                     : 0x" << std::hex << std::uppercase << p_MemAlloc_Wrapper.GetPtr()    << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: CTSListBase_Wrapper                  : 0x" << std::hex << std::uppercase << p_CTSListBase_Wrapper.GetPtr() << std::setw(nPad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
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

		CTSListBase_Wrapper = p_CTSListBase_Wrapper.RCast<void* (*)(void)>();     /*40 53 48 83 EC 20 BB ? ? ? ? 33 C0*/
		MemAlloc_Wrapper = p_MemAlloc_Wrapper.RCast<void* (*)(size_t)>();         /*40 53 48 83 EC 20 48 8B 05 6B 83 25 0D 48 8B D9*/
		malloc_internal = p_MemAlloc_Internal.RCast<void* (*)(void*, int64_t)>(); /*E9 ? ? ? ? CC CC CC 40 53 48 83 EC 20 48 8D 05 ? ? ? ?*/
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HTSListBase);

#endif // TSLIST_H
