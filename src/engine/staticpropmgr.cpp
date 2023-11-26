#include "core/stdafx.h"
#include "datacache/mdlcache.h"
#include "engine/staticpropmgr.h"

//-----------------------------------------------------------------------------
// Purpose: initialises static props from the static prop gamelump
//-----------------------------------------------------------------------------
void* __fastcall CStaticProp_Init(int64_t thisptr, int64_t a2, unsigned int idx, unsigned int a4, StaticPropLump_t* lump, int64_t a6, int64_t a7)
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

    return v_CStaticProp_Init(thisptr, a2, idx, a4, lump, a6, a7);
}

void VStaticPropMgr::Detour(const bool bAttach) const
{
#ifndef DEDICATED
    DetourSetup(&v_CStaticProp_Init, &CStaticProp_Init, bAttach);
#endif // !DEDICATED
}
