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
extern "C" void* R_malloc(size_t nSize)
{
    Assert(nSize);
    InitAllocator();
    return MemAllocSingleton()->Alloc<void>(nSize);
}

extern "C" void R_free(void* pBlock)
{
    //Assert(pBlock);
    InitAllocator();
    MemAllocSingleton()->Free(pBlock);
}

extern "C" void* R_realloc(void* pBlock, size_t nSize)
{
    //Assert(pBlock && nSize);

    InitAllocator();

    if (nSize)
        return MemAllocSingleton()->Realloc<void>(pBlock, nSize);
    else
    {
        MemAllocSingleton()->FreeDbg(pBlock, "tier0_static128", 0);
        return nullptr;
    }
}

extern "C" char* R_strdup(const char* pString)
{
    Assert(pString);

    InitAllocator();

    const size_t nLen = strlen(pString) + 1;
    void* pNew = MemAllocSingleton()->Alloc<char>(nLen);

    if (!pNew)
        return nullptr;

    return reinterpret_cast<char*>(memcpy(pNew, pString, nLen));
}

extern "C" void* R_calloc(size_t nCount, size_t nSize)
{
    Assert(nCount && nSize);

    InitAllocator();

    const size_t nTotal = nCount * nSize;
    void* pNew = MemAllocSingleton()->Alloc<void>(nTotal);

    memset(pNew, NULL, nTotal);
    return pNew;
}

// !TODO: other 'new' operators introduced in C++17.
void* operator new(std::size_t n) noexcept(false)
{
    return malloc(n);
}
void operator delete(void* p) throw()
{
    return free(p);
}
