//=====================================================================================//
//
// model loading and caching
//
// $NoKeywords: $
//=====================================================================================//

#include "core/stdafx.h"
#include "datacache/mdlcache.h"
#include "datacache/imdlcache.h"
#include "engine/sys_utils.h"
#include "rtech/rtech_utils.h"
#include "public/include/studio.h"


//-----------------------------------------------------------------------------
// Purpose: finds an MDL
// Input  : *this - 
//          handle - 
//          *a3 - 
// Output : a pointer to the studiohdr_t object
//-----------------------------------------------------------------------------
studiohdr_t* CMDLCache::FindMDL(CMDLCache* pMDLCache, MDLHandle_t handle, void* a3)
{
    __int64          v4; // rbp
    __int64          v6; // rbx
    __int64*         v7; // rax
    studiohdr_t* result; // rax

    v4 = handle;
    EnterCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(&*m_MDLMutex));
    v6 = *(_QWORD*)(m_MDLDict.Deref().GetPtr() + 24 * v4 + 16);
    LeaveCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(&*m_MDLMutex));

    if (!g_pMDLFallback->m_hErrorMDL || !g_pMDLFallback->m_hEmptyMDL)
    {
        studiohdr_t* pStudioHDR = **(studiohdr_t***)v6;
        string svStudio = ConvertToUnixPath(string(pStudioHDR->name));

        if (strcmp(svStudio.c_str(), ERROR_MODEL) == 0)
        {
            g_pMDLFallback->m_pErrorHDR = pStudioHDR;
            g_pMDLFallback->m_hErrorMDL = handle;
        }
        if (strcmp(svStudio.c_str(), EMPTY_MODEL) == 0)
        {
            g_pMDLFallback->m_pEmptyHDR = pStudioHDR;
            g_pMDLFallback->m_hEmptyMDL = handle;
        }
    }

    if (!v6)
    {
        if (!g_pMDLFallback->m_hErrorMDL)
            Error(eDLL_T::ENGINE, "Model with handle \"%hu\" not found and \"%s\" couldn't be loaded.\n", handle, ERROR_MODEL);

        return g_pMDLFallback->m_pErrorHDR;
    }

    if ((*(_WORD*)(v6 + 18) & 0x600) != 0)
    {
        v7 = *(__int64**)v6;
        if (*(_QWORD*)v6)
        {
            if (a3)
            {
                CMDLCache::FindCachedMDL(pMDLCache, (void*)v6, a3);
                v7 = *(__int64**)v6;
            }
        LABEL_6:
            result = (studiohdr_t*)*v7;
            if (result)
                return result;

            return CMDLCache::FindUncachedMDL(pMDLCache, v4, (void*)v6, a3);
        }
        v7 = *(__int64**)(v6 + 8);
        if (v7)
            goto LABEL_6;
    }
    return CMDLCache::FindUncachedMDL(pMDLCache, v4, (void*)v6, a3);
}

//-----------------------------------------------------------------------------
// Purpose: finds an MDL cached
// Input  : *this - 
//          *a2 - 
//          *a3 - 
//-----------------------------------------------------------------------------
void CMDLCache::FindCachedMDL(CMDLCache* pMDLCache, void* a2, void* a3)
{
    v_CMDLCache__FindCachedMDL(pMDLCache, a2, a3);
}

//-----------------------------------------------------------------------------
// Purpose: finds an MDL uncached
// Input  : *this - 
//          handle - 
//          *a3 - 
//          *a4
// Output : a pointer to the studiohdr_t object
//-----------------------------------------------------------------------------
studiohdr_t* CMDLCache::FindUncachedMDL(CMDLCache* cache, MDLHandle_t handle, void* a3, void* a4)
{
    const char*    v8; // rdi
    __int64        v9; // rax
    int           v10; // eax
    const char*   v11; // r11
    int           v12; // eax
    const char*   v13; // r11
    int           v14; // eax
    bool          v16; // zf
    studiohdr_t*  v17; // rdi
    studiohdr_t** v18; // rax

    //CThreadFastMutexSlow::WaitForLock(a3 + 0x80);
    EnterCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(&*m_MDLMutex));
    void* modelCache = cache->m_pModelCacheSection;
    v8 = (const char*)(*(_QWORD*)((int64)modelCache + 24 * static_cast<int64>(handle) + 8));
    LeaveCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(&*m_MDLMutex));
    v9 = -1i64;
    do
        ++v9;
    while (v8[v9]);

    if (v9 < 5 ||
        (_stricmp(&v8[v9 - 5], ".rmdl") != 0) &&
        (_stricmp(&v8[v9 - 5], ".rrig") != 0) &&
        (_stricmp(&v8[v9 - 5], ".rpak") != 0))
    {
        Error(eDLL_T::ENGINE, "Attempted to load old model \"%s\"; replace with rmdl.\n", v8);
        goto LABEL_ERROR;
    }

    g_pRTech->StringToGuid(v8);
    v16 = *(_QWORD*)a3 == 0i64;
    *(_BYTE*)((int64)a3 + 152) = 0;
    if (v16)
    {
        v18 = *(studiohdr_t***)((int64)a3 + 8);
        if (v18)
        {
            v17 = *v18;
        }
        else
        {
            LABEL_ERROR:
            if (g_pMDLFallback->m_hErrorMDL)
                Error(eDLL_T::ENGINE, "Model \"%s\" not found; replacing with \"%s\".\n", v8, ERROR_MODEL);
            else
                Error(eDLL_T::ENGINE, "Model \"%s\" not found and \"%s\" couldn't be loaded.\n", v8, ERROR_MODEL);
            v17 = g_pMDLFallback->m_pErrorHDR;
        }
    }
    else
    {
        v_CMDLCache__FindCachedMDL(cache, a3, a4);
        v17 = **(studiohdr_t***)a3;
    }
    //CThreadFastMutexSlow::ReleaseWaiter(a3 + 128);
    return v17;
}

studiohdr_t* CMDLCache::GetStudioHdr(CMDLCache* pMDLCache, MDLHandle_t handle)
{
    __int64 v2; // rbx
    __int64 v3; // rbx
    __int64 v4; // rdx
    studiohdr_t* result; // rax

    if (!handle)
    {
        if (!g_pMDLFallback->m_hErrorMDL)
            Error(eDLL_T::ENGINE, "Model with handle \"%hu\" not found and \"%s\" couldn't be loaded.\n", handle, ERROR_MODEL);

        return g_pMDLFallback->m_pErrorHDR;
    }

    v2 = handle;
    EnterCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(&*m_MDLMutex));
    v3 = *(_QWORD*)(m_MDLDict.Deref().GetPtr() + 24 * v2 + 16);
    LeaveCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(&*m_MDLMutex));
    v4 = *(_QWORD*)(*(_QWORD*)(*(_QWORD*)v3 + 8i64) + 24i64);
    result = (studiohdr_t*)(v4 + 16);
    if (!v4)
        result = nullptr;;
    return result;
}

void MDLCache_Attach()
{
    DetourAttach((LPVOID*)&v_CMDLCache__FindMDL, &CMDLCache::FindMDL);
    DetourAttach((LPVOID*)&v_CMDLCache__FindUncachedMDL, &CMDLCache::FindUncachedMDL);
    //DetourAttach((LPVOID*)&v_CMDLCache__GetStudioHdr, &CMDLCache::GetStudioHdr);
}

void MDLCache_Detach()
{
    DetourDetach((LPVOID*)&v_CMDLCache__FindMDL, &CMDLCache::FindMDL);
    DetourDetach((LPVOID*)&v_CMDLCache__FindUncachedMDL, &CMDLCache::FindUncachedMDL);
    //DetourDetach((LPVOID*)&v_CMDLCache__GetStudioHdr, &CMDLCache::GetStudioHdr);
}