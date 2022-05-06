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
#include "datacache/idatacache.h"
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
    studiohdr_t*  pStudioHdr;  // rax

    EnterCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(&*m_MDLMutex));
    studiodata_t* pStudioData = m_MDLDict->Find(handle);
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
        pStudioHdr = GetErrorModel();

        if (!IsKnownBadModel(handle))
        {
            if (!pStudioHdr)
                Error(eDLL_T::ENGINE, "Model with handle \"hu\" not found and \"%s\" couldn't be loaded.\n", handle, ERROR_MODEL);
            else
                Error(eDLL_T::ENGINE, "Model with handle \"hu\" not found; replacing with \"%s\".\n", handle, ERROR_MODEL);

            g_BadMDLHandles.push_back(handle);
        }

        return pStudioHdr;
    }

    int nFlags = STUDIOHDR_FLAGS_NEEDS_DEFERRED_ADDITIVE | STUDIOHDR_FLAGS_OBSOLETE;
    if ((pStudioData->m_nFlags & nFlags))
    {
        void* pMDLCache = *reinterpret_cast<void**>(pStudioData);
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
    if (a3)
    {
        pStudioData->m_Mutex.WaitForLock();
        *(_QWORD*)((int64_t)a3 + 0x880) = *(_QWORD*)&pStudioData->pad[0x24];
        int64_t v6 = *(_QWORD*)&pStudioData->pad[0x24];
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
    studiohdr_t*   pStudioHdr; // rdi
    studiohdr_t** ppStudioHdr; // rax

    pStudioData->m_Mutex.WaitForLock();

    EnterCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(&*m_MDLMutex));
    void* pModelCache = cache->m_pModelCacheSection;
    char* szModelName = (char*)(*(_QWORD*)((int64)pModelCache + 24 * static_cast<int64>(handle) + 8));
    LeaveCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(&*m_MDLMutex));

    if (IsBadReadPtrV2(reinterpret_cast<void*>(szModelName)))
    {
        pStudioHdr = GetErrorModel();
        if (!IsKnownBadModel(handle))
        {
            if (!pStudioHdr)
                Error(eDLL_T::ENGINE, "Model with handle \"hu\" not found and \"%s\" couldn't be loaded.\n", handle, ERROR_MODEL);
            else
                Error(eDLL_T::ENGINE, "Model with handle \"hu\" not found; replacing with \"%s\".\n", handle, ERROR_MODEL);

            g_BadMDLHandles.push_back(handle);
        }

        pStudioData->m_Mutex.ReleaseWaiter();
        return pStudioHdr;
    }

    size_t nFileNameLen = strlen(szModelName);

    if (static_cast<int>(nFileNameLen) < 5 ||
        (_stricmp(&szModelName[nFileNameLen - 5], ".rmdl") != 0) &&
        (_stricmp(&szModelName[nFileNameLen - 5], ".rrig") != 0) &&
        (_stricmp(&szModelName[nFileNameLen - 5], ".rpak") != 0))
    {
        pStudioHdr = GetErrorModel();
        if (!IsKnownBadModel(handle))
        {
            if (!pStudioHdr)
                Error(eDLL_T::ENGINE, "Attempted to load old model \"%s\" and \"%s\" couldn't be loaded.\n", szModelName);
            else
                Error(eDLL_T::ENGINE, "Attempted to load old model \"%s\"; replacing with \"%s\".\n", szModelName);

            g_BadMDLHandles.push_back(handle);
        }

        pStudioData->m_Mutex.ReleaseWaiter();
        return pStudioHdr;
    }

    LOBYTE(pStudioData->m_nGuidLock) = 1;
    g_pRTech->StringToGuid(szModelName);
    LOBYTE(pStudioData->m_nGuidLock) = 0;

    if (!pStudioData->m_MDLCache)
    {
        ppStudioHdr = (studiohdr_t**)pStudioData->m_pAnimData;
        if (ppStudioHdr)
        {
            pStudioHdr = *ppStudioHdr;
        }
        else
        {
            pStudioHdr = GetErrorModel();
            if (!IsKnownBadModel(handle))
            {
                if (pStudioHdr)
                    Error(eDLL_T::ENGINE, "Model \"%s\" not found; replacing with \"%s\".\n", szModelName, ERROR_MODEL);
                else
                    Error(eDLL_T::ENGINE, "Model \"%s\" not found and \"%s\" couldn't be loaded.\n", szModelName, ERROR_MODEL);

                g_BadMDLHandles.push_back(handle);
            }

            pStudioData->m_Mutex.ReleaseWaiter();
            return pStudioHdr;
        }
    }
    else
    {
        v_CMDLCache__FindCachedMDL(cache, pStudioData, a4);
        if ((__int64)*(studiohdr_t**)pStudioData)
        {
            if ((__int64)*(studiohdr_t**)pStudioData == 0xDEADFEEDDEADFEED)
            {
                pStudioHdr = GetErrorModel();
                if (!IsKnownBadModel(handle))
                {
                    if (!pStudioHdr)
                        Error(eDLL_T::ENGINE, "Model \"%s\" has bad studio data and \"%s\" couldn't be loaded.\n", szModelName, ERROR_MODEL);
                    else
                        Error(eDLL_T::ENGINE, "Model \"%s\" has bad studio data; replacing with \"%s\".\n", szModelName, ERROR_MODEL);

                    g_BadMDLHandles.push_back(handle);
                }
            }
            else
                pStudioHdr = **(studiohdr_t***)pStudioData;
        }
        else
        {
            pStudioHdr = GetErrorModel();
            if (!IsKnownBadModel(handle))
            {
                if (!pStudioHdr)
                    Error(eDLL_T::ENGINE, "Model \"%s\" has no studio data and \"%s\" couldn't be loaded.\n", szModelName, ERROR_MODEL);
                else
                    Error(eDLL_T::ENGINE, "Model \"%s\" has no studio data; replacing with \"%s\".\n", szModelName, ERROR_MODEL);

                g_BadMDLHandles.push_back(handle);
            }
        }
    }
    pStudioData->m_Mutex.ReleaseWaiter();
    return pStudioHdr;
}

//-----------------------------------------------------------------------------
// Purpose: gets the studiohdr from cache pool by handle
// Input  : *this - 
//          handle - 
// Output : a pointer to the studiohdr_t object
//-----------------------------------------------------------------------------
studiohdr_t* CMDLCache::GetStudioHDR(CMDLCache* pMDLCache, MDLHandle_t handle)
{
    studiohdr_t* pStudioHdr = nullptr; // rax

    if (!handle)
    {
        pStudioHdr = GetErrorModel();
        if (!pStudioHdr)
            Error(eDLL_T::ENGINE, "Attempted to load model with no handle and \"%s\" couldn't be loaded.\n", ERROR_MODEL);

        return pStudioHdr;
    }

    EnterCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(&*m_MDLMutex));
    studiodata_t* pStudioData = m_MDLDict->Find(handle);
    LeaveCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(&*m_MDLMutex));
    if (*(_QWORD*)(pStudioData))
    {
        void* v4 = *(void**)(*((_QWORD*)pStudioData->m_MDLCache + 1) + 24i64);
        if (v4)
            pStudioHdr = (studiohdr_t*)((char*)v4 + 0x10);
    }
    return pStudioHdr;
}

//-----------------------------------------------------------------------------
// Purpose: gets the studio hardware data from cache pool by handle
// Input  : *this - 
//          handle - 
// Output : a pointer to the studiohwdata_t object
//-----------------------------------------------------------------------------
studiohwdata_t* CMDLCache::GetHardwareData(CMDLCache* cache, MDLHandle_t handle)
{
    EnterCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(&*m_MDLMutex));
    studiodata_t* pStudioData = m_MDLDict->Find(handle);
    LeaveCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(&*m_MDLMutex));

    if (!pStudioData)
    {
        if (!g_pMDLFallback->m_hErrorMDL)
        {
            Error(eDLL_T::ENGINE, "Studio hardware with handle \"%hu\" not found and \"%s\" couldn't be loaded.\n", handle, ERROR_MODEL);
            return nullptr;
        }
        pStudioData = m_MDLDict->Find(g_pMDLFallback->m_hErrorMDL);
    }

    if (pStudioData->m_MDLCache)
    {
        if (reinterpret_cast<int64_t>(pStudioData->m_MDLCache) == 0xDEADFEEDDEADFEED)
            return nullptr;

        void* pAnimData = (void*)*((_QWORD*)pStudioData->m_MDLCache + 1);

        AcquireSRWLockExclusive(reinterpret_cast<PSRWLOCK>(&*m_MDLLock));
        v_CStudioHWDataRef__SetFlags(reinterpret_cast<CStudioHWDataRef*>(pAnimData), 1i64); // !!! DECLARED INLINE IN < S3 !!!
        ReleaseSRWLockExclusive(reinterpret_cast<PSRWLOCK>(&*m_MDLLock));
    }
    if ((pStudioData->m_nFlags & STUDIODATA_FLAGS_STUDIOMESH_LOADED))
        return &pStudioData->m_pHardwareRef->m_HardwareData;
    else
        return nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: gets the studio material glue from cache pool by handle
// Input  : *this - 
//          handle - 
// Output : a pointer to the CMaterialGlue object
//-----------------------------------------------------------------------------
void* CMDLCache::GetMaterialTable(CMDLCache* cache, MDLHandle_t handle)
{
    EnterCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(&*m_MDLMutex));
    studiodata_t* pStudioData = m_MDLDict->Find(handle);
    LeaveCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(&*m_MDLMutex));

    return &pStudioData->m_pMaterialTable;
}

//-----------------------------------------------------------------------------
// Purpose: gets the error model
// Output : *studiohdr_t
//-----------------------------------------------------------------------------
studiohdr_t* CMDLCache::GetErrorModel(void)
{
    old_gather_props->SetValue(true); // mdl/error.rmdl fallback is not supported (yet) in the new GatherProps solution!
    return g_pMDLFallback->m_pErrorHDR;
}

//-----------------------------------------------------------------------------
// Purpose: checks if this model handle is within the vector of bad models
// Input  : handle - 
// Output : true if exist, false otherwise
//-----------------------------------------------------------------------------
bool CMDLCache::IsKnownBadModel(MDLHandle_t handle)
{
    return std::find(g_BadMDLHandles.begin(), g_BadMDLHandles.end(), handle) != g_BadMDLHandles.end();
}

void MDLCache_Attach()
{
    DetourAttach((LPVOID*)&v_CMDLCache__FindMDL, &CMDLCache::FindMDL);
    DetourAttach((LPVOID*)&v_CMDLCache__FindCachedMDL, &CMDLCache::FindCachedMDL);
    DetourAttach((LPVOID*)&v_CMDLCache__FindUncachedMDL, &CMDLCache::FindUncachedMDL);
    DetourAttach((LPVOID*)&v_CMDLCache__GetHardwareData, &CMDLCache::GetHardwareData);
    DetourAttach((LPVOID*)&v_CMDLCache__GetStudioHDR, &CMDLCache::GetStudioHDR);
}

void MDLCache_Detach()
{
    DetourDetach((LPVOID*)&v_CMDLCache__FindMDL, &CMDLCache::FindMDL);
    DetourDetach((LPVOID*)&v_CMDLCache__FindCachedMDL, &CMDLCache::FindCachedMDL);
    DetourDetach((LPVOID*)&v_CMDLCache__FindUncachedMDL, &CMDLCache::FindUncachedMDL);
    DetourDetach((LPVOID*)&v_CMDLCache__GetHardwareData, &CMDLCache::GetHardwareData);
    DetourDetach((LPVOID*)&v_CMDLCache__GetStudioHDR, &CMDLCache::GetStudioHDR);
}