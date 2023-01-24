//=============================================================================//
//
// Purpose: Shared AI utility.
//
//=============================================================================//
// ai_utility_shared.cpp: requires server.dll and client.dll!
//
/////////////////////////////////////////////////////////////////////////////////

#include "core/stdafx.h"
#include "tier1/cvar.h"
#include "mathlib/color.h"
#include "mathlib/vector.h"
#include "engine/debugoverlay.h"
#include "game/shared/ai_utility_shared.h"
#include "game/server/ai_utility.h"
#include "game/server/ai_networkmanager.h"
#include "game/server/ai_network.h"
#include "game/client/viewrender.h"
#include "thirdparty/recast/detour/include/detourcommon.h"
#include "thirdparty/recast/detour/include/detournavmesh.h"

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
CAI_Utility::CAI_Utility(void)
    : m_BoxColor(0, 255, 0, 255)
    , m_LinkColor(255, 0, 0, 255)
{
}

//------------------------------------------------------------------------------
// Purpose: draw AI script network
// Input  : *pAINetwork - 
//------------------------------------------------------------------------------
void CAI_Utility::DrawAIScriptNetwork(const CAI_Network* pNetwork) const
{
    if (!pNetwork)
        return; // AI Network not build or loaded.

    const bool bUseDepthBuffer = r_debug_draw_depth_test->GetBool();
    const bool bDrawNearest = ai_script_nodes_draw_nearest->GetBool();
    const int  nNodeRange = ai_script_nodes_draw_range->GetInt();
    const float flCameraRange = navmesh_debug_camera_range->GetFloat();
    const Vector3D vCamera = MainViewOrigin();

    static const __m128 xMins = _mm_setzero_ps();
    static const __m128 xMaxs = _mm_setr_ps(50.0f, 50.0f, 50.0f, 0.0f);
    static const __m128 xSubMask = _mm_setr_ps(25.0f, 25.0f, 25.0f, 0.0f);

    OverlayBox_t::Transforms vTransforms;
    std::unordered_set<int64_t> uLinkSet;

    for (int i = ai_script_nodes_draw->GetInt(), ns = pNetwork->GetNumScriptNodes(); i < ns; i++)
    {
        if (nNodeRange && i > nNodeRange)
            break;

        const CAI_ScriptNode* pScriptNode = &pNetwork->m_ScriptNode[i];

        __m128 xOrigin = _mm_setr_ps(pScriptNode->m_vOrigin.x, pScriptNode->m_vOrigin.y, pScriptNode->m_vOrigin.z, vCamera.z);
        xOrigin = _mm_sub_ps(xOrigin, xSubMask); // Subtract 25.f from our scalars to align box with node.

        if (flCameraRange > 0.0f)
        {
            // Flip the script node Z axis with that of the camera, so that it won't be used for
            // the final distance computation. This allows for viewing the AI Network from above.
            const __m128 xOriginCamZ = _mm_shuffle_ps(xOrigin, xOrigin, _MM_SHUFFLE(2, 3, 1, 0));

            if (vCamera.DistTo(*reinterpret_cast<const Vector3D*>(&xOriginCamZ)) > flCameraRange)
                continue; // Do not render if node is not within range set by cvar 'navmesh_debug_camera_range'.
        }

        // Construct box transforms.
        vTransforms.xmm[0] = _mm_set_ps(xOrigin.m128_f32[0], 0.0f, 0.0f, 1.0f);
        vTransforms.xmm[1] = _mm_set_ps(xOrigin.m128_f32[1], 0.0f, 1.0f, 0.0f);
        vTransforms.xmm[2] = _mm_set_ps(xOrigin.m128_f32[2], 1.0f, 0.0f, 0.0f);

        v_RenderBox(vTransforms.mat, *reinterpret_cast<const Vector3D*>(&xMins),
            *reinterpret_cast<const Vector3D*>(&xMaxs), m_BoxColor, bUseDepthBuffer);

        if (bDrawNearest) // Render links to the nearest node.
        {
            int nNearest = GetNearestNodeToPos(pNetwork, &pScriptNode->m_vOrigin);
            if (nNearest != NO_NODE) // NO_NODE = -1
            {
                auto p = uLinkSet.insert(PackNodeLink(i, nNearest).m128i_i64[1]);
                if (p.second) // Only render if link hasn't already been rendered.
                {
                    const CAI_ScriptNode* pNearestNode = &pNetwork->m_ScriptNode[nNearest];
                    v_RenderLine(pScriptNode->m_vOrigin, pNearestNode->m_vOrigin, m_LinkColor, bUseDepthBuffer);
                }
            }
        }
        else if (i > 0) // Render links in the order the AI Network was build.
            v_RenderLine((pScriptNode - 1)->m_vOrigin, pScriptNode->m_vOrigin, m_LinkColor, bUseDepthBuffer);
    }
}

//------------------------------------------------------------------------------
// Purpose: draw NavMesh BVTree
// Input  : *pMesh - 
//------------------------------------------------------------------------------
void CAI_Utility::DrawNavMeshBVTree(dtNavMesh* pMesh) const
{
    if (!pMesh)
        pMesh = GetNavMeshForHull(navmesh_debug_type->GetInt());
    if (!pMesh)
        return; // NavMesh for hull not loaded.

    const Vector3D vCamera = MainViewOrigin();
    const bool bDepthBuffer = r_debug_draw_depth_test->GetBool();
    const int nTileRange = navmesh_debug_tile_range->GetInt();
    const float flCameraRange = navmesh_debug_camera_range->GetFloat();

    OverlayBox_t::Transforms vTransforms;
    for (int i = navmesh_draw_bvtree->GetInt(), nt = pMesh->getTileCount(); i < nt; ++i)
    {
        if (nTileRange > 0 && i > nTileRange)
            break;

        const dtMeshTile* pTile = &pMesh->m_tiles[i];
        if (!pTile->header)
            continue;

        if (flCameraRange > 0.0f)
        {
            const __m128 xMinBound = _mm_setr_ps(pTile->header->bmin[0], pTile->header->bmin[1], vCamera.z, 0.0f);
            const __m128 xMaxBound = _mm_setr_ps(pTile->header->bmax[0], pTile->header->bmax[1], vCamera.z, 0.0f);

            if (vCamera.DistTo(*reinterpret_cast<const Vector3D*>(&xMinBound)) > flCameraRange ||
                vCamera.DistTo(*reinterpret_cast<const Vector3D*>(&xMaxBound)) > flCameraRange)
                continue;
        }

        const float flCellSize = 1.0f / pTile->header->bvQuantFactor;

        const __m128 xTileAABB = _mm_setr_ps(pTile->header->bmin[0], pTile->header->bmin[1], pTile->header->bmin[2], 0.0f);
        const __m128 xCellSize = _mm_setr_ps(flCellSize, flCellSize, flCellSize, 0.0f);

        for (int j = 0, nc = pTile->header->bvNodeCount; j < nc; ++j)
        {
            const dtBVNode* pNode = &pTile->bvTree[j];
            if (pNode->i < 0) // Leaf indices are positive.
                continue;

            vTransforms.xmm[0] = _mm_set_ps(0.0f, 0.0f, 0.0f, 1.0f);
            vTransforms.xmm[1] = _mm_set_ps(0.0f, 0.0f, 1.0f, 0.0f);
            vTransforms.xmm[2] = _mm_set_ps(0.0f, 1.0f, 0.0f, 0.0f);

            // Parallel Vector3D construction.
            const __m128 xMins = _mm_add_ps(xTileAABB, _mm_mul_ps( // Formula: tile->header->bmin[axis]+node->bmin[axis]*cs;
                _mm_setr_ps(pNode->bmin[0], pNode->bmin[1], pNode->bmin[2], 0.0f), xCellSize));
            const __m128 xMaxs = _mm_add_ps(xTileAABB, _mm_mul_ps( // Formula: tile->header->bmax[axis]+node->bmax[axis]*cs;
                _mm_setr_ps(pNode->bmax[0], pNode->bmax[1], pNode->bmax[2], 0.0f), xCellSize));

            v_RenderBox(vTransforms.mat, *reinterpret_cast<const Vector3D*>(&xMins), *reinterpret_cast<const Vector3D*>(&xMaxs),
                Color(188, 188, 188, 255), bDepthBuffer);
        }
    }
}

//------------------------------------------------------------------------------
// Purpose: draw NavMesh portals
// Input  : *pMesh - 
//------------------------------------------------------------------------------
void CAI_Utility::DrawNavMeshPortals(dtNavMesh* pMesh) const
{
    if (!pMesh)
        pMesh = GetNavMeshForHull(navmesh_debug_type->GetInt());
    if (!pMesh)
        return; // NavMesh for hull not loaded.

    const Vector3D vCamera = MainViewOrigin();
    const bool bDepthBuffer = r_debug_draw_depth_test->GetBool();
    const int nTileRange = navmesh_debug_tile_range->GetInt();
    const float flCameraRange = navmesh_debug_camera_range->GetFloat();

    for (int i = navmesh_draw_portal->GetInt(), nt = pMesh->getTileCount(); i < nt; ++i)
    {
        if (nTileRange > 0 && i > nTileRange)
            break;

        const dtMeshTile* pTile = &pMesh->m_tiles[i];
        if (!pTile->header)
            continue;

        if (flCameraRange > 0.0f)
        {
            const __m128 xMinBound = _mm_setr_ps(pTile->header->bmin[0], pTile->header->bmin[1], vCamera.z, 0.0f);
            const __m128 xMaxBound = _mm_setr_ps(pTile->header->bmax[0], pTile->header->bmax[1], vCamera.z, 0.0f);

            if (vCamera.DistTo(*reinterpret_cast<const Vector3D*>(&xMinBound)) > flCameraRange ||
                vCamera.DistTo(*reinterpret_cast<const Vector3D*>(&xMaxBound)) > flCameraRange)
                continue;
        }

        // Draw portals
        const float flPadX = 0.04f;
        const float flPadZ = pTile->header->walkableClimb;

        for (int nSide = 0; nSide < 8; ++nSide)
        {
            unsigned short m = DT_EXT_LINK | static_cast<unsigned short>(nSide);
            for (int j = 0, np = pTile->header->polyCount; j < np; ++j)
            {
                const dtPoly* pPoly = &pTile->polys[j];

                // Create new links.
                for (int v = 0, nv = pPoly->vertCount; v < nv; ++v)
                {
                    // Skip edges which do not point to the right side.
                    if (pPoly->neis[v] != m)
                        continue;

                    // Create new links
                    const float* va = &pTile->verts[pPoly->verts[v] * 3];
                    const float* vb = &pTile->verts[pPoly->verts[(v + 1) % nv] * 3];

                    /*****************
                     Vertex indices:
                     va - = 0 +------+
                     vb - = 1 |      |
                     va + = 2 |      |
                     vb + = 3 +------+
                     *****************/
                    __m128 xVerts = _mm_setr_ps(va[2], vb[2], va[2], vb[2]);
                    xVerts = _mm_sub_ps(xVerts, _mm_setr_ps(flPadZ, flPadZ, 0.0f, 0.0f));
                    xVerts = _mm_add_ps(xVerts, _mm_setr_ps(0.0f, 0.0f, flPadZ, flPadZ));

                    if (nSide == 0 || nSide == 4)
                    {
                        Color col = nSide == 0 ? Color(188, 0, 0, 255) : Color(188, 0, 188, 255);
                        const float x = va[0] + ((nSide == 0) ? -flPadX : flPadX);

                        __m128 xOrigin = _mm_setr_ps(x, va[1], xVerts.m128_f32[0], 0);
                        __m128 xDest = _mm_setr_ps(x, va[1], xVerts.m128_f32[2], 0);
                        v_RenderLine(*reinterpret_cast<Vector3D*>(&xOrigin), 
                            *reinterpret_cast<Vector3D*>(&xDest), col, bDepthBuffer);
                        xOrigin = _mm_setr_ps(x, va[1], xVerts.m128_f32[2], 0);
                        xDest = _mm_setr_ps(x, vb[1], xVerts.m128_f32[3], 0);
                        v_RenderLine(*reinterpret_cast<Vector3D*>(&xOrigin), 
                            *reinterpret_cast<Vector3D*>(&xDest), col, bDepthBuffer);
                        xOrigin = _mm_setr_ps(x, vb[1], xVerts.m128_f32[3], 0);
                        xDest = _mm_setr_ps(x, vb[1], xVerts.m128_f32[1], 0);
                        v_RenderLine(*reinterpret_cast<Vector3D*>(&xOrigin), 
                            *reinterpret_cast<Vector3D*>(&xDest), col, bDepthBuffer);
                        xOrigin = _mm_setr_ps(x, vb[1], xVerts.m128_f32[1], 0);
                        xDest = _mm_setr_ps(x, va[1], xVerts.m128_f32[0], 0);
                        v_RenderLine(*reinterpret_cast<Vector3D*>(&xOrigin), 
                            *reinterpret_cast<Vector3D*>(&xDest), col, bDepthBuffer);
                    }
                    else if (nSide == 2 || nSide == 6)
                    {
                        Color col = nSide == 2 ? Color(0, 188, 0, 255) : Color(188, 188, 0, 255);
                        const float y = va[1] + ((nSide == 2) ? -flPadX : flPadX);

                        __m128 xOrigin = _mm_setr_ps(va[0], y, xVerts.m128_f32[0], 0);
                        __m128 xDest = _mm_setr_ps(va[0], y, xVerts.m128_f32[2], 0);
                        v_RenderLine(*reinterpret_cast<Vector3D*>(&xOrigin),
                            *reinterpret_cast<Vector3D*>(&xDest), col, bDepthBuffer);
                        xOrigin = _mm_setr_ps(va[0], y, xVerts.m128_f32[2], 0);
                        xDest = _mm_setr_ps(vb[0], y, xVerts.m128_f32[3], 0);
                        v_RenderLine(*reinterpret_cast<Vector3D*>(&xOrigin),
                            *reinterpret_cast<Vector3D*>(&xDest), col, bDepthBuffer);
                        xOrigin = _mm_setr_ps(vb[0], y, xVerts.m128_f32[3], 0);
                        xDest = _mm_setr_ps(vb[0], y, xVerts.m128_f32[1], 0);
                        v_RenderLine(*reinterpret_cast<Vector3D*>(&xOrigin),
                            *reinterpret_cast<Vector3D*>(&xDest), col, bDepthBuffer);
                        xOrigin = _mm_setr_ps(vb[0], y, xVerts.m128_f32[1], 0);
                        xDest = _mm_setr_ps(va[0], y, xVerts.m128_f32[0], 0);
                        v_RenderLine(*reinterpret_cast<Vector3D*>(&xOrigin),
                            *reinterpret_cast<Vector3D*>(&xDest), col, bDepthBuffer);
                    }
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
// Purpose: draw NavMesh polys
// Input  : *pMesh - 
//------------------------------------------------------------------------------
void CAI_Utility::DrawNavMeshPolys(dtNavMesh* pMesh) const
{
    if (!pMesh)
        pMesh = GetNavMeshForHull(navmesh_debug_type->GetInt());
    if (!pMesh)
        return; // NavMesh for hull not loaded.

    const Vector3D vCamera = MainViewOrigin();
    const bool bDepthBuffer = r_debug_draw_depth_test->GetBool();
    const int nTileRange = navmesh_debug_tile_range->GetInt();
    const float flCameraRange = navmesh_debug_camera_range->GetFloat();

    for (int i = navmesh_draw_polys->GetInt(); i < pMesh->getTileCount(); ++i)
    {
        if (nTileRange > 0 && i > nTileRange)
            break;

        const dtMeshTile* pTile = &pMesh->m_tiles[i];
        if (!pTile->header)
            continue;

        if (flCameraRange > 0.0f)
        {
            const __m128 xMinBound = _mm_setr_ps(pTile->header->bmin[0], pTile->header->bmin[1], vCamera.z, 0.0f);
            const __m128 xMaxBound = _mm_setr_ps(pTile->header->bmax[0], pTile->header->bmax[1], vCamera.z, 0.0f);

            if (vCamera.DistTo(*reinterpret_cast<const Vector3D*>(&xMinBound)) > flCameraRange ||
                vCamera.DistTo(*reinterpret_cast<const Vector3D*>(&xMaxBound)) > flCameraRange)
                continue;
        }

        for (int j = 0; j < pTile->header->polyCount; j++)
        {
            const dtPoly* pPoly = &pTile->polys[j];

            Color col{ 110, 200, 220, 255 };
            const unsigned int ip = (unsigned int)(pPoly - pTile->polys);

            if (pPoly->getType() == DT_POLYTYPE_OFFMESH_CONNECTION)
            {
                const dtOffMeshConnection* con = &pTile->offMeshCons[ip - pTile->header->offMeshBase];
                v_RenderLine(Vector3D(con->pos[0], con->pos[1], con->pos[2]), 
                    Vector3D(con->pos[3], con->pos[4], con->pos[5]), Color(188, 0, 188, 255), bDepthBuffer);
            }
            else
            {
                const dtPolyDetail* pDetail = &pTile->detailMeshes[ip];
                __m128 xTris[3] = { _mm_setzero_ps() };
                for (int k = 0; k < pDetail->triCount; ++k)
                {
                    const unsigned char* t = &pTile->detailTris[(pDetail->triBase + k) * 4];
                    for (int e = 0; e < 3; ++e)
                    {
                        if (t[e] < pPoly->vertCount)
                        {
                            float* pflVerts = &pTile->verts[pPoly->verts[t[e]] * 3];
                            xTris[e] = _mm_setr_ps(pflVerts[0], pflVerts[1], pflVerts[2], 0.0f);
                        }
                        else
                        {
                            float* pflVerts = &pTile->detailVerts[(pDetail->vertBase + t[e] - pPoly->vertCount) * 3];
                            xTris[e] = _mm_setr_ps(pflVerts[0], pflVerts[1], pflVerts[2], 0.0f);
                        }
                    }

                    v_RenderLine(*reinterpret_cast<const Vector3D*>(&xTris[0]), 
                        *reinterpret_cast<const Vector3D*>(&xTris[1]), col, bDepthBuffer);
                    v_RenderLine(*reinterpret_cast<const Vector3D*>(&xTris[1]), 
                        *reinterpret_cast<const Vector3D*>(&xTris[2]), col, bDepthBuffer);
                    v_RenderLine(*reinterpret_cast<const Vector3D*>(&xTris[2]), 
                        *reinterpret_cast<const Vector3D*>(&xTris[0]), col, bDepthBuffer);
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
// Purpose : draw NavMesh poly boundaries
// Input  : *pMesh - 
//------------------------------------------------------------------------------
void CAI_Utility::DrawNavMeshPolyBoundaries(dtNavMesh* pMesh) const
{
    static const float thr = 0.01f * 0.01f;
    Color col{ 20, 140, 255, 255 };

    if (!pMesh)
        pMesh = GetNavMeshForHull(navmesh_debug_type->GetInt());
    if (!pMesh)
        return; // NavMesh for hull not loaded.

    const Vector3D vCamera = MainViewOrigin();
    const bool bDepthBuffer = r_debug_draw_depth_test->GetBool();
    const bool bDrawInner = navmesh_draw_poly_bounds_inner->GetBool();
    const int nTileRange = navmesh_debug_tile_range->GetInt();
    const float flCameraRange = navmesh_debug_camera_range->GetFloat();

    for (int i = navmesh_draw_poly_bounds->GetInt(), nt = pMesh->getTileCount(); i < nt; ++i)
    {
        if (nTileRange > 0 && i > nTileRange)
            break;

        const dtMeshTile* pTile = &pMesh->m_tiles[i];
        if (!pTile->header)
            continue;

        if (flCameraRange > 0.0f)
        {
            const __m128 xMinBound = _mm_setr_ps(pTile->header->bmin[0], pTile->header->bmin[1], vCamera.z, 0.0f);
            const __m128 xMaxBound = _mm_setr_ps(pTile->header->bmax[0], pTile->header->bmax[1], vCamera.z, 0.0f);

            if (vCamera.DistTo(*reinterpret_cast<const Vector3D*>(&xMinBound)) > flCameraRange ||
                vCamera.DistTo(*reinterpret_cast<const Vector3D*>(&xMaxBound)) > flCameraRange)
                continue;
        }

        for (int i = 0; i < pTile->header->polyCount; ++i)
        {
            const dtPoly* pPoly = &pTile->polys[i];

            if (pPoly->getType() == DT_POLYTYPE_OFFMESH_CONNECTION)
                continue;

            const dtPolyDetail* pd = &pTile->detailMeshes[i];

            for (int j = 0, nj = static_cast<int>(pPoly->vertCount); j < nj; ++j)
            {
                if (bDrawInner)
                {
                    if (pPoly->neis[j] == 0)
                        continue;

                    if (pPoly->neis[j] & DT_EXT_LINK)
                    {
                        bool bCon = false;
                        for (unsigned int k = pPoly->firstLink; k != DT_NULL_LINK; k = pTile->links[k].next)
                        {
                            if (pTile->links[k].edge == j)
                            {
                                bCon = true;
                                break;
                            }
                        }
                        if (bCon)
                            col = Color(255, 255, 255, 255);
                        else
                            col = Color(0, 0, 0, 255);
                    }
                    else
                        col = Color(0, 48, 64, 255);
                }
                else
                {
                    if (pPoly->neis[j] != 0)
                        continue;
                }

                const float* v0 = &pTile->verts[pPoly->verts[j] * 3];
                const float* v1 = &pTile->verts[pPoly->verts[(j + 1) % nj] * 3];

                // Draw detail mesh edges which align with the actual poly edge.
                // This is really slow.
                for (int k = 0, e = pd->triCount; k < e; ++k)
                {
                    const unsigned char* t = &pTile->detailTris[(pd->triBase + k) * 4];
                    const float* tv[3];
                    for (int m = 0; m < 3; ++m)
                    {
                        if (t[m] < pPoly->vertCount)
                            tv[m] = &pTile->verts[pPoly->verts[t[m]] * 3];
                        else
                            tv[m] = &pTile->detailVerts[(pd->vertBase + (t[m] - pPoly->vertCount)) * 3];
                    }
                    for (int m = 0, n = 2; m < 3; n = m++)
                    {
                        if ((dtGetDetailTriEdgeFlags(t[3], n) & DT_DETAIL_EDGE_BOUNDARY) == 0)
                            continue;

                        if (distancePtLine2d(tv[n], v0, v1) < thr &&
                            distancePtLine2d(tv[m], v0, v1) < thr)
                        {
                            v_RenderLine(Vector3D(tv[n][0], tv[n][1], tv[n][2]), Vector3D(tv[m][0], tv[m][1], tv[m][2]), col, bDepthBuffer);
                        }
                    }
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
// Purpose: packs 4 node indices together
// Input  : a - (set 1)
//          b - 
//          c - (set 2)
//          d - 
// Output : packed node set as i64x2
//------------------------------------------------------------------------------
__m128i CAI_Utility::PackNodeLink(int32_t a, int32_t b, int32_t c, int32_t d) const
{
    __m128i xResult = _mm_set_epi32(a, b, c, d);

    // We shuffle a b and c d if following condition is met, this is to 
    // ensure we always end up with one possible combination of indices.
    if (a < b) // Swap 'a' with 'b'.
        xResult = _mm_shuffle_epi32(xResult, _MM_SHUFFLE(2, 3, 1, 0));
    if (c < d) // Swap 'c' with 'd'.
        xResult = _mm_shuffle_epi32(xResult, _MM_SHUFFLE(3, 2, 0, 1));

    return xResult;
}

//------------------------------------------------------------------------------
// Purpose: gets the nearest node index to position
// Input  : *vPos       - 
//          *pAINetwork - 
// Output : node index ('NO_NODE' if no node has been found)
//------------------------------------------------------------------------------
int64_t CAI_Utility::GetNearestNodeToPos(const CAI_Network* pAINetwork, const Vector3D* vPos) const
{
    __int64 result; // rax
    unsigned int v3; // er10
    __int64 v4; // rdx
    float v5; // xmm3_4
    unsigned int v6; // er8
    CAI_ScriptNode* v7; // rax
    float v8; // xmm4_4
    float v9; // xmm5_4
    float v10; // xmm6_4
    float* v11; // rcx
    float* v12; // rax
    float v13; // xmm7_4
    float v14; // xmm2_4
    unsigned int v15; // er9
    float v16; // xmm8_4
    float v17; // xmm2_4
    unsigned int v18; // er8
    float v19; // xmm9_4
    float v20; // xmm2_4
    unsigned int v21; // er9
    float v22; // xmm7_4
    float v23; // xmm2_4
    float* v24; // r9
    float v25; // xmm4_4
    float v26; // xmm2_4
    unsigned int v27; // ecx

    if (pAINetwork)
    {
        v3 = pAINetwork->m_iNumScriptNodes;
        v4 = 0i64;
        v5 = 640000.0;
        v6 = -1;
        if (v3 >= 4)
        {
            v7 = pAINetwork->m_ScriptNode;
            v8 = vPos->x;
            v9 = vPos->y;
            v10 = vPos->z;
            v11 = &v7->m_vOrigin.z;
            v12 = &v7[1].m_vOrigin.y;
            do
            {
                v13 = v5;
                v14 = (float)((float)((float)(*(v11 - 1) - v9) * (float)(*(v11 - 1) - v9)) + (float)((float)(*(v11 - 2) - v8) * (float)(*(v11 - 2) - v8))) + (float)((float)(*v11 - v10) * (float)(*v11 - v10));
                if (v5 > v14)
                    v5 = (float)((float)((float)(*(v11 - 1) - v9) * (float)(*(v11 - 1) - v9)) + (float)((float)(*(v11 - 2) - v8) * (float)(*(v11 - 2) - v8))) + (float)((float)(*v11 - v10) * (float)(*v11 - v10));
                v15 = v4;
                if (v13 <= v14)
                    v15 = v6;
                v16 = v5;
                v17 = (float)((float)((float)(*(v12 - 1) - v9) * (float)(*(v12 - 1) - v9)) + (float)((float)(v11[3] - v8) * (float)(v11[3] - v8))) + (float)((float)(*v12 - v10) * (float)(*v12 - v10));
                if (v5 > v17)
                    v5 = (float)((float)((float)(*(v12 - 1) - v9) * (float)(*(v12 - 1) - v9)) + (float)((float)(v11[3] - v8) * (float)(v11[3] - v8))) + (float)((float)(*v12 - v10) * (float)(*v12 - v10));
                v18 = v4 + 1;
                if (v16 <= v17)
                    v18 = v15;
                v19 = v5;
                v20 = (float)((float)((float)(v12[4] - v9) * (float)(v12[4] - v9)) + (float)((float)(v11[8] - v8) * (float)(v11[8] - v8))) + (float)((float)(v12[5] - v10) * (float)(v12[5] - v10));
                if (v5 > v20)
                    v5 = (float)((float)((float)(v12[4] - v9) * (float)(v12[4] - v9)) + (float)((float)(v11[8] - v8) * (float)(v11[8] - v8))) + (float)((float)(v12[5] - v10) * (float)(v12[5] - v10));
                v21 = v4 + 2;
                if (v19 <= v20)
                    v21 = v18;
                v22 = v5;
                v23 = (float)((float)((float)(v12[9] - v9) * (float)(v12[9] - v9)) + (float)((float)(v11[13] - v8) * (float)(v11[13] - v8))) + (float)((float)(v12[10] - v10) * (float)(v12[10] - v10));
                if (v5 > v23)
                    v5 = (float)((float)((float)(v12[9] - v9) * (float)(v12[9] - v9)) + (float)((float)(v11[13] - v8) * (float)(v11[13] - v8))) + (float)((float)(v12[10] - v10) * (float)(v12[10] - v10));
                v6 = v4 + 3;
                if (v22 <= v23)
                    v6 = v21;
                v11 += 20;
                v12 += 20;
                v4 = (unsigned int)(v4 + 4);
            }       while ((unsigned int)v4 < v3 - 3);
        }
        if ((unsigned int)v4 < v3)
        {
            v24 = &pAINetwork->m_ScriptNode->m_vOrigin.x + 5 * v4;
            do
            {
                v25 = v5;
                v26 = (float)((float)((float)(v24[1] - vPos->y) * (float)(v24[1] - vPos->y)) + (float)((float)(*v24 - vPos->x) * (float)(*v24 - vPos->x)))
                    + (float)((float)(v24[2] - vPos->z) * (float)(v24[2] - vPos->z));
                if (v5 > v26)
                    v5 = (float)((float)((float)(v24[1] - vPos->y) * (float)(v24[1] - vPos->y)) + (float)((float)(*v24 - vPos->x) * (float)(*v24 - vPos->x)))
                    + (float)((float)(v24[2] - vPos->z) * (float)(v24[2] - vPos->z));
                v27 = v4;
                if (v25 <= v26)
                    v27 = v6;
                v24 += 5;
                LODWORD(v4) = v4 + 1;
                v6 = v27;
            }       while ((unsigned int)v4 < v3);
        }
        result = v6;
    }
    else
    {
        result = 0i64;
    }
    return result;
}

CAI_Utility* g_pAIUtility = new (CAI_Utility);
