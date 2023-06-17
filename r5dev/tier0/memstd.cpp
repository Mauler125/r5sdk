//=============================================================================//
// 
// Purpose:
// 
//=============================================================================//
#include "memstd.h"

//=============================================================================//
// reimplementation of standard C functions for callbacks
// ----------------------------------------------------------------------------
// 
// Ideally this didn't exist, but since 'CreateGlobalMemAlloc' is part of the
// monolithic game executable, it couldn't be imported early enough to bind
// the C functions to feature the internal memalloc system instead. This code
// basically replicates the compiled code in the executable, and can be used
// to set callbacks up to allow hooking external code with internal without
// having to change the source code.
// 
//=============================================================================//
extern "C" void* R_malloc(size_t nSize)
{
    Assert(nSize);
    return MemAllocSingleton()->Alloc<void>(nSize);
}

extern "C" void R_free(void* pBlock)
{
    Assert(pBlock);
    MemAllocSingleton()->Free(pBlock);
}

extern "C" void* R_realloc(void* pBlock, size_t nSize)
{
    Assert(pBlock && nSize);

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

    const size_t nLen = strlen(pString) + 1;
    void* pNew = MemAllocSingleton()->Alloc<char>(nLen);

    if (!pNew)
        return nullptr;

    return reinterpret_cast<char*>(memcpy(pNew, pString, nLen));
}

extern "C" void* R_calloc(size_t nCount, size_t nSize)
{
    Assert(nCount && nSize);

    const size_t nTotal = nCount * nSize;
    void* pNew = MemAllocSingleton()->Alloc<void>(nTotal);

    memset(pNew, NULL, nTotal);
    return pNew;
}
