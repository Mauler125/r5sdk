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
studiohdr_t* CMDLCache::FindMDL(CMDLCache* const cache, const MDLHandle_t handle, void* a3)
{
    studiodata_t* const pStudioData = cache->GetStudioData(handle);

    if (!pStudioData)
    {
        studiohdr_t* const pStudioHdr = GetErrorModel();

        if (!IsKnownBadModel(handle))
        {
            if (!pStudioHdr)
                Error(eDLL_T::ENGINE, EXIT_FAILURE, "Model with handle \"%hu\" not found and \"%s\" couldn't be loaded.\n", handle, ERROR_MODEL);
            else
                Error(eDLL_T::ENGINE, NO_ERROR, "Model with handle \"%hu\" not found; replacing with \"%s\".\n", handle, ERROR_MODEL);
        }

        return pStudioHdr;
    }

    studiocache_t* studioCache = pStudioData->GetStudioCache();

    // Store error and empty fallback models.
    if (studioCache && studioCache != DC_INVALID_HANDLE)
    {
        studiohdr_t* const pStudioHDR = pStudioData->GetStudioCache()->GetStudioHdr();

        if (pStudioHDR)
        {
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

    const int nFlags = STUDIOHDR_FLAGS_NEEDS_DEFERRED_ADDITIVE | STUDIOHDR_FLAGS_OBSOLETE;

    if ((pStudioData->m_nFlags & nFlags))
    {
        if (studioCache)
        {
            if (a3)
            {
                FindCachedMDL(cache, pStudioData, a3);
                studioCache = pStudioData->m_pStudioCache;
            }

            studiohdr_t* const pStudioHdr = studioCache->GetStudioHdr();

            if (pStudioHdr)
                return pStudioHdr;

            return FindUncachedMDL(cache, handle, pStudioData, a3);
        }

        studioanimcache_t* const animCache = pStudioData->m_pAnimCache;

        if (animCache)
        {
            studiohdr_t* const pStudioHdr = animCache->GetStudioHdr();

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
void CMDLCache::FindCachedMDL(CMDLCache* const cache, studiodata_t* const pStudioData, void* a3)
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
studiohdr_t* CMDLCache::FindUncachedMDL(CMDLCache* const cache, const MDLHandle_t handle, studiodata_t* pStudioData, void* a4)
{
    AUTO_LOCK(pStudioData->m_Mutex);

    const char* szModelName = cache->GetModelName(handle);
    const size_t nFileNameLen = strlen(szModelName);

    studiohdr_t* pStudioHdr = nullptr;

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

    pStudioData->m_bSearchingModelName = true;
    g_pRTech->StringToGuid(szModelName);
    pStudioData->m_bSearchingModelName = false;

    if (!pStudioData->m_pStudioCache)
    {
        studioanimcache_t* const animCache = pStudioData->GetAnimCache();
        if (animCache)
        {
            pStudioHdr = animCache->GetStudioHdr();
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
        studiocache_t* const dataCache = pStudioData->GetStudioCache();

        if (dataCache)
        {
            if (dataCache == DC_INVALID_HANDLE)
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
                pStudioHdr = dataCache->GetStudioHdr();
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

    assert(pStudioHdr);
    return pStudioHdr;
}

//-----------------------------------------------------------------------------
// Purpose: gets the studio physics cache from cache pool by handle
// Input  : *this - 
//          handle - 
// Output : a pointer to the studiophysicsref_t object
//-----------------------------------------------------------------------------
studiophysicsref_t* CMDLCache::GetStudioPhysicsCache(const MDLHandle_t handle)
{
    if (handle == MDLHANDLE_INVALID)
        return nullptr;

    const studiodata_t* const studioData = GetStudioData(handle);

    if (!studioData)
    {
        Warning(eDLL_T::ENGINE, "Attempted to load studio physics ref on model \"%s\" with no studio data!\n", GetModelName(handle));
        return nullptr;
    }

    const studiocache_t* const studioCache = studioData->GetStudioCache();

    if (!studioCache || studioCache == DC_INVALID_HANDLE)
    {
        Warning(eDLL_T::ENGINE, "Attempted to load studio physics ref on model \"%s\" with invalid studio cache!\n", GetModelName(handle));
        return nullptr;
    }

    return studioCache->GetPhysicsCache();
}

//-----------------------------------------------------------------------------
// Purpose: gets the vcollide data from cache pool by handle
// Input  : *this - 
//          handle - 
// Output : a pointer to the vcollide_t object
//-----------------------------------------------------------------------------
vcollide_t* CMDLCache::GetVCollide(CMDLCache* const cache, const MDLHandle_t handle)
{
    studiophysicsref_t* const physicsCache = cache->GetStudioPhysicsCache(handle);

    if (!physicsCache)
        return nullptr;

    CStudioVCollide* const pVCollide = physicsCache->GetStudioVCollide();

    if (!pVCollide)
        return nullptr;

    return pVCollide->GetVCollide();
}

//-----------------------------------------------------------------------------
// Purpose: gets the physics geometry data from cache pool by handle
// Input  : *this - 
//          handle - 
// Output : a pointer to the physics geometry descriptor
//-----------------------------------------------------------------------------
void* CMDLCache::GetPhysicsGeometry(CMDLCache* const cache, const MDLHandle_t handle)
{
    studiophysicsref_t* const physicsCache = cache->GetStudioPhysicsCache(handle);

    if (!physicsCache)
        return nullptr;

    CStudioPhysicsGeoms* const physicsGeoms = physicsCache->GetPhysicsGeoms();

    if (!physicsGeoms)
        return nullptr;

    return physicsGeoms->GetGeometryData();
}

//-----------------------------------------------------------------------------
// Purpose: gets the studio hardware data from cache pool by handle
// Input  : *this - 
//          handle - 
// Output : a pointer to the studiohwdata_t object
//-----------------------------------------------------------------------------
studiohwdata_t* CMDLCache::GetHardwareData(CMDLCache* const cache, const MDLHandle_t handle)
{
    const studiodata_t* pStudioData = cache->GetStudioData(handle);

    if (!pStudioData)
    {
        if (!g_pMDLFallback->m_hErrorMDL)
        {
            Error(eDLL_T::ENGINE, EXIT_FAILURE, "Studio hardware with handle \"%hu\" not found and \"%s\" couldn't be loaded.\n", handle, ERROR_MODEL);
            return nullptr;
        }
        pStudioData = cache->GetStudioData(g_pMDLFallback->m_hErrorMDL);
    }

    const studiocache_t* const dataCache = pStudioData->GetStudioCache();

    if (!dataCache || dataCache == DC_INVALID_HANDLE)
        return nullptr;

    studiophysicsref_t* const physicsCache = dataCache->GetPhysicsCache();

    AcquireSRWLockExclusive(g_pMDLLock);
    CMDLCache__CheckData(physicsCache, 1i64); // !!! DECLARED INLINE IN < S3 !!!
    ReleaseSRWLockExclusive(g_pMDLLock);

    if ((pStudioData->m_nFlags & STUDIODATA_FLAGS_STUDIOMESH_LOADED))
        return pStudioData->GetHardwareDataRef()->GetHardwareData();

    return nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: gets the error model
//-----------------------------------------------------------------------------
studiohdr_t* CMDLCache::GetErrorModel(void)
{
    return g_pMDLFallback->m_pErrorHDR;
}
MDLHandle_t CMDLCache::GetErrorModelHandle(void)
{
    return g_pMDLFallback->m_hErrorMDL;
}

//-----------------------------------------------------------------------------
// Purpose: checks if this model handle is within the set of bad models
// Input  : handle - 
// Output : true if exist, false otherwise
//-----------------------------------------------------------------------------
bool CMDLCache::IsKnownBadModel(const MDLHandle_t handle)
{
    auto p = g_vBadMDLHandles.insert(handle);
    return !p.second;
}

void VMDLCache::Detour(const bool bAttach) const
{
    DetourSetup(&CMDLCache__FindMDL, &CMDLCache::FindMDL, bAttach);
    DetourSetup(&CMDLCache__FindCachedMDL, &CMDLCache::FindCachedMDL, bAttach);
    DetourSetup(&CMDLCache__FindUncachedMDL, &CMDLCache::FindUncachedMDL, bAttach);
    DetourSetup(&CMDLCache__GetHardwareData, &CMDLCache::GetHardwareData, bAttach);
    DetourSetup(&CMDLCache__GetVCollide, &CMDLCache::GetVCollide, bAttach);
    DetourSetup(&CMDLCache__GetPhysicsGeometry, &CMDLCache::GetPhysicsGeometry, bAttach);
}
