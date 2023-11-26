//=====================================================================================//
//
// model loading and caching
//
// $NoKeywords: $
//=====================================================================================//

#include "core/stdafx.h"
#include "tier0/threadtools.h"
#include "tier1/cvar.h"
#include "tier1/utldict.h"
#include "datacache/mdlcache.h"
#include "datacache/imdlcache.h"
#include "datacache/idatacache.h"
#include "rtech/rtech_utils.h"
#include "public/studio.h"

RMDLFallBack_t* g_pMDLFallback = new RMDLFallBack_t();
std::unordered_set<MDLHandle_t> g_vBadMDLHandles;


//-----------------------------------------------------------------------------
// Purpose: finds an MDL
// Input  : *this - 
//          handle - 
//          *a3 - 
// Output : a pointer to the studiohdr_t object
//-----------------------------------------------------------------------------
studiohdr_t* CMDLCache::FindMDL(CMDLCache* cache, MDLHandle_t handle, void* a3)
{
    studiodata_t* pStudioData = cache->GetStudioData(handle);
    studiohdr_t* pStudioHdr;

    if (pStudioData)
    {
        if (pStudioData->m_MDLCache &&
            pStudioData->m_MDLCache != DC_INVALID_HANDLE)
        {
            studiohdr_t* pStudioHDR = **reinterpret_cast<studiohdr_t***>(pStudioData);

            if (!g_pMDLFallback->m_hErrorMDL && V_ComparePath(pStudioHDR->name, ERROR_MODEL))
            {
                g_pMDLFallback->m_pErrorHDR = pStudioHDR;
                g_pMDLFallback->m_hErrorMDL = handle;
            }
            else if (!g_pMDLFallback->m_hEmptyMDL && V_ComparePath(pStudioHDR->name, EMPTY_MODEL))
            {
                g_pMDLFallback->m_pEmptyHDR = pStudioHDR;
                g_pMDLFallback->m_hEmptyMDL = handle;
            }
        }
    }
    else
    {
        pStudioHdr = GetErrorModel();

        if (!IsKnownBadModel(handle))
        {
            if (!pStudioHdr)
                Error(eDLL_T::ENGINE, EXIT_FAILURE, "Model with handle \"%hu\" not found and \"%s\" couldn't be loaded.\n", handle, ERROR_MODEL);
            else
                Error(eDLL_T::ENGINE, NO_ERROR, "Model with handle \"%hu\" not found; replacing with \"%s\".\n", handle, ERROR_MODEL);
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

            pStudioHdr = *reinterpret_cast<studiohdr_t**>(pMDLCache);
            if (pStudioHdr)
                return pStudioHdr;

            return FindUncachedMDL(cache, handle, pStudioData, a3);
        }
        pMDLCache = pStudioData->m_pAnimData;
        if (pMDLCache)
        {
            pStudioHdr = *reinterpret_cast<studiohdr_t**>(pMDLCache);
            if (pStudioHdr)
                return pStudioHdr;
        }
    }
    return FindUncachedMDL(cache, handle, pStudioData, a3);
}

//-----------------------------------------------------------------------------
// Purpose: finds an MDL cached
// Input  : *this - 
//          *pStudioData - 
//          *a3 - 
//-----------------------------------------------------------------------------
void CMDLCache::FindCachedMDL(CMDLCache* cache, studiodata_t* pStudioData, void* a3)
{
    if (a3)
    {
        AUTO_LOCK(pStudioData->m_Mutex);

        *(_QWORD*)((int64_t)a3 + 0x880) = *(_QWORD*)&pStudioData->pad[0x24];
        int64_t v6 = *(_QWORD*)&pStudioData->pad[0x24];
        if (v6)
            *(_QWORD*)(v6 + 0x878) = (int64_t)a3;
        *(_QWORD*)&pStudioData->pad[0x24] = (int64_t)a3;
        *(_QWORD*)((int64_t)a3 + 0x870) = (int64_t)cache;
        *(_WORD*)((int64_t)a3 + 0x888) = pStudioData->m_Handle;
    }
}

//-----------------------------------------------------------------------------
// Purpose: finds an MDL uncached
// Input  : *this - 
//          handle - 
//          *pStudioData - 
//          *a4 - 
// Output : a pointer to the studiohdr_t object
//-----------------------------------------------------------------------------
studiohdr_t* CMDLCache::FindUncachedMDL(CMDLCache* cache, MDLHandle_t handle, studiodata_t* pStudioData, void* a4)
{
    AUTO_LOCK(pStudioData->m_Mutex);

    const char* szModelName = cache->GetModelName(handle);
    size_t nFileNameLen = strlen(szModelName);

    studiohdr_t* pStudioHdr;

    if (nFileNameLen < 5 ||
        (Q_stricmp(&szModelName[nFileNameLen - 5], ".rmdl") != 0) &&
        (Q_stricmp(&szModelName[nFileNameLen - 5], ".rrig") != 0) &&
        (Q_stricmp(&szModelName[nFileNameLen - 5], ".rpak") != 0))
    {
        pStudioHdr = GetErrorModel();
        if (!IsKnownBadModel(handle))
        {
            if (!pStudioHdr)
                Error(eDLL_T::ENGINE, EXIT_FAILURE, "Attempted to load old model \"%s\" and \"%s\" couldn't be loaded.\n", szModelName, ERROR_MODEL);
            else
                Error(eDLL_T::ENGINE, NO_ERROR, "Attempted to load old model \"%s\"; replacing with \"%s\".\n", szModelName, ERROR_MODEL);
        }

        return pStudioHdr;
    }

    LOBYTE(pStudioData->m_nGuidLock) = 1;
    g_pRTech->StringToGuid(szModelName);
    LOBYTE(pStudioData->m_nGuidLock) = 0;

    if (!pStudioData->m_MDLCache)
    {
        studiohdr_t**  pAnimData = (studiohdr_t**)pStudioData->m_pAnimData;
        if (pAnimData)
        {
            pStudioHdr = *pAnimData;
        }
        else
        {
            pStudioHdr = GetErrorModel();
            if (!IsKnownBadModel(handle))
            {
                if (!pStudioHdr)
                    Error(eDLL_T::ENGINE, EXIT_FAILURE, "Model \"%s\" not found and \"%s\" couldn't be loaded.\n", szModelName, ERROR_MODEL);
                else
                    Error(eDLL_T::ENGINE, NO_ERROR, "Model \"%s\" not found; replacing with \"%s\".\n", szModelName, ERROR_MODEL);
            }

            return pStudioHdr;
        }
    }
    else
    {
        FindCachedMDL(cache, pStudioData, a4);
        DataCacheHandle_t dataHandle = pStudioData->m_MDLCache;

        if (dataHandle)
        {
            if (dataHandle == DC_INVALID_HANDLE)
            {
                pStudioHdr = GetErrorModel();
                if (!IsKnownBadModel(handle))
                {
                    if (!pStudioHdr)
                        Error(eDLL_T::ENGINE, EXIT_FAILURE, "Model \"%s\" has bad studio data and \"%s\" couldn't be loaded.\n", szModelName, ERROR_MODEL);
                    else
                        Error(eDLL_T::ENGINE, NO_ERROR, "Model \"%s\" has bad studio data; replacing with \"%s\".\n", szModelName, ERROR_MODEL);
                }
            }
            else
                pStudioHdr = *(studiohdr_t**)dataHandle;
        }
        else
        {
            pStudioHdr = GetErrorModel();
            if (!IsKnownBadModel(handle))
            {
                if (!pStudioHdr)
                    Error(eDLL_T::ENGINE, EXIT_FAILURE, "Model \"%s\" has no studio data and \"%s\" couldn't be loaded.\n", szModelName, ERROR_MODEL);
                else
                    Error(eDLL_T::ENGINE, NO_ERROR, "Model \"%s\" has no studio data; replacing with \"%s\".\n", szModelName, ERROR_MODEL);
            }
        }
    }

    return pStudioHdr;
}

//-----------------------------------------------------------------------------
// Purpose: gets the studiohdr from cache pool by handle
// Input  : *this - 
//          handle - 
// Output : a pointer to the studiohdr_t object
//-----------------------------------------------------------------------------
studiohdr_t* CMDLCache::GetStudioHDR(CMDLCache* cache, MDLHandle_t handle)
{
    studiohdr_t* pStudioHdr = nullptr; // rax

    if (!handle)
    {
        pStudioHdr = GetErrorModel();
        if (!pStudioHdr)
            Error(eDLL_T::ENGINE, EXIT_FAILURE, "Attempted to load model with no handle and \"%s\" couldn't be loaded.\n", ERROR_MODEL);

        return pStudioHdr;
    }

    studiodata_t* pStudioData = cache->GetStudioData(handle);
    DataCacheHandle_t dataCache = pStudioData->m_MDLCache;

    if (dataCache &&
        dataCache != DC_INVALID_HANDLE)
    {
        void* v4 = *(void**)(*((_QWORD*)dataCache + 1) + 24i64);
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
    studiodata_t* pStudioData = cache->GetStudioData(handle);

    if (!pStudioData)
    {
        if (!g_pMDLFallback->m_hErrorMDL)
        {
            Error(eDLL_T::ENGINE, EXIT_FAILURE, "Studio hardware with handle \"%hu\" not found and \"%s\" couldn't be loaded.\n", handle, ERROR_MODEL);
            return nullptr;
        }
        pStudioData = cache->GetStudioData(g_pMDLFallback->m_hErrorMDL);
    }

    DataCacheHandle_t dataCache = pStudioData->m_MDLCache;

    if (dataCache)
    {
        if (dataCache == DC_INVALID_HANDLE)
            return nullptr;

        void* pAnimData = (void*)*((_QWORD*)dataCache + 1);

        AcquireSRWLockExclusive(g_pMDLLock);
#if !defined (GAMEDLL_S0) && !defined (GAMEDLL_S1) && !defined (GAMEDLL_S2)
        v_CStudioHWDataRef__SetFlags(reinterpret_cast<CStudioHWDataRef*>(pAnimData), 1i64); // !!! DECLARED INLINE IN < S3 !!!
#endif
        ReleaseSRWLockExclusive(g_pMDLLock);
    }
    if ((pStudioData->m_nFlags & STUDIODATA_FLAGS_STUDIOMESH_LOADED))
        return &pStudioData->m_pHardwareRef->m_HardwareData;
    else
        return nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: gets the error model
// Output : *studiohdr_t
//-----------------------------------------------------------------------------
studiohdr_t* CMDLCache::GetErrorModel(void)
{
    // !TODO [AMOS]: mdl/error.rmdl fallback is not supported (yet) in the new GatherProps solution!
    if (!old_gather_props->GetBool())
        old_gather_props->SetValue(true);

    return g_pMDLFallback->m_pErrorHDR;
}

//-----------------------------------------------------------------------------
// Purpose: checks if this model handle is within the set of bad models
// Input  : handle - 
// Output : true if exist, false otherwise
//-----------------------------------------------------------------------------
bool CMDLCache::IsKnownBadModel(MDLHandle_t handle)
{
    auto p = g_vBadMDLHandles.insert(handle);
    return !p.second;
}

void VMDLCache::Detour(const bool bAttach) const
{
    DetourSetup(&v_CMDLCache__FindMDL, &CMDLCache::FindMDL, bAttach);
#ifdef GAMEDLL_S3 // !!! DECLARED INLINE WITH FINDMDL IN < S3 !!!
    DetourSetup(&v_CMDLCache__FindCachedMDL, &CMDLCache::FindCachedMDL, bAttach);
    DetourSetup(&v_CMDLCache__FindUncachedMDL, &CMDLCache::FindUncachedMDL, bAttach);
#endif // GAMEDLL_S3
#ifdef GAMEDLL_S3 // !TODO:
    DetourSetup(&v_CMDLCache__GetHardwareData, &CMDLCache::GetHardwareData, bAttach);
    DetourSetup(&v_CMDLCache__GetStudioHDR, &CMDLCache::GetStudioHDR, bAttach);
#endif
}