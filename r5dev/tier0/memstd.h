#ifndef MEMSTD_H
#define MEMSTD_H

// this magic only works under win32
// under linux this malloc() overrides the libc malloc() and so we
// end up in a recursion (as MemAlloc_Alloc() calls malloc)
#if _MSC_VER >= 1400

#if _MSC_VER >= 1900
#define _CRTNOALIAS
#endif

#define ALLOC_CALL _CRTNOALIAS _CRTRESTRICT
#define FREE_CALL _CRTNOALIAS 
#else
#define ALLOC_CALL
#define FREE_CALL
#endif

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
class IMemAlloc
{
public:
	// Same functions internally.
	virtual void* InternalAlloc(size_t nSize, const char* pFileName, int nLine) = 0;
	virtual void* Alloc(size_t nSize) = 0;

	// Same functions internally.
	virtual void* InternalRealloc(void* pMem, size_t nSize, const char* pFileName, int nLine) = 0;
	virtual void* Realloc(void* pMem, size_t nSize) = 0;

	// Same as Free, but takes debug parameters.
	virtual void  InternalFree(void* pMem, const char* pFileName, int nLine) = 0;
	virtual void  Free(void* pMem) = 0;

	virtual size_t GetSize(void* pMem) = 0;
};

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
class CStdMemAlloc : public IMemAlloc{};

inline CStdMemAlloc* (*CreateGlobalMemAlloc)() = nullptr;
inline CStdMemAlloc* g_pMemAllocSingleton = nullptr;

inline IMemAlloc* MemAllocSingleton()
{
	if (!g_pMemAllocSingleton)
	{
		g_pMemAllocSingleton = CreateGlobalMemAlloc();
	}
	return g_pMemAllocSingleton;
}

#endif // MEMSTD_H
