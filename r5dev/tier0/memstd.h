#ifndef MEMSTD_H
#define MEMSTD_H

class IMemAlloc
{
public:
	template<typename T>
	T* Alloc(size_t nSize)
	{
		const int index = 0;
		return CallVFunc<T*>(index, this, nSize);
	}
	template<typename T>
	T* Realloc(T* pMem, size_t nSize)
	{
		const int index = 2;
		return CallVFunc<T*>(index, this, nSize);
	}
	template<typename T>
	void Free(T* pMem)
	{
		const int index = 4;
		CallVFunc<void>(index, this, pMem);
	}
};

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
class CStdMemAlloc : public IMemAlloc{};

inline CMemory p_CreateGlobalMemAlloc;
inline auto v_CreateGlobalMemAlloc = p_CreateGlobalMemAlloc.RCast<CStdMemAlloc* (*)(void)>();

inline CStdMemAlloc** g_pMemAllocSingleton = nullptr;


inline CStdMemAlloc* MemAllocSingleton()
{
	if (!(*g_pMemAllocSingleton))
	{
		(*g_pMemAllocSingleton) = v_CreateGlobalMemAlloc();
	}
	return (*g_pMemAllocSingleton);
}

///////////////////////////////////////////////////////////////////////////////
class VMemStd : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: CreateGlobalMemAlloc                 : {:#18x} |\n", p_CreateGlobalMemAlloc.GetPtr());
		spdlog::debug("| VAR: g_pMemAllocSingleton                 : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_pMemAllocSingleton));
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
		p_CreateGlobalMemAlloc = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x20\xBB\x00\x00\x00\x00\x33\xC0"), "xxxxxxx????xx");
		v_CreateGlobalMemAlloc = p_CreateGlobalMemAlloc.RCast<CStdMemAlloc* (*)(void)>();    /*40 53 48 83 EC 20 BB ?? ?? ?? ?? 33 C0*/
	}
	virtual void GetVar(void) const
	{
		g_pMemAllocSingleton = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x57\x48\x81\xEC\x00\x00\x00\x00\x41\x8B\xD8"),
			"xxxx?xxxx????xxx").OffsetSelf(0x5A).FindPatternSelf("48 8B", CMemory::Direction::DOWN, 100).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CStdMemAlloc**>();
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VMemStd);

#endif // MEMSTD_H
