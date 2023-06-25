#ifndef MEMSTD_H
#define MEMSTD_H

extern "C" void* R_malloc(size_t nSize);
extern "C" void  R_free(void* pBlock);
extern "C" void* R_realloc(void* pBlock, size_t nSize);
extern "C" char* R_strdup(const char* pString);
extern "C" void* R_calloc(size_t nCount, size_t nSize);

// Shadow standard implementation with ours.
#define malloc(nSize) R_malloc(nSize)
#define free(pBlock) R_free(pBlock)
#define realloc(pBlock, nSize) R_realloc(pBlock, nSize)
#define strdup(pString) R_strdup(pString)
#define calloc(nCount, nSize) R_calloc(nCount, nSize)

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
