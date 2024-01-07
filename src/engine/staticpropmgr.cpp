#include "core/stdafx.h"
#include "datacache/mdlcache.h"
#include "engine/staticpropmgr.h"

//-----------------------------------------------------------------------------
// Purpose: initialises static props from the static prop gamelump
//-----------------------------------------------------------------------------
void* CStaticProp::Init(CStaticProp* thisptr, int64_t a2, unsigned int idx, unsigned int a4, StaticPropLump_t* lump, int64_t a6, int64_t a7)
{
    MDLHandle_t handle = *reinterpret_cast<uint16_t*>(a7 + 0x140);
    studiohdr_t* pStudioHdr = g_pMDLCache->FindMDL(g_pMDLCache, handle, nullptr);

    if (lump->m_Skin >= pStudioHdr->numskinfamilies)
    {
        Error(eDLL_T::ENGINE, NO_ERROR,
            "Invalid skin index for static prop #%i with model '%s' (got %i, max %i)\n",
            idx, pStudioHdr->name, lump->m_Skin, pStudioHdr->numskinfamilies-1);

        lump->m_Skin = 0;
    }

    return CStaticProp__Init(thisptr, a2, idx, a4, lump, a6, a7);
}

//-----------------------------------------------------------------------------
// NOTE: the following gather props functions have been hooked as we must
// enable the old gather props logic for fall back models to draw !!! The
// new solution won't call CMDLCache::GetHardwareData() on bad model handles.
//-----------------------------------------------------------------------------
void* GatherStaticPropsSecondPass_PreInit(GatherProps_t* gather)
{
    if (g_StudioMdlFallbackHandler.HasInvalidModelHandles())
        g_StudioMdlFallbackHandler.EnableLegacyGatherProps();

    return v_GatherStaticPropsSecondPass_PreInit(gather);
}
void* GatherStaticPropsSecondPass_PostInit(GatherProps_t* gather)
{
    if (g_StudioMdlFallbackHandler.HasInvalidModelHandles())
        g_StudioMdlFallbackHandler.EnableLegacyGatherProps();

    return v_GatherStaticPropsSecondPass_PostInit(gather);
}

void VStaticPropMgr::Detour(const bool bAttach) const
{
#ifndef DEDICATED
    DetourSetup(&CStaticProp__Init, &CStaticProp::Init, bAttach);
#endif // !DEDICATED

    DetourSetup(&v_GatherStaticPropsSecondPass_PreInit, &GatherStaticPropsSecondPass_PreInit, bAttach);
    DetourSetup(&v_GatherStaticPropsSecondPass_PostInit, &GatherStaticPropsSecondPass_PostInit, bAttach);
}
