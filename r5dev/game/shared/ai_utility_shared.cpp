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
#include "game/client/view.h"
#include "thirdparty/recast/detour/include/detourcommon.h"

//------------------------------------------------------------------------------
// Purpose : draw AIN script nodes
//------------------------------------------------------------------------------
void DrawAIScriptNodes()
{
    if (*g_pAINetwork)
    {
        const bool bUseDepthBuffer = r_debug_overlay_zbuffer->GetBool();
        static const __m128 xSubMask = _mm_setr_ps(50.0f, 50.0f, 50.0f, 0.0f);

        OverlayBox_t::Transforms vTransforms;
        std::unordered_set<uint64_t> linkSet;

        for (int i = ai_script_nodes_draw->GetInt(), j = (*g_pAINetwork)->GetNumScriptNodes(); i < j; i++)
        {
            if (ai_script_nodes_draw_range->GetBool())
            {
                if (i > ai_script_nodes_draw_range->GetInt())
                    break;
            }

            const CAI_ScriptNode* pScriptNode = &(*g_pAINetwork)->m_ScriptNode[i];

            const __m128 xOrigin = _mm_setr_ps(pScriptNode->m_vOrigin.x, pScriptNode->m_vOrigin.y, pScriptNode->m_vOrigin.z, 0.0f);
            const __m128 xResult = _mm_sub_ps(xOrigin, xSubMask); // Subtract 50.f from our scalars to align box with node.

            vTransforms.xmm[0] = _mm_set_ps(xResult.m128_f32[0], 0.0f, 0.0f, 1.0f);
            vTransforms.xmm[1] = _mm_set_ps(xResult.m128_f32[1], 0.0f, 1.0f, 0.0f);
            vTransforms.xmm[2] = _mm_set_ps(xResult.m128_f32[2], 1.0f, 0.0f, 0.0f);

            v_RenderBox(vTransforms, { 0, 0, 0 }, { 100, 100, 100 }, Color(0, 255, 0, 255), bUseDepthBuffer);

            if (ai_script_nodes_draw_nearest->GetBool())
            {
                int nNearest = GetNearestNodeToPos(&pScriptNode->m_vOrigin);
                if (nNearest != NO_NODE) // NO_NODE = -1
                {
                    auto p = linkSet.insert(PackNodeLink(i, nNearest));
                    if (p.second)
                    {
                        const CAI_ScriptNode* pNearestNode = &(*g_pAINetwork)->m_ScriptNode[nNearest];
                        v_RenderLine(pScriptNode->m_vOrigin, pNearestNode->m_vOrigin, Color(255, 0, 0, 255), bUseDepthBuffer);
                    }
                }
            }
            else if (i > 0) // Render links in the order the AI Network was build.
                v_RenderLine((pScriptNode - 1)->m_vOrigin, pScriptNode->m_vOrigin, Color(255, 0, 0, 255), bUseDepthBuffer);
        }
    }
}

//------------------------------------------------------------------------------
// Purpose : draw NavMesh BVTree
//------------------------------------------------------------------------------
void DrawNavMeshBVTree()
{
    const dtNavMesh* mesh = GetNavMeshForHull(navmesh_debug_type->GetInt());
    if (!mesh)
        return;
    const Vector3D vCamera = MainViewOrigin();

    OverlayBox_t::Transforms vTransforms;
    for (int i = navmesh_draw_bvtree->GetInt(); i < mesh->getTileCount(); ++i)
    {
        if (navmesh_debug_tile_range->GetBool())
        {
            if (i > navmesh_debug_tile_range->GetInt())
                break;
        }

        const dtMeshTile* tile = &mesh->m_tiles[i];
        if (!tile->header)
            continue;

        if (navmesh_debug_camera_range->GetBool())
        {
            if (vCamera.DistTo(Vector3D(tile->header->bmin[0], tile->header->bmin[1], vCamera.z)) > navmesh_debug_camera_range->GetFloat() ||
                vCamera.DistTo(Vector3D(tile->header->bmax[0], tile->header->bmax[1], vCamera.z)) > navmesh_debug_camera_range->GetFloat())
                continue;
        }

        const float cs = 1.0f / tile->header->bvQuantFactor;
        for (int j = 0, k = tile->header->bvNodeCount; j < k; ++j)
        {
            const dtBVNode* node = &tile->bvTree[j];
            if (node->i < 0) // Leaf indices are positive.
                continue;

            vTransforms.xmm[0] = _mm_set_ps(0.0f, 0.0f, 0.0f, 1.0f);
            vTransforms.xmm[1] = _mm_set_ps(0.0f, 0.0f, 1.0f, 0.0f);
            vTransforms.xmm[2] = _mm_set_ps(0.0f, 1.0f, 0.0f, 0.0f);

            const __m128 xMinTileAABB = _mm_setr_ps(tile->header->bmin[0], tile->header->bmin[1], tile->header->bmin[2], 0.0f);
            const __m128 xQuantMask = _mm_setr_ps(cs, cs, cs, 0.0f);

            // Parallel Vector3D construction.
            const __m128 xMinRet = _mm_add_ps(xMinTileAABB, _mm_mul_ps( // Formula: tile->header->bmin[axis] + node->bmin[axis] * cs;
                _mm_setr_ps(node->bmin[0], node->bmin[1], node->bmin[2], 0.0f), xQuantMask));
            const __m128 xMaxRet = _mm_add_ps(xMinTileAABB, _mm_mul_ps( // Formula: tile->header->bmin[axis] + node->bmax[axis] * cs;
                _mm_setr_ps(node->bmax[0], node->bmax[1], node->bmax[2], 0.0f), xQuantMask));

            v_RenderBox(vTransforms, *reinterpret_cast<const Vector3D*>(&xMinRet), *reinterpret_cast<const Vector3D*>(&xMaxRet), 
                Color(188, 188, 188, 255), r_debug_overlay_zbuffer->GetBool());
        }
    }
}

//------------------------------------------------------------------------------
// Purpose : draw NavMesh portals
//------------------------------------------------------------------------------
void DrawNavMeshPortals()
{
    const dtNavMesh* mesh = GetNavMeshForHull(navmesh_debug_type->GetInt());
    if (!mesh)
        return;

    OverlayBox_t::Transforms vTransforms;
    for (int i = navmesh_draw_portal->GetInt(); i < mesh->getTileCount(); ++i)
    {
        if (navmesh_debug_tile_range->GetBool())
        {
            if (i > navmesh_debug_tile_range->GetInt())
                break;
        }

        const dtMeshTile* tile = &mesh->m_tiles[i];
        if (!tile->header)
            continue;

        if (navmesh_debug_camera_range->GetBool())
        {
            const Vector3D vCamera = MainViewOrigin();

            if (vCamera.DistTo(Vector3D(tile->header->bmin[0], tile->header->bmin[1], vCamera.z)) > navmesh_debug_camera_range->GetFloat() ||
                vCamera.DistTo(Vector3D(tile->header->bmax[0], tile->header->bmax[1], vCamera.z)) > navmesh_debug_camera_range->GetFloat())
                continue;
        }

        // Draw portals
        const float padx = 0.04f;
        const float padz = tile->header->walkableClimb;

        for (int side = 0; side < 8; ++side)
        {
            unsigned short m = DT_EXT_LINK | static_cast<unsigned short>(side);
            for (int i = 0; i < tile->header->polyCount; ++i)
            {
                const dtPoly* poly = &tile->polys[i];

                // Create new links.
                const int nv = poly->vertCount;
                for (int j = 0; j < nv; ++j)
                {
                    // Skip edges which do not point to the right side.
                    if (poly->neis[j] != m)
                        continue;

                    // Create new links
                    const float* va = &tile->verts[poly->verts[j] * 3];
                    const float* vb = &tile->verts[poly->verts[(j + 1) % nv] * 3];

                    if (side == 0 || side == 4)
                    {
                        Color col = side == 0 ? Color(188, 0, 0, 255) : Color(188, 0, 188, 255);
                        const float x = va[0] + ((side == 0) ? -padx : padx);

                        v_RenderLine(Vector3D(x, va[1], va[2] - padz), Vector3D(x, va[1], va[2] + padz), col, r_debug_overlay_zbuffer->GetBool());
                        v_RenderLine(Vector3D(x, va[1], va[2] + padz), Vector3D(x, vb[1], vb[2] + padz), col, r_debug_overlay_zbuffer->GetBool());
                        v_RenderLine(Vector3D(x, vb[1], vb[2] + padz), Vector3D(x, vb[1], vb[2] - padz), col, r_debug_overlay_zbuffer->GetBool());
                        v_RenderLine(Vector3D(x, vb[1], vb[2] - padz), Vector3D(x, va[1], va[2] - padz), col, r_debug_overlay_zbuffer->GetBool());
                    }
                    else if (side == 2 || side == 6)
                    {
                        Color col = side == 2 ? Color(0, 188, 0, 255) : Color(188, 188, 0, 255);
                        const float y = va[1] + ((side == 2) ? -padx : padx);

                        v_RenderLine(Vector3D(va[0], y, va[2] - padz), Vector3D(va[0], y, va[2] + padz), col, r_debug_overlay_zbuffer->GetBool());
                        v_RenderLine(Vector3D(va[0], y, va[2] + padz), Vector3D(vb[0], y, vb[2] + padz), col, r_debug_overlay_zbuffer->GetBool());
                        v_RenderLine(Vector3D(vb[0], y, vb[2] + padz), Vector3D(vb[0], y, vb[2] - padz), col, r_debug_overlay_zbuffer->GetBool());
                        v_RenderLine(Vector3D(vb[0], y, vb[2] - padz), Vector3D(va[0], y, va[2] - padz), col, r_debug_overlay_zbuffer->GetBool());
                    }
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
// Purpose : draw NavMesh polys
//------------------------------------------------------------------------------
void DrawNavMeshPolys()
{
    const dtNavMesh* mesh = GetNavMeshForHull(navmesh_debug_type->GetInt());
    if (!mesh)
        return;

    OverlayBox_t::Transforms vTransforms;
    for (int i = navmesh_draw_polys->GetInt(); i < mesh->getTileCount(); ++i)
    {
        if (navmesh_debug_tile_range->GetBool())
        {
            if (i > navmesh_debug_tile_range->GetInt())
                break;
        }

        const dtMeshTile* tile = &mesh->m_tiles[i];
        if (!tile->header)
            continue;

        if (navmesh_debug_camera_range->GetBool())
        {
            const Vector3D vCamera = MainViewOrigin();

            if (vCamera.DistTo(Vector3D(tile->header->bmin[0], tile->header->bmin[1], vCamera.z)) > navmesh_debug_camera_range->GetFloat() ||
                vCamera.DistTo(Vector3D(tile->header->bmax[0], tile->header->bmax[1], vCamera.z)) > navmesh_debug_camera_range->GetFloat())
                continue;
        }

        for (int j = 0; j < tile->header->polyCount; j++)
        {
            const dtPoly* poly = &tile->polys[j];

            Color col{ 110, 200, 220, 255 };
            const unsigned int ip = (unsigned int)(poly - tile->polys);

            if (poly->getType() == DT_POLYTYPE_OFFMESH_CONNECTION)
            {
                const dtOffMeshConnection* con = &tile->offMeshCons[ip - tile->header->offMeshBase];
                v_RenderLine(Vector3D(con->pos[0], con->pos[1], con->pos[2]), Vector3D(con->pos[3], con->pos[4], con->pos[5]), Color(188, 0, 188, 255), r_debug_overlay_zbuffer->GetBool());
            }
            else
            {
                const dtPolyDetail* pd = &tile->detailMeshes[ip];

                //dd->begin(DU_DRAW_TRIS);
                for (int k = 0; k < pd->triCount; ++k)
                {
                    Vector3D tris[3];
                    const unsigned char* t = &tile->detailTris[(pd->triBase + k) * 4];
                    for (int e = 0; e < 3; ++e)
                    {
                        if (t[e] < poly->vertCount)
                        {
                            float* verts = &tile->verts[poly->verts[t[e]] * 3];
                            tris[e].x = verts[0];
                            tris[e].y = verts[1];
                            tris[e].z = verts[2];
                        }
                        else
                        {
                            float* verts = &tile->detailVerts[(pd->vertBase + t[e] - poly->vertCount) * 3];
                            tris[e].x = verts[0];
                            tris[e].y = verts[1];
                            tris[e].z = verts[2];
                        }
                    }

                    v_RenderLine(tris[0], tris[1], col, r_debug_overlay_zbuffer->GetBool());
                    v_RenderLine(tris[1], tris[2], col, r_debug_overlay_zbuffer->GetBool());
                    v_RenderLine(tris[2], tris[0], col, r_debug_overlay_zbuffer->GetBool());
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
// Purpose : draw NavMesh poly boundaries
//------------------------------------------------------------------------------
void DrawNavMeshPolyBoundaries()
{
    static const float thr = 0.01f * 0.01f;
    Color col{ 20, 140, 255, 255 };

    const dtNavMesh* mesh = GetNavMeshForHull(navmesh_debug_type->GetInt());
    if (!mesh)
        return;

    OverlayBox_t::Transforms vTransforms;
    for (int i = navmesh_draw_poly_bounds->GetInt(); i < mesh->getTileCount(); ++i)
    {
        if (navmesh_debug_tile_range->GetBool())
        {
            if (i > navmesh_debug_tile_range->GetInt())
                break;
        }

        const dtMeshTile* tile = &mesh->m_tiles[i];
        if (!tile->header)
            continue;

        if (navmesh_debug_camera_range->GetBool())
        {
            const Vector3D vCamera = MainViewOrigin();

            if (vCamera.DistTo(Vector3D(tile->header->bmin[0], tile->header->bmin[1], vCamera.z)) > navmesh_debug_camera_range->GetFloat() ||
                vCamera.DistTo(Vector3D(tile->header->bmax[0], tile->header->bmax[1], vCamera.z)) > navmesh_debug_camera_range->GetFloat())
                continue;
        }

        for (int i = 0; i < tile->header->polyCount; ++i)
        {
            const dtPoly* p = &tile->polys[i];

            if (p->getType() == DT_POLYTYPE_OFFMESH_CONNECTION)
                continue;

            const dtPolyDetail* pd = &tile->detailMeshes[i];

            for (int j = 0, nj = (int)p->vertCount; j < nj; ++j)
            {
                if (navmesh_draw_poly_bounds_inner->GetBool())
                {
                    if (p->neis[j] == 0)
                        continue;

                    if (p->neis[j] & DT_EXT_LINK)
                    {
                        bool con = false;
                        for (unsigned int k = p->firstLink; k != DT_NULL_LINK; k = tile->links[k].next)
                        {
                            if (tile->links[k].edge == j)
                            {
                                con = true;
                                break;
                            }
                        }
                        if (con)
                            col = Color(255, 255, 255, 255);
                        else
                            col = Color(0, 0, 0, 255);
                    }
                    else
                        col = Color(0, 48, 64, 255);
                }
                else
                {
                    if (p->neis[j] != 0) continue;
                }

                const float* v0 = &tile->verts[p->verts[j] * 3];
                const float* v1 = &tile->verts[p->verts[(j + 1) % nj] * 3];

                // Draw detail mesh edges which align with the actual poly edge.
                // This is really slow.
                for (int k = 0; k < pd->triCount; ++k)
                {
                    const unsigned char* t = &tile->detailTris[(pd->triBase + k) * 4];
                    const float* tv[3];
                    for (int m = 0; m < 3; ++m)
                    {
                        if (t[m] < p->vertCount)
                            tv[m] = &tile->verts[p->verts[t[m]] * 3];
                        else
                            tv[m] = &tile->detailVerts[(pd->vertBase + (t[m] - p->vertCount)) * 3];
                    }
                    for (int m = 0, n = 2; m < 3; n = m++)
                    {
                        if ((dtGetDetailTriEdgeFlags(t[3], n) & DT_DETAIL_EDGE_BOUNDARY) == 0)
                            continue;

                        if (distancePtLine2d(tv[n], v0, v1) < thr &&
                            distancePtLine2d(tv[m], v0, v1) < thr)
                        {
                            v_RenderLine(Vector3D(tv[n][0], tv[n][1], tv[n][2]), Vector3D(tv[m][0], tv[m][1], tv[m][2]), col, r_debug_overlay_zbuffer->GetBool());
                        }
                    }
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
// Purpose: packs 2 node indices together
// Input  : a - 
//          b - 
// Output : packed node set as u64
//------------------------------------------------------------------------------
uint64_t PackNodeLink(uint32_t a, uint32_t b)
{
    if (a < b)
        std::swap(a, b);

    uint64_t c = static_cast<uint64_t>(a) << 32;
    return c = c + static_cast<uint64_t>(b);
}

//------------------------------------------------------------------------------
// Purpose: gets the nearest node index to position
// Input  : *vPos
// Output : node index (NO_NODEif no node has been found)
//------------------------------------------------------------------------------
int64_t GetNearestNodeToPos(const Vector3D* vPos)
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

    if (*g_pAINetwork)
    {
        v3 = (*g_pAINetwork)->m_iNumScriptNodes;
        v4 = 0i64;
        v5 = 640000.0;
        v6 = -1;
        if (v3 >= 4)
        {
            v7 = (*g_pAINetwork)->m_ScriptNode;
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
            v24 = &(*g_pAINetwork)->m_ScriptNode->m_vOrigin.x + 5 * v4;
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
