//=============================================================================//
// 
// Purpose:
// 
//=============================================================================//
#include "memstd.h"

static bool s_bAllocatorInitialized = false;
static void InitAllocator()
{
    if (!s_bAllocatorInitialized)
    {
        s_bAllocatorInitialized = true;

        // https://en.wikipedia.org/wiki/Win32_Thread_Information_Block
        const PEB64* processEnvBlock = reinterpret_cast<PEB64*>(__readgsqword(0x60));
        const QWORD imageBase = processEnvBlock->ImageBaseAddress;

        CreateGlobalMemAlloc = CModule::GetExportedSymbol(imageBase,
            "CreateGlobalMemAlloc").RCast<CStdMemAlloc* (*)(void)>();

        g_pMemAllocSingleton = CModule::GetExportedSymbol(imageBase,
            "g_pMemAllocSingleton").DerefSelf().RCast<CStdMemAlloc*>();
    }
}

//=============================================================================//
// Reimplementation of standard C functions for memalloc callbacks
// ---------------------------------------------------------------------------
// The replacement functions use the game's internal memalloc system instead
//=============================================================================//
// !TODO: other 'new' operators introduced in C++17.
void* operator new(std::size_t n) noexcept(false)
{
    return malloc(n);
}
void operator delete(void* p) throw()
{
    return free(p);
}

extern "C"
{
    __declspec(restrict) void* __cdecl _malloc_base(size_t const nSize)
    {
        InitAllocator();
        return MemAllocSingleton()->Alloc(nSize);
    }
    __declspec(restrict) void* __cdecl _calloc_base(size_t const nCount, size_t const nSize)
    {
        InitAllocator();

        const size_t nTotal = nCount * nSize;
        void* pNew = MemAllocSingleton()->Alloc(nTotal);

        memset(pNew, NULL, nTotal);
        return pNew;
    }
    __declspec(restrict) void* __cdecl _realloc_base(void* const pBlock, size_t const nSize)
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
    __declspec(restrict) void* __cdecl _recalloc_base(void* const pBlock, size_t const nCount, size_t const nSize)
    {
        InitAllocator();

        void* pMemOut = MemAllocSingleton()->Realloc(pBlock, nSize);

        if (!pBlock)
            memset(pMemOut, NULL, nSize);

        return pMemOut;
    }
    __declspec(noinline) void __cdecl _free_base(void* const pBlock)
    {
        InitAllocator();
        MemAllocSingleton()->Free(pBlock);
    }
    __declspec(noinline) size_t __cdecl _msize_base(void* const pBlock)
    {
        InitAllocator();

        size_t nSize = MemAllocSingleton()->GetSize(pBlock);
        return nSize;
    }
    char* __cdecl _strdup(const char* pString)
    {
        InitAllocator();

        const size_t nLen = strlen(pString) + 1;
        void* pNew = MemAllocSingleton()->Alloc(nLen);

        if (!pNew)
            return nullptr;

        return reinterpret_cast<char*>(memcpy(pNew, pString, nLen));
    }
    void* __cdecl _expand_base(void* const pBlock, size_t const nNewSize, int const nBlockUse)
    {
        // Expanding isn't supported!!!
        Assert(0);
        return NULL;
    }
}


extern "C"
{
    __declspec(restrict) void* __cdecl _malloc_dbg(size_t const nSize, int const, char const* const, int const)
    {
        return _malloc_base(nSize);
    }
    __declspec(restrict) void* __cdecl _calloc_dbg(size_t const nCount, size_t const nSize, int const, char const* const, int const)
    {
        return _calloc_base(nCount, nSize);
    }
    __declspec(restrict) void* __cdecl _realloc_dbg(void* const pBlock, size_t const nSize, int const, char const* const, int const)
    {
        return _realloc_base(pBlock, nSize);
    }
    __declspec(restrict) void* __cdecl _recalloc_dbg(void* const pBlock, size_t const nCount, size_t const nSize, int const, char const* const, int const)
    {
        return _recalloc_base(pBlock, nCount, nSize);
    }
    __declspec(noinline) void __cdecl _free_dbg(void* const pBlock, int const)
    {
        return _free_base(pBlock);
    }
    __declspec(noinline) size_t __cdecl _msize_dbg(void* const pBlock, int const)
    {
        return _msize_base(pBlock);
    }
    void* __cdecl _expand_dbg(void* pBlock, size_t nNewSize, int nBlockUse,
        const char* pFileName, int nLine)
    {
        // Expanding isn't supported!!!
        Assert(0);
        return NULL;
    }
}
