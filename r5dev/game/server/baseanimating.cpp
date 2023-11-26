//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Base class for all animating characters and objects.
//
//===========================================================================//
#include "core/stdafx.h"
#include "tier1/cvar.h"
#include "baseanimating.h"
#include "engine/modelinfo.h"
#include "public/idebugoverlay.h"

static const Vector3D s_HullColor[8] =
{
	Vector3D(1.0, 1.0, 1.0),
	Vector3D(1.0, 0.5, 0.5),
	Vector3D(0.5, 1.0, 0.5),
	Vector3D(1.0, 1.0, 0.5),
	Vector3D(0.5, 0.5, 1.0),
	Vector3D(1.0, 0.5, 1.0),
	Vector3D(0.5, 1.0, 1.0),
	Vector3D(1.0, 1.0, 1.0)
};

//-----------------------------------------------------------------------------
// Purpose: Send the current hitboxes for this model to the client ( to compare with
//  r_drawentities 3 client side boxes ).
// WARNING:  This uses a ton of bandwidth, only use on a listen server
//-----------------------------------------------------------------------------
void CBaseAnimating::DrawServerHitboxes(float duration /*= 0.0f*/)
{
	CStudioHdr* pStudioHdr = GetModelPtr();
	if (!GetModelPtr())
		return;

    mstudiohitboxset_t* pSet = pStudioHdr->GetHitboxSet(m_nHitboxSet);
    if (!pSet)
        return;

    matrix3x4_t transforms;

	int r = 0;
	int g = 0;
	int b = 255;
	bool bDepthTest = !debug_draw_box_depth_test->GetBool();

    for (int i = 0; i < pSet->numhitboxes; i++)
    {
        mstudiobbox_t* pBox = pSet->pHitbox(i);

		int j = (pBox->group % 8);

		r = static_cast<int>(255.0f * s_HullColor[j][0]);
		g = static_cast<int>(255.0f * s_HullColor[j][1]);
		b = static_cast<int>(255.0f * s_HullColor[j][2]);

        HitboxToWorldTransforms(pBox->bone, &transforms);
        g_pDebugOverlay->AddBoxOverlay(transforms, pBox->bbmin, pBox->bbmax, r, g, b, 0, bDepthTest, duration);
    }
}

void CBaseAnimating::HitboxToWorldTransforms(uint32_t iBone, matrix3x4_t* transforms)
{
    constexpr int index = 285;
    CallVFunc<void>(index, this, iBone, transforms);
}

//-----------------------------------------------------------------------------
// Purpose: sets studio pointer to an updated studiomdl cache
//-----------------------------------------------------------------------------
void CBaseAnimating::LockStudioHdr()
{
	// Populates the 'm_pStudioHdr' field.
	v_CBaseAnimating__LockStudioHdr(this);
}

CStudioHdr* CBaseAnimating::GetModelPtr(void)
{
	if (!m_pStudioHdr && GetModel())
	{
		LockStudioHdr();
	}
	return (m_pStudioHdr && m_pStudioHdr->IsValid()) ? m_pStudioHdr : nullptr;
}
