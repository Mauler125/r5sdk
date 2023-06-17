#ifndef MEMSTD_H
#define MEMSTD_H

extern "C" void* R_malloc(size_t nSize);
extern "C" void R_free(void* pBlock);
extern "C" void* R_realloc(void* pBlock, size_t nSize);
extern "C" char* R_strdup(const char* pString);
extern "C" void* R_calloc(size_t nCount, size_t nSize);

class IMemAlloc
{
public:
	template<typename T>
	inline T* Alloc(size_t nSize)
	{
		const static int index = 1;
		return CallVFunc<T*>(index, this, nSize);
	}
	template<typename T>
	inline T* Realloc(T* pMem, size_t nSize)
	{
		const static int index = 3;
		return CallVFunc<T*>(index, this, pMem, nSize);
	}
	template<typename T>
	inline void FreeDbg(T* pMem, const char* pFileName, int nLine)
	{
		const static int index = 4; // Same as free, but takes debug parameters.
		CallVFunc<void>(index, this, pMem, pFileName, nLine);
	}
	template<typename T>
	inline void Free(T* pMem)
	{
		const static int index = 5;
		CallVFunc<void>(index, this, pMem);
	}
	template<typename T>
	inline size_t GetSize(T* pMem)
	{
		const static int index = 6;
		return CallVFunc<size_t>(index, this, pMem);
	}
};

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
class CStdMemAlloc : public IMemAlloc{};

inline CMemory p_CreateGlobalMemAlloc;
inline auto v_CreateGlobalMemAlloc = p_CreateGlobalMemAlloc.RCast<CStdMemAlloc* (*)(void)>();

inline CStdMemAlloc** g_pMemAllocSingleton = nullptr;


inline IMemAlloc* MemAllocSingleton()
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
		LogFunAdr("CreateGlobalMemAlloc", p_CreateGlobalMemAlloc.GetPtr());
		LogVarAdr("g_pMemAllocSingleton", reinterpret_cast<uintptr_t>(g_pMemAllocSingleton));
	}
	virtual void GetFun(void) const
	{
		p_CreateGlobalMemAlloc = g_GameDll.FindPatternSIMD("40 53 48 83 EC 20 BB ?? ?? ?? ?? 33 C0");
		v_CreateGlobalMemAlloc = p_CreateGlobalMemAlloc.RCast<CStdMemAlloc* (*)(void)>();    /*40 53 48 83 EC 20 BB ?? ?? ?? ?? 33 C0*/
	}
	virtual void GetVar(void) const
	{
		g_pMemAllocSingleton = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 57 48 81 EC ?? ?? ?? ?? 41 8B D8")
			.OffsetSelf(0x5A).FindPatternSelf("48 8B", CMemory::Direction::DOWN, 100).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CStdMemAlloc**>();
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

#endif // MEMSTD_H
