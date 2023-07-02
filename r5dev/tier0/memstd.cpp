//=============================================================================//
// 
// Purpose: Memory allocation override functions
//-----------------------------------------------------------------------------
// The replacement functions use the game's internal memalloc system instead
// of the CRT one, this allows for sharing allocated memory directly between
// the SDK and the engine itself.
// 
//=============================================================================//
#include "memstd.h"

//-----------------------------------------------------------------------------
// Purpose: initialize the global memory allocator singleton pointer
//-----------------------------------------------------------------------------
static bool s_bAllocatorInitialized = false;
static void InitAllocator()
{
    if (!s_bAllocatorInitialized)
    {
        s_bAllocatorInitialized = true;
        const QWORD imageBase = CModule::GetProcessEnvironmentBlock()->ImageBaseAddress;

        CreateGlobalMemAlloc = CModule::GetExportedSymbol(imageBase,
            "CreateGlobalMemAlloc").RCast<CStdMemAlloc* (*)(void)>();

        g_pMemAllocSingleton = CModule::GetExportedSymbol(imageBase,
            "g_pMemAllocSingleton").DerefSelf().RCast<CStdMemAlloc*>();
    }
}

//-----------------------------------------------------------------------------
// Purpose: new/delete operator override
//-----------------------------------------------------------------------------
void* operator new(size_t const nSize) noexcept(false)
{
    return malloc(nSize);
}
void* __cdecl operator new(size_t const nSize,
    int const nBlockUse, const char* const pFileName, int const nLine)
{
    NOTE_UNUSED(nBlockUse);

    InitAllocator();
    return MemAllocSingleton()->InternalAlloc(nSize, pFileName, nLine);
}
void operator delete(void* const pBlock) throw()
{
    return free(pBlock);
}

//-----------------------------------------------------------------------------
extern "C"
{
    //-------------------------------------------------------------------------
    // Base overrides
    //-------------------------------------------------------------------------
    __declspec(restrict) void* __cdecl _malloc_base(size_t const nSize)
    {
        InitAllocator();
        return MemAllocSingleton()->Alloc(nSize);
    }
    //-------------------------------------------------------------------------
    __declspec(restrict) void* __cdecl _calloc_base(size_t const nCount,
        size_t const nSize)
    {
        InitAllocator();

        size_t const nTotal = nCount * nSize;
        void* const pNew = MemAllocSingleton()->Alloc(nTotal);

        memset(pNew, NULL, nTotal);
        return pNew;
    }
    //-------------------------------------------------------------------------
    __declspec(restrict) void* __cdecl _realloc_base(void* const pBlock,
        size_t const nSize)
    {
        InitAllocator();

        if (nSize)
            return MemAllocSingleton()->Realloc(pBlock, nSize);
        else
        {
            MemAllocSingleton()->InternalFree(pBlock, "tier0_static128", 0);
            return nullptr;
        }
    }
    //-------------------------------------------------------------------------
    __declspec(restrict) void* __cdecl _recalloc_base(void* const pBlock,
        size_t const nCount, size_t const nSize)
    {
        InitAllocator();

        const size_t nTotal = nCount * nSize;
        void* const pMemOut = MemAllocSingleton()->Realloc(pBlock, nTotal);

        if (!pBlock)
            memset(pMemOut, NULL, nTotal);

        return pMemOut;
    }
    //-------------------------------------------------------------------------
    __declspec(noinline) void __cdecl _free_base(void* const pBlock)
    {
        InitAllocator();
#if !(defined(_DEBUG) && !defined(USE_MEM_DEBUG))
        MemAllocSingleton()->Free(pBlock);
#else
        MemAllocSingleton()->InternalFree(pBlock, "tier0_static128", 0);
#endif // !_DEBUG && !USE_MEM_DEBUG
    }
    //-------------------------------------------------------------------------
    void* __cdecl _expand_base(void* const pBlock, size_t const nNewSize, int const nBlockUse)
    {
        // Expanding isn't supported!!!
        Assert(0);
        return NULL;
    }
    //-------------------------------------------------------------------------
    __declspec(noinline) size_t __cdecl _msize(void* const pBlock)
    {
        InitAllocator();
        return MemAllocSingleton()->GetSize(pBlock);
    }
    //-------------------------------------------------------------------------
    char* __cdecl _strdup(const char* const pString)
    {
        InitAllocator();

        const size_t nLen = strlen(pString) + 1;
        void* const pNew = MemAllocSingleton()->Alloc(nLen);

        if (!pNew)
            return nullptr;

        return reinterpret_cast<char*>(memcpy(pNew, pString, nLen));
    }

    //-------------------------------------------------------------------------
    // CRT overrides
    //-------------------------------------------------------------------------
    void* __cdecl _heap_alloc(size_t const nSize)
    {
        return _malloc_base(nSize);
    }
    void* __cdecl _malloc_crt(size_t const nSize)
    {
        return _malloc_base(nSize);
    }
    void* __cdecl _calloc_crt(size_t const nCount, size_t const nSize)
    {
        return _calloc_base(nCount, nSize);
    }
    void* __cdecl _realloc_crt(void* const pBlock, size_t const nSize)
    {
        return _realloc_base(pBlock, nSize);
    }
    void* __cdecl _recalloc_crt(void* const pBlock, size_t const nCount, size_t const nSize)
    {
        return _recalloc_base(pBlock, nSize, nCount);
    }

#if (defined(_DEBUG) || defined(USE_MEM_DEBUG))
    //-------------------------------------------------------------------------
    // Debug overrides
    //-------------------------------------------------------------------------
    __declspec(noinline) void* __cdecl _malloc_dbg(size_t const nSize,
        int const nBlockUse, const char* const pFileName, int const nLine)
    {
        NOTE_UNUSED(nBlockUse);
        InitAllocator();

        return MemAllocSingleton()->InternalAlloc(nSize, pFileName, nLine);
    }
    //-------------------------------------------------------------------------
    __declspec(noinline) void* __cdecl _calloc_dbg(size_t const nCount,
        size_t const nSize, int const nBlockUse, const char* const pFileName,
        int const nLine)
    {
        NOTE_UNUSED(nBlockUse);
        InitAllocator();

        size_t const nTotal = nCount * nSize;
        void* const pNew = MemAllocSingleton()->InternalAlloc(nTotal, pFileName, nLine);

        memset(pNew, NULL, nTotal);
        return pNew;
    }
    //-------------------------------------------------------------------------
    __declspec(noinline) void* __cdecl _realloc_dbg(void* const pBlock,
        size_t const nSize, int const nBlockUse, const char* const pFileName,
        int const nLine)
    {
        NOTE_UNUSED(nBlockUse);
        InitAllocator();

        if (nSize)
            return MemAllocSingleton()->Realloc(pBlock, nSize);
        else
        {
            MemAllocSingleton()->InternalFree(pBlock, pFileName, nLine);
            return nullptr;
        }
    }
    //-------------------------------------------------------------------------
    __declspec(noinline) void* __cdecl _recalloc_dbg(void* const pBlock,
        size_t const nCount, size_t const nSize, int const nBlockUse,
        const char* const pFileName, int const nLine)
    {
        NOTE_UNUSED(nBlockUse);
        InitAllocator();

        const size_t nTotal = nCount * nSize;
        void* const pMemOut = MemAllocSingleton()->InternalRealloc(pBlock, nTotal,
            pFileName, nLine);

        if (!pBlock)
            memset(pMemOut, NULL, nTotal);

        return pMemOut;
    }
    //-------------------------------------------------------------------------
    __declspec(noinline) void __cdecl _free_dbg(void* const pBlock,
        int const nBlockUse)
    {
        NOTE_UNUSED(nBlockUse);
        InitAllocator();

        MemAllocSingleton()->InternalFree(pBlock, "tier0_static128", 0);
    }
    //-------------------------------------------------------------------------
    void* __cdecl _expand_dbg(void* pBlock, size_t nNewSize, int nBlockUse,
        const char* const pFileName, int nLine)
    {
        NOTE_UNUSED(pFileName);
        NOTE_UNUSED(nLine);

        return _expand_base(pBlock, nNewSize, nBlockUse);
    }
    //-------------------------------------------------------------------------
    __declspec(noinline) size_t __cdecl _msize_dbg(void* const pBlock, int const)
    {
        return _msize(pBlock);
    }
    //-------------------------------------------------------------------------
    void* __cdecl _heap_alloc_dbg(size_t nSize, int nBlockUse, const char* szFileName, int nLine)
    {
        return _malloc_dbg(nSize, nBlockUse, szFileName, nLine);
    }

    //-------------------------------------------------------------------------
    // CRT debug nolocks
    //-------------------------------------------------------------------------
    void __cdecl _free_nolock(void* pUserData)
    {
        // I don't think the second param is used in memoverride
        _free_dbg(pUserData, 0);
    }
    void __cdecl _free_dbg_nolock(void* pUserData, int nBlockUse)
    {
        _free_dbg(pUserData, 0);
    }

    //-------------------------------------------------------------------------
    // CRT debug stubs
    //-------------------------------------------------------------------------
    _CRT_ALLOC_HOOK __cdecl _CrtGetAllocHook(void)
    {
        Assert(0);
        return NULL;
    }
    int __cdecl CheckBytes(unsigned char* pb, unsigned char bCheck, size_t nSize)
    {
        int bOkay = TRUE;
        return bOkay;
    }
    _CRT_DUMP_CLIENT __cdecl _CrtGetDumpClient(void)
    {
        Assert(0);
        return NULL;
    }
    void __cdecl _printMemBlockData(_locale_t plocinfo, _CrtMemBlockHeader* pHead)
    {
    }
    void __cdecl _CrtMemDumpAllObjectsSince_stat(const _CrtMemState* state, _locale_t plocinfo)
    {
        // Might need to be renamed to '_CrtMemDumpAllObjectsSince'
        // instead of '_CrtMemDumpAllObjectsSince_stat'.
    }
#endif // _DEBUG || USE_MEM_DEBUG
}
