#ifndef MEMSTD_H
#define MEMSTD_H

extern "C" void* R_malloc(size_t nSize);
extern "C" void  R_free(void* pBlock);
extern "C" void* R_realloc(void* pBlock, size_t nSize);
extern "C" char* R_strdup(const char* pString);
extern "C" void* R_calloc(size_t nCount, size_t nSize);
extern "C" size_t R_mallocsize(void* pBlock);

// Shadow standard implementation with ours.
#define malloc(nSize) R_malloc(nSize)
#define free(pBlock) R_free(pBlock)
#define realloc(pBlock, nSize) R_realloc(pBlock, nSize)
#define strdup(pString) R_strdup(pString)
#define calloc(nCount, nSize) R_calloc(nCount, nSize)
#define recalloc(pBlock, nSize) R_recalloc(pBlock, nSize)
#define mallocsize(pBlock) R_mallocsize(pBlock)

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
