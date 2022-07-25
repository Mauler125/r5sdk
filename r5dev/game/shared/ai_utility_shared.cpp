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
#include "game/server/ai_network.h"
#include "game/server/ai_networkmanager.h"
#include "game/client/view.h"
#include "thirdparty/recast/detour/include/detourcommon.h"

//------------------------------------------------------------------------------
// Purpose : draw AIN script nodes
//------------------------------------------------------------------------------
void DrawAIScriptNodes()
{
    if (*g_pAINetwork)
    {
        OverlayBox_t::Transforms vTransforms;

        for (int i = ai_script_nodes_draw->GetInt(); i < (*g_pAINetwork)->GetNumScriptNodes(); i++)
        {
            if (ai_script_nodes_draw_range->GetBool())
            {
                if (i > ai_script_nodes_draw_range->GetInt())
                    break;
            }

            vTransforms.xmm[0] = _mm_set_ps((*g_pAINetwork)->m_ScriptNode[i].m_vOrigin.x - 50.f, 0.0f, 0.0f, 1.0f);
            vTransforms.xmm[1] = _mm_set_ps((*g_pAINetwork)->m_ScriptNode[i].m_vOrigin.y - 50.f, 0.0f, 1.0f, 0.0f);
            vTransforms.xmm[2] = _mm_set_ps((*g_pAINetwork)->m_ScriptNode[i].m_vOrigin.z - 50.f, 1.0f, 0.0f, 0.0f);

            v_RenderBox(vTransforms, { 0, 0, 0 }, { 100, 100, 100 }, Color(0, 255, 0, 255), r_debug_overlay_zbuffer->GetBool());

            if (i > 0)
                v_RenderLine((*g_pAINetwork)->m_ScriptNode[i - 1].m_vOrigin, (*g_pAINetwork)->m_ScriptNode[i].m_vOrigin, Color(255, 0, 0, 255), r_debug_overlay_zbuffer->GetBool());
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
            const Vector3D vCamera = MainViewOrigin();

            if (vCamera.DistTo(Vector3D(tile->header->bmin[0], tile->header->bmin[1], vCamera.z)) > navmesh_debug_camera_range->GetFloat() ||
                vCamera.DistTo(Vector3D(tile->header->bmax[0], tile->header->bmax[1], vCamera.z)) > navmesh_debug_camera_range->GetFloat())
                continue;
        }

        const float cs = 1.0f / tile->header->bvQuantFactor;
        for (int j = 0; j < tile->header->bvNodeCount; ++j)
        {
            const dtBVNode* node = &tile->bvTree[j];
            if (node->i < 0) // Leaf indices are positive.
                continue;

            vTransforms.xmm[0] = _mm_set_ps(0.0f, 0.0f, 0.0f, 1.0f);
            vTransforms.xmm[1] = _mm_set_ps(0.0f, 0.0f, 1.0f, 0.0f);
            vTransforms.xmm[2] = _mm_set_ps(0.0f, 1.0f, 0.0f, 0.0f);

            const Vector3D vMins(
                tile->header->bmin[0] + node->bmin[0] * cs,
                tile->header->bmin[1] + node->bmin[1] * cs,
                tile->header->bmin[2] + node->bmin[2] * cs);
            const Vector3D vMaxs(
                tile->header->bmin[0] + node->bmax[0] * cs,
                tile->header->bmin[1] + node->bmax[1] * cs,
                tile->header->bmin[2] + node->bmax[2] * cs);

            v_RenderBox(vTransforms, vMins, vMaxs, Color(188, 188, 188, 255), r_debug_overlay_zbuffer->GetBool());
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
