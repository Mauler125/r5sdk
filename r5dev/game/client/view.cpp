//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
//===========================================================================//

#include "core/stdafx.h"
#include "mathlib/vector.h"
#include "game/client/view.h"

const Vector3D& MainViewOrigin()
{
	return (*g_vecRenderOrigin);
}

const QAngle& MainViewAngles()
{
	return (*g_vecRenderAngles);
}
