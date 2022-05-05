//=====================================================================================//
//
// model loading and caching
//
// $NoKeywords: $
//=====================================================================================//

#include "core/stdafx.h"
#include "tier0/threadtools.h"
#include "tier1/cvar.h"
#include "datacache/mdlcache.h"
#include "datacache/imdlcache.h"
#include "engine/sys_utils.h"
#include "rtech/rtech_utils.h"
#include "public/include/studio.h"
#include "tier1/utldict.h"


//-----------------------------------------------------------------------------
// Purpose: finds an MDL
// Input  : *this - 
//          handle - 
//          *a3 - 
// Output : a pointer to the studiohdr_t object
//-----------------------------------------------------------------------------
studiohdr_t* CMDLCache::FindMDL(CMDLCache* cache, MDLHandle_t handle, void* a3)
{
    studiodata_t* pStudioData; // rbx
    void*         pMDLCache;   // rax
    studiohdr_t*  pStudioHdr;  // rax

    EnterCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(&*m_MDLMutex));
    auto mdlDict = CUtlDict<studiodata_t*, MDLHandle_t>(m_MDLDict.Deref().GetPtr());
    pStudioData = mdlDict.Find(static_cast<int64>(handle));
    LeaveCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(&*m_MDLMutex));

    if (!g_pMDLFallback->m_hErrorMDL || !g_pMDLFallback->m_hEmptyMDL)
    {
        studiohdr_t* pStudioHDR = **reinterpret_cast<studiohdr_t***>(pStudioData);
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

    if (!pStudioData)
    {
        if (!g_pMDLFallback->m_hErrorMDL)
            Error(eDLL_T::ENGINE, "Model with handle \"%hu\" not found and \"%s\" couldn't be loaded.\n", handle, ERROR_MODEL);

        return g_pMDLFallback->m_pErrorHDR;
    }

    int nFlags = STUDIOHDR_FLAGS_NEEDS_DEFERRED_ADDITIVE | STUDIOHDR_FLAGS_OBSOLETE;
    if ((pStudioData->m_nFlags & nFlags))
    {
        pMDLCache = *reinterpret_cast<void**>(pStudioData);
        if (pStudioData->m_MDLCache)
        {
            if (a3)
            {
                FindCachedMDL(cache, pStudioData, a3);
                pMDLCache = *reinterpret_cast<void**>(pStudioData);
            }
        LABEL_6:
            pStudioHdr = *reinterpret_cast<studiohdr_t**>(pMDLCache);
            if (pStudioHdr)
                return pStudioHdr;

            return FindUncachedMDL(cache, handle, pStudioData, a3);
        }
        pMDLCache = pStudioData->m_pAnimData;
        if (pMDLCache)
            goto LABEL_6;
    }
    return FindUncachedMDL(cache, handle, pStudioData, a3);
}

//-----------------------------------------------------------------------------
// Purpose: finds an MDL cached
// Input  : *this - 
//          *a2 - 
//          *a3 - 
//-----------------------------------------------------------------------------
void CMDLCache::FindCachedMDL(CMDLCache* cache, studiodata_t* pStudioData, void* a3)
{
    __int64 v6; // rax

    if (a3)
    {
        pStudioData->m_Mutex.WaitForLock();
        *(_QWORD*)((int64_t)a3 + 0x880) = *(_QWORD*)&pStudioData->pad[0x24];
        v6 = *(_QWORD*)&pStudioData->pad[0x24];
        if (v6)
            *(_QWORD*)(v6 + 0x878) = (int64_t)a3;
        *(_QWORD*)&pStudioData->pad[0x24] = (int64_t)a3;
        *(_QWORD*)((int64_t)a3 + 0x870) = (int64_t)cache;
        *(_WORD*)((int64_t)a3 + 0x888) = pStudioData->m_Handle;
        pStudioData->m_Mutex.ReleaseWaiter();
    }
}

//-----------------------------------------------------------------------------
// Purpose: finds an MDL uncached
// Input  : *this - 
//          handle - 
//          *a3 - 
//          *a4 - 
// Output : a pointer to the studiohdr_t object
//-----------------------------------------------------------------------------
studiohdr_t* CMDLCache::FindUncachedMDL(CMDLCache* cache, MDLHandle_t handle, studiodata_t* pStudioData, void* a4)
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
    bool    bOldModel     {};
    bool    bInvalidHandle{};

    pStudioData->m_Mutex.WaitForLock();
    EnterCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(&*m_MDLMutex));
    void* modelCache = cache->m_pModelCacheSection;
    v8 = (const char*)(*(_QWORD*)((int64)modelCache + 24 * static_cast<int64>(handle) + 8));
    LeaveCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(&*m_MDLMutex));
    if (IsBadReadPtrV2((void*)v8))
    {
        bInvalidHandle = true;
        goto LABEL_ERROR;
    }

    v9 = -1i64;
    do
        ++v9;
    while (v8[v9]);

    if (v9 < 5 ||
        (_stricmp(&v8[v9 - 5], ".rmdl") != 0) &&
        (_stricmp(&v8[v9 - 5], ".rrig") != 0) &&
        (_stricmp(&v8[v9 - 5], ".rpak") != 0))
    {
        bOldModel = true;
        goto LABEL_ERROR;
    }

    g_pRTech->StringToGuid(v8);
    v16 = *(_QWORD*)pStudioData == 0i64;
    *(_BYTE*)((int64)pStudioData + 152) = 0;
    if (v16)
    {
        v18 = *(studiohdr_t***)((int64)pStudioData + 8);
        if (v18)
        {
            v17 = *v18;
        }
        else
        {
        LABEL_ERROR:
            if (std::find(g_BadMDLHandles.begin(), g_BadMDLHandles.end(), handle) == g_BadMDLHandles.end())
            {
                if (bInvalidHandle)
                    Error(eDLL_T::ENGINE, "Model with handle \"hu\" not found; replacing with \"%s\".\n", handle, ERROR_MODEL);
                else if (bOldModel)
                    Error(eDLL_T::ENGINE, "Attempted to load old model \"%s\"; replace with rmdl.\n", v8);
                else
                {
                    if (g_pMDLFallback->m_hErrorMDL)
                        Error(eDLL_T::ENGINE, "Model \"%s\" not found; replacing with \"%s\".\n", v8, ERROR_MODEL);
                    else
                        Error(eDLL_T::ENGINE, "Model \"%s\" not found and \"%s\" couldn't be loaded.\n", v8, ERROR_MODEL);
                }

                g_BadMDLHandles.push_back(handle);
            }
            v17 = g_pMDLFallback->m_pErrorHDR;
            old_gather_props->SetValue(true); // mdl/error.rmdl fallback is not supported (yet) in the new GatherProps solution!
        }
    }
    else
    {
        v_CMDLCache__FindCachedMDL(cache, pStudioData, a4);
        if ((__int64)*(studiohdr_t**)pStudioData)
        {
            if ((__int64)*(studiohdr_t**)pStudioData == 0xDEADFEEDDEADFEED)
                v17 = g_pMDLFallback->m_pErrorHDR;
            else
                v17 = **(studiohdr_t***)pStudioData;
        }
        else
            v17 = g_pMDLFallback->m_pErrorHDR;
    }
    pStudioData->m_Mutex.ReleaseWaiter();
    return v17;
}

//-----------------------------------------------------------------------------
// Purpose: gets the studiohdr from cache pool by handle
// Input  : *this - 
//          handle - 
// Output : a pointer to the studiohdr_t object
//-----------------------------------------------------------------------------
studiohdr_t* CMDLCache::GetStudioHDR(CMDLCache* pMDLCache, MDLHandle_t handle)
{
    __int64 v2; // rbx
    __int64 v3; // rbx
    __int64 v4; // rdx
    studiohdr_t* result = nullptr; // rax

    if (!handle)
    {
        LABEL_ERROR:
        if (!g_pMDLFallback->m_hErrorMDL)
            Error(eDLL_T::ENGINE, "Model with handle \"%hu\" not found and \"%s\" couldn't be loaded.\n", handle, ERROR_MODEL);

        return g_pMDLFallback->m_pErrorHDR;
    }

    v2 = handle;
    EnterCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(&*m_MDLMutex));
    v3 = *(_QWORD*)(m_MDLDict.Deref().GetPtr() + 24 * v2 + 16);
    LeaveCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(&*m_MDLMutex));
    if (*(_QWORD*)(v3))
    {
        v4 = *(_QWORD*)(*(_QWORD*)(*(_QWORD*)v3 + 8i64) + 24i64);
        if (v4)
            result = (studiohdr_t*)(v4 + 16);
    }
    return result;
}

//-----------------------------------------------------------------------------
// Purpose: gets the studio hardware data reference from cache pool by handle
// Input  : *this - 
//          handle - 
// Output : a pointer to the CStudioHWDataRef object
//-----------------------------------------------------------------------------
CStudioHWDataRef* CMDLCache::GetStudioHardwareRef(CMDLCache* cache, MDLHandle_t handle)
{
    __int64 v2; // rbx
    __int64 v3; // rdi
    __int64 v4; // rbx
    __int64 result; // rax

    v2 = handle;
    EnterCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(&*m_MDLMutex));
    v3 = *(_QWORD*)(m_MDLDict.Deref().GetPtr() + 24 * v2 + 16);
    LeaveCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(&*m_MDLMutex));

    if (!v3)
    {
        if (!g_pMDLFallback->m_hErrorMDL)
        {
            Error(eDLL_T::ENGINE, "Studio hardware with handle \"%hu\" not found and \"%s\" couldn't be loaded.\n", handle, ERROR_MODEL);
            return nullptr;
        }
        v3 = *(_QWORD*)(m_MDLDict.Deref().GetPtr() + 24i64 * g_pMDLFallback->m_hErrorMDL + 16);
    }

    if (*(_QWORD*)v3)
    {
        if (*(_QWORD*)v3 == 0xDEADFEEDDEADFEED)
            return nullptr;

        v4 = *(_QWORD*)(*(_QWORD*)v3 + 8i64);
        AcquireSRWLockExclusive(reinterpret_cast<PSRWLOCK>(&*m_MDLLock));
        v_CStudioHWDataRef__SetFlags(reinterpret_cast<CStudioHWDataRef*>(v4), 1i64); // !!! DECLARED INLINE IN < S3 !!!
        ReleaseSRWLockExclusive(reinterpret_cast<PSRWLOCK>(&*m_MDLLock));
    }
    if ((*(_BYTE*)(v3 + 18) & 1) != 0)
        result = *(_QWORD*)(v3 + 0x20) + 16i64;
    else
        result = 0i64;
    return reinterpret_cast<CStudioHWDataRef*>(result);
}

//-----------------------------------------------------------------------------
// Purpose: gets the studio material glue from cache pool by handle
// Input  : *this - 
//          handle - 
// Output : a pointer to the CMaterialGlue object
//-----------------------------------------------------------------------------
void* CMDLCache::GetStudioMaterialGlue(CMDLCache* cache, MDLHandle_t handle)
{
    __int64 v2; // rbx
    __int64 v3; // rbx

    v2 = handle;
    EnterCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(&*m_MDLMutex));
    v3 = *(_QWORD*)(m_MDLDict.Deref().GetPtr() + 24 * v2 + 16);
    LeaveCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(&*m_MDLMutex));
    return (char*)v3 + 40;
}

void MDLCache_Attach()
{
    DetourAttach((LPVOID*)&v_CMDLCache__FindMDL, &CMDLCache::FindMDL);
    DetourAttach((LPVOID*)&v_CMDLCache__FindCachedMDL, &CMDLCache::FindCachedMDL);
    DetourAttach((LPVOID*)&v_CMDLCache__FindUncachedMDL, &CMDLCache::FindUncachedMDL);
    DetourAttach((LPVOID*)&v_CMDLCache__GetStudioHardwareRef, &CMDLCache::GetStudioHardwareRef);
    DetourAttach((LPVOID*)&v_CMDLCache__GetStudioHDR, &CMDLCache::GetStudioHDR);
}

void MDLCache_Detach()
{
    DetourDetach((LPVOID*)&v_CMDLCache__FindMDL, &CMDLCache::FindMDL);
    DetourDetach((LPVOID*)&v_CMDLCache__FindCachedMDL, &CMDLCache::FindCachedMDL);
    DetourDetach((LPVOID*)&v_CMDLCache__FindUncachedMDL, &CMDLCache::FindUncachedMDL);
    DetourDetach((LPVOID*)&v_CMDLCache__GetStudioHardwareRef, &CMDLCache::GetStudioHardwareRef);
    DetourDetach((LPVOID*)&v_CMDLCache__GetStudioHDR, &CMDLCache::GetStudioHDR);
}