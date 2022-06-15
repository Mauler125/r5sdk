//============================================================================//
//
// Purpose: Debug interface functions
//
//============================================================================//

#include "core/stdafx.h"
#include "tier0/tslist.h"
#include "tier0/basetypes.h"
#include "tier1/cvar.h"
#include "common/pseudodefs.h"
#include "engine/client/clientstate.h"
#include "engine/host_cmd.h"
#include "engine/debugoverlay.h"
#include "materialsystem/cmaterialsystem.h"


//------------------------------------------------------------------------------
// Purpose: checks if overlay should be decayed
//------------------------------------------------------------------------------
bool OverlayBase_t::IsDead() const
{
    if (r_debug_overlay_nodecay->GetBool())
    {
        // Keep rendering the overlay if nodecay is set.
        return false;
    }

    if (g_pClientState->IsPaused())
    {
        // Keep rendering the overlay if the client simulation is paused.
        return false;
    }

    if (m_nCreationTick == -1)
    {
        if (m_nOverlayTick == -1)
        {
            if (m_flEndTime == NDEBUG_PERSIST_TILL_NEXT_SERVER)
            {
                return false;
            }
            return g_pClientState->GetClientTime() >= m_flEndTime;
        }
        else
        {
            return m_nOverlayTick < *overlay_tickcount;
        }
        return false;
    }
    else
    {
        return m_nCreationTick < *render_tickcount;
    }
    return g_pClientState->GetClientTime() >= m_flEndTime;
}

//------------------------------------------------------------------------------
// Purpose: destroys the overlay
//------------------------------------------------------------------------------
void DestroyOverlay(OverlayBase_t* pOverlay)
{
    size_t pOverlaySize;

    EnterCriticalSection(&*s_OverlayMutex);
    switch (pOverlay->m_Type)
    {
    case OverlayType_t::OVERLAY_BOX:
        pOverlaySize = 128i64;
        goto LABEL_MALLOC;
    case OverlayType_t::OVERLAY_SPHERE:
        pOverlaySize = 72i64;
        goto LABEL_MALLOC;
    case OverlayType_t::OVERLAY_LINE:
        pOverlaySize = 80i64;
        goto LABEL_MALLOC;
    case OverlayType_t::OVERLAY_TRIANGLE:
        pOverlaySize = 6200i64;
        goto LABEL_MALLOC;
    case OverlayType_t::OVERLAY_SWEPT_BOX:
        pOverlay->m_Type = OverlayType_t::OVERLAY_UNK1;
        LeaveCriticalSection(&*s_OverlayMutex);
        return;
    case OverlayType_t::OVERLAY_BOX2:
        break;
    case OverlayType_t::OVERLAY_CAPSULE:
        pOverlaySize = 112i64;
        break;
    case OverlayType_t::OVERLAY_UNK0:
        pOverlaySize = 88i64;
        goto LABEL_MALLOC;
    LABEL_MALLOC:
        pOverlay->m_Type = OverlayType_t::OVERLAY_UNK1;
        v_MemAlloc_Internal(pOverlay, pOverlaySize);
        break;
    default:
        break;
    }

    LeaveCriticalSection(&*s_OverlayMutex);
}

//------------------------------------------------------------------------------
// Purpose: draws a generic overlay
//------------------------------------------------------------------------------
void DrawOverlay(OverlayBase_t* pOverlay)
{
    //void* pRenderContext = nullptr; // ptr to CMatQueuedRenderContext vtable.

    EnterCriticalSection(&*s_OverlayMutex);

    //pRenderContext = (*(void*(__fastcall**)(void*))(**(std::int64_t**)g_pMaterialSystem + MATERIALSYSTEM_VCALL_OFF_0))((void*)g_pMaterialSystem);
    //(*(void(__fastcall**)(void*, std::int64_t))(*(std::int64_t*)pRenderContext + CMATQUEUEDRENDERCONTEXT_VCALL_OFS_0))((void*)pRenderContext, 0x1);

    switch (pOverlay->m_Type)
    {
    case OverlayType_t::OVERLAY_BOX:
    {
        OverlayBox_t* pBox = static_cast<OverlayBox_t*>(pOverlay); // TODO: debug this since it doesn't work but does compute something..

        // for testing, since RenderWireframeBox doesn't seem to work properly
        //RenderWireframeSphere({ pBox->origin_X, pBox->origin_Y, pBox->origin_Z }, pBox->maxs.x, 8, 8, Color(pBox->r, pBox->g, pBox->b, 255), false);
        //RenderLine({ pBox->origin_X, pBox->origin_Y, pBox->origin_Z }, { pBox->origin_X, pBox->origin_Y, pBox->origin_Z+pBox->maxs.z }, Color(pBox->r, pBox->g, pBox->b, 255), false);

        //if (pBox->a < 255)
        //{
        //    RenderWireframeBox({ pBox->origin_X, pBox->origin_Y, pBox->origin_Z }, pBox->angles, pBox->maxs, Color(pBox->r, pBox->g, pBox->b, 255), false);
        //}
        //else {
        //    RenderBox({ pBox->origin_X, pBox->origin_Y, pBox->origin_Z }, pBox->angles, pBox->mins, pBox->maxs, Color(pBox->r, pBox->g, pBox->b, pBox->a), false);
        //}
        break;
    }
    case OverlayType_t::OVERLAY_SPHERE:
    {
        OverlaySphere_t* pSphere = static_cast<OverlaySphere_t*>(pOverlay);
        v_RenderWireframeSphere(pSphere->vOrigin, pSphere->flRadius, pSphere->nTheta, pSphere->nPhi, Color(pSphere->r, pSphere->g, pSphere->b, pSphere->a), false);
        break;
    }
    case OverlayType_t::OVERLAY_LINE:
    {
        OverlayLine_t* pLine = static_cast<OverlayLine_t*>(pOverlay);
        v_RenderLine(pLine->origin, pLine->dest, Color(pLine->r, pLine->g, pLine->b, pLine->a), pLine->noDepthTest);
        break;
    }
    case OverlayType_t::OVERLAY_TRIANGLE:
    {
        break;
    }
    case OverlayType_t::OVERLAY_SWEPT_BOX:
    {
        break;
    }
    case OverlayType_t::OVERLAY_BOX2:
    {
        break;
    }
    case OverlayType_t::OVERLAY_CAPSULE:
    {
        break;
    }
    case OverlayType_t::OVERLAY_UNK0:
    {
        break;
    }
    case OverlayType_t::OVERLAY_UNK1:
    {
        break;
    }
    }
    //(*(void(__fastcall**)(void*))(*(_QWORD*)pRenderContext + CMATQUEUEDRENDERCONTEXT_VCALL_OFS_1))(pRenderContext);
    //(*(void(__fastcall**)(void*))(*(_QWORD*)pRenderContext + CMATQUEUEDRENDERCONTEXT_VCALL_OFS_2))(pRenderContext);

    LeaveCriticalSection(&*s_OverlayMutex);
}

//------------------------------------------------------------------------------
// Purpose : overlay drawing entrypoint
//------------------------------------------------------------------------------
void DrawAllOverlays(char pOverlay)
{
    if (!enable_debug_overlays->GetBool())
    {
        return;
    }
    EnterCriticalSection(&*s_OverlayMutex);

    OverlayBase_t* pCurrOverlay = *s_pOverlays; // rdi
    OverlayBase_t* pPrevOverlay = nullptr;      // rsi
    OverlayBase_t* pNextOverlay = nullptr;      // rbx

    while (pCurrOverlay)
    {
        if (pCurrOverlay->IsDead())
        {
            if (pPrevOverlay)
            {
                // If I had a last overlay reset it's next pointer
                pPrevOverlay->m_pNextOverlay = pCurrOverlay->m_pNextOverlay;
            }
            else
            {
                // If the first line, reset the s_pOverlays pointer
                *s_pOverlays = pCurrOverlay->m_pNextOverlay;
            }

            pNextOverlay = pCurrOverlay->m_pNextOverlay;
            DestroyOverlay(pCurrOverlay);
            pCurrOverlay = pNextOverlay;
        }
        else
        {
            bool bDraw{ };
            if (pCurrOverlay->m_nCreationTick == -1)
            {
                if (pCurrOverlay->m_nOverlayTick == *overlay_tickcount)
                {
                    // Draw overlay if unk0 == *overlay_tickcount
                    bDraw = true;
                }
                if (pCurrOverlay->m_nOverlayTick == -1)
                {
                    // Draw overlay if unk0 == -1
                    bDraw = true;
                }
            }
            else
            {
                bDraw = pCurrOverlay->m_nCreationTick == *render_tickcount;
            }
            if (bDraw)
            {
                if (pOverlay)
                {
                    DrawOverlay(pCurrOverlay);
                }
            }
            pPrevOverlay = pCurrOverlay;
            pCurrOverlay = pCurrOverlay->m_pNextOverlay;
        }
    }
    LeaveCriticalSection(&*s_OverlayMutex);
}

///////////////////////////////////////////////////////////////////////////////
void DebugOverlays_Attach()
{
//#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
//    p_DrawAllOverlays.Offset(0x189).Patch({ 0x83, 0x3F, 0x02 });  // Default value in memory is 0x2, condition is 0x4. Patch to fullfill condition.
//#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
//    p_DrawAllOverlays.Offset(0x188).Patch({ 0x83, 0x3F, 0x02 });  // Default value in memory is 0x2, condition is 0x4. Patch to fullfill condition.
//#endif
    DetourAttach((LPVOID*)&v_DrawAllOverlays, &DrawAllOverlays);
}

void DebugOverlays_Detach()
{
    DetourDetach((LPVOID*)&v_DrawAllOverlays, &DrawAllOverlays);
}
