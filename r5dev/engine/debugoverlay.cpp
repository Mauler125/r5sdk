//============================================================================//
//
// Purpose: Debug interface functions
//
//============================================================================//

#include "core/stdafx.h"
#include "common/pseudodefs.h"
#include "tier0/memstd.h"
#include "tier0/basetypes.h"
#include "tier1/cvar.h"
#include "tier2/renderutils.h"
#include "engine/client/clientstate.h"
#include "engine/host_cmd.h"
#include "engine/debugoverlay.h"
#include "materialsystem/cmaterialsystem.h"
#include "mathlib/mathlib.h"
#ifndef CLIENT_DLL
#include "game/shared/ai_utility_shared.h"
#include "game/server/ai_network.h"
#endif // !CLIENT_DLL


//------------------------------------------------------------------------------
// Purpose: checks if overlay should be decayed
// Output : true to decay, false otherwise
//------------------------------------------------------------------------------
bool OverlayBase_t::IsDead() const
{
    if (r_debug_overlay_nodecay->GetBool())
    {
        // Keep rendering the overlay if no-decay is set.
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
            return m_nOverlayTick < *g_nOverlayTickCount;
        }
        return false;
    }
    else
    {
        return m_nCreationTick < *g_nRenderTickCount;
    }
    return g_pClientState->GetClientTime() >= m_flEndTime;
}

//------------------------------------------------------------------------------
// Purpose: destroys the overlay
// Input  : *pOverlay - 
//------------------------------------------------------------------------------
void DestroyOverlay(OverlayBase_t* pOverlay)
{
    size_t pOverlaySize;

    EnterCriticalSection(&*s_OverlayMutex);
    switch (pOverlay->m_Type)
    {
    case OverlayType_t::OVERLAY_BOX:
        pOverlaySize = sizeof(OverlayBox_t);
        goto LABEL_MALLOC;
    case OverlayType_t::OVERLAY_SPHERE:
        pOverlaySize = sizeof(OverlaySphere_t);
        goto LABEL_MALLOC;
    case OverlayType_t::OVERLAY_LINE:
        pOverlaySize = sizeof(OverlayLine_t);
        goto LABEL_MALLOC;
    case OverlayType_t::OVERLAY_TRIANGLE:
        pOverlaySize = 6200i64;
        goto LABEL_MALLOC;
    case OverlayType_t::OVERLAY_LASER_LINE:
        pOverlay->m_Type = OverlayType_t::OVERLAY_UNK1;
        LeaveCriticalSection(&*s_OverlayMutex);
        return;
    case OverlayType_t::OVERLAY_BOX2:
        pOverlaySize = 88i64;
        goto LABEL_MALLOC;
    case OverlayType_t::OVERLAY_CAPSULE:
        pOverlaySize = sizeof(OverlayCapsule_t);
        break;
    case OverlayType_t::OVERLAY_UNK0:
        pOverlaySize = 88i64;
        goto LABEL_MALLOC;
    LABEL_MALLOC:
        pOverlay->m_Type = OverlayType_t::OVERLAY_UNK1;
        MemAllocSingleton()->Free(pOverlay);
        break;
    default:
        break;
    }

    LeaveCriticalSection(&*s_OverlayMutex);
}

//------------------------------------------------------------------------------
// Purpose: draws a generic overlay
// Input  : *pOverlay - 
//------------------------------------------------------------------------------
void DrawOverlay(OverlayBase_t* pOverlay)
{
    EnterCriticalSection(&*s_OverlayMutex);

    switch (pOverlay->m_Type)
    {
    case OverlayType_t::OVERLAY_BOX:
    {
        OverlayBox_t* pBox = static_cast<OverlayBox_t*>(pOverlay);
        if (pBox->a < 1)
        {
            if (r_debug_overlay_invisible->GetBool())
            {
                pBox->a = 255;
            }
            else
            {
                LeaveCriticalSection(&*s_OverlayMutex);
                return;
            }
        }

        v_RenderBox(pBox->transforms, pBox->mins, pBox->maxs, Color(pBox->r, pBox->g, pBox->b, pBox->a), r_debug_overlay_zbuffer->GetBool());
        break;
    }
    case OverlayType_t::OVERLAY_SPHERE:
    {
        OverlaySphere_t* pSphere = static_cast<OverlaySphere_t*>(pOverlay);
        if (pSphere->a < 1)
        {
            if (r_debug_overlay_invisible->GetBool())
            {
                pSphere->a = 255;
            }
            else
            {
                LeaveCriticalSection(&*s_OverlayMutex);
                return;
            }
        }

        if (r_debug_overlay_wireframe->GetBool())
        {
            v_RenderWireframeSphere(pSphere->vOrigin, pSphere->flRadius, pSphere->nTheta, pSphere->nPhi, 
                Color(pSphere->r, pSphere->g, pSphere->b, pSphere->a), r_debug_overlay_zbuffer->GetBool());
        }
        else
        {
            DebugDrawSphere(pSphere->vOrigin, pSphere->flRadius, Color(pSphere->r, pSphere->g, pSphere->b, pSphere->a), 16, r_debug_overlay_zbuffer->GetBool());
        }
        break;
    }
    case OverlayType_t::OVERLAY_LINE:
    {
        OverlayLine_t* pLine = static_cast<OverlayLine_t*>(pOverlay);
        if (pLine->a < 1)
        {
            if (r_debug_overlay_invisible->GetBool())
            {
                pLine->a = 255;
            }
            else
            {
                LeaveCriticalSection(&*s_OverlayMutex);
                return;
            }
        }

        v_RenderLine(pLine->origin, pLine->dest, Color(pLine->r, pLine->g, pLine->b, pLine->a), !pLine->noDepthTest);
        break;
    }
    case OverlayType_t::OVERLAY_TRIANGLE:
    {
        //printf("TRIANGLE %p\n", pOverlay);
        break;
    }
    case OverlayType_t::OVERLAY_LASER_LINE:
    {
        OverlayLaserLine_t* pLaser = static_cast<OverlayLaserLine_t*>(pOverlay);
        v_RenderLine(pLaser->start, pLaser->end, Color(pLaser->r, pLaser->g, pLaser->b, pLaser->a), !pLaser->noDepthTest);
        break;
    }
    case OverlayType_t::OVERLAY_BOX2:
    {
        //printf("BOX2 %p\n", pOverlay);
        break;
    }
    case OverlayType_t::OVERLAY_CAPSULE:
    {
        OverlayCapsule_t* pCapsule = static_cast<OverlayCapsule_t*>(pOverlay);
        if (pCapsule->a < 1)
        {
            if (r_debug_overlay_invisible->GetBool())
            {
                pCapsule->a = 255;
            }
            else
            {
                LeaveCriticalSection(&*s_OverlayMutex);
                return;
            }
        }

        QAngle angles;

        VectorAngles(pCapsule->end, pCapsule->start, angles);
        AngleInverse(angles, angles);

        DebugDrawCapsule(pCapsule->start, angles, pCapsule->radius, pCapsule->start.DistTo(pCapsule->end), 
            Color(pCapsule->r, pCapsule->g, pCapsule->b, pCapsule->a), r_debug_overlay_zbuffer->GetBool());
        break;
    }
    case OverlayType_t::OVERLAY_UNK0:
    {
        //printf("UNK0 %p\n", pOverlay);
        break;
    }
    case OverlayType_t::OVERLAY_UNK1:
    {
        //printf("UNK1 %p\n", pOverlay);
        break;
    }
    }

    LeaveCriticalSection(&*s_OverlayMutex);
}

//------------------------------------------------------------------------------
// Purpose : overlay drawing entrypoint
// Input  : bDraw - 
//------------------------------------------------------------------------------
void DrawAllOverlays(bool bDraw)
{
    EnterCriticalSection(&*s_OverlayMutex);

    const bool bOverlayEnabled = enable_debug_overlays->GetBool();
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
                if (pCurrOverlay->m_nOverlayTick == *g_nOverlayTickCount)
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
                bDraw = pCurrOverlay->m_nCreationTick == *g_nRenderTickCount;
            }
            if (bOverlayEnabled && bDraw)
            {
                if (bDraw)
                {
                    DrawOverlay(pCurrOverlay);
                }
            }
            pPrevOverlay = pCurrOverlay;
            pCurrOverlay = pCurrOverlay->m_pNextOverlay;
        }
    }

#ifndef CLIENT_DLL
    if (bOverlayEnabled)
    {
        if (ai_script_nodes_draw->GetInt() > -1)
            g_pAIUtility->DrawAIScriptNetwork(*g_pAINetwork);
        if (navmesh_draw_bvtree->GetInt() > -1)
            g_pAIUtility->DrawNavMeshBVTree();
        if (navmesh_draw_portal->GetInt() > -1)
            g_pAIUtility->DrawNavMeshPortals();
        if (navmesh_draw_polys->GetInt() > -1)
            g_pAIUtility->DrawNavMeshPolys();
        if (navmesh_draw_poly_bounds->GetInt() > -1)
            g_pAIUtility->DrawNavMeshPolyBoundaries();
    }
#endif // !CLIENT_DLL

    LeaveCriticalSection(&*s_OverlayMutex);
}

///////////////////////////////////////////////////////////////////////////////
void DebugOverlays_Attach()
{
    DetourAttach(&v_DrawAllOverlays, &DrawAllOverlays);
}

void DebugOverlays_Detach()
{
    DetourDetach(&v_DrawAllOverlays, &DrawAllOverlays);
}
