//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
//===========================================================================//

#include "core/stdafx.h"
#include "mathlib/vector.h"
#include "game/client/viewrender.h"

VMatrix* CViewRender::GetWorldMatrixForView(int8_t slot)
{
	return CViewRender_GetWorldMatrixForView(this, slot);
}

const Vector3D& MainViewOrigin()
{
	return (*g_vecRenderOrigin);
}

const QAngle& MainViewAngles()
{
	return (*g_vecRenderAngles);
}
