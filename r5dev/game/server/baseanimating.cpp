//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Base class for all animating characters and objects.
//
//===========================================================================//
#include "core/stdafx.h"
#include "baseanimating.h"
#include "engine/modelinfo.h"
#include "public/idebugoverlay.h"

static Vector3D	hullcolor[8] =
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
	if (!m_pStudioHdr && g_pModelInfoServer->GetModel(m_nModelIndex))
	{
		UpdateModelPtr();
		if (!m_pStudioHdr)
		{
			return;
		}
	}

    mstudiohitboxset_t* set = m_pStudioHdr->pHitboxSet(m_nHitboxSet);
    if (!set)
        return;

    matrix3x4_t transforms;

	int r = 0;
	int g = 0;
	int b = 255;

    for (int i = 0; i < set->numhitboxes; i++)
    {
        mstudiobbox_t* pBox = set->pHitbox(i);

		int j = (pBox->group % 8);

		r = (int)(255.0f * hullcolor[j][0]);
		g = (int)(255.0f * hullcolor[j][1]);
		b = (int)(255.0f * hullcolor[j][2]);

        HitboxToWorldTransforms(pBox->bone, &transforms);
        g_pDebugOverlay->AddBoxOverlay(transforms, pBox->bbmin, pBox->bbmax, r, g, b, 0, true, duration);
    }
}

void CBaseAnimating::HitboxToWorldTransforms(uint32_t iBone, matrix3x4_t* transforms)
{
    const static int index = 285;
    CallVFunc<void>(index, this, iBone, transforms);
}

//-----------------------------------------------------------------------------
// Purpose: sets studio pointer to an updated studiomdl cache
//-----------------------------------------------------------------------------
void CBaseAnimating::UpdateModelPtr()
{
	// Populates the 'm_pStudioHdr' field.
	v_CBaseAnimating__UpdateModelPtr(this);
}

void DrawServerHitboxes(CBaseAnimating* thisp, float duration)
{
    thisp->DrawServerHitboxes(duration);
}

void BaseAnimating_Attach()
{
    DetourAttach(&v_CBaseAnimating__DrawServerHitboxes, &DrawServerHitboxes);
}
void BaseAnimating_Detach()
{
    DetourDetach(&v_CBaseAnimating__DrawServerHitboxes, &DrawServerHitboxes);
}