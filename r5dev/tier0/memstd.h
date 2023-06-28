#ifndef MEMSTD_H
#define MEMSTD_H

extern "C"
{
	__declspec(restrict) void* __cdecl _malloc_base(size_t const nSize);
	__declspec(restrict) void* __cdecl _calloc_base(size_t const nCount, size_t const nSize);
    __declspec(restrict) void* __cdecl _realloc_base(void* const pBlock, size_t const nSize);
    __declspec(restrict) void* __cdecl _recalloc_base(void* const pBlock, size_t const nCount, size_t const nSize);
    __declspec(noinline) void __cdecl _free_base(void* const pBlock);
    __declspec(noinline) size_t __cdecl _msize_base(void* const pBlock);
    char* __cdecl _strdup(const char* pString);
	void* __cdecl _expand(void* pBlock, size_t nSize);
}

class IMemAlloc
{
public:
	// Same functions internally.
	virtual void* InternalAlloc(size_t nSize/*, const char* pFileName, int nLine*/) = 0;
	virtual void* Alloc(size_t nSize) = 0;

	// Same functions internally.
	virtual void* InternalRealloc(void* pMem, size_t nSize/*, const char* pFileName, int nLine*/) = 0;
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
