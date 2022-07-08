//===== Copyright � 2005-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: A set of utilities to render standard shapes
//
//===========================================================================//

#include "core/stdafx.h"
#include "mathlib/color.h"
#include "mathlib/vector.h"
#include "mathlib/vector2d.h"
#include "mathlib/vector4d.h"
#include "mathlib/mathlib.h"
#include "tier2/renderutils.h"
#include "engine/debugoverlay.h"

void DrawAngledBox(const Vector3D& origin, const QAngle& angles, Vector3D mins, Vector3D maxs, int r, int g, int b, int a, bool throughSolid)
{
    Vector3D orgs[8];
    PointsFromAngledBox(angles, mins, maxs, &*orgs);

    v_RenderLine(origin + orgs[0], origin + orgs[1], Color(r, g, b, a), throughSolid);
    v_RenderLine(origin + orgs[1], origin + orgs[2], Color(r, g, b, a), throughSolid);
    v_RenderLine(origin + orgs[2], origin + orgs[3], Color(r, g, b, a), throughSolid);
    v_RenderLine(origin + orgs[3], origin + orgs[0], Color(r, g, b, a), throughSolid);

    v_RenderLine(origin + orgs[4], origin + orgs[5], Color(r, g, b, a), throughSolid);
    v_RenderLine(origin + orgs[5], origin + orgs[6], Color(r, g, b, a), throughSolid);
    v_RenderLine(origin + orgs[6], origin + orgs[7], Color(r, g, b, a), throughSolid);
    v_RenderLine(origin + orgs[7], origin + orgs[4], Color(r, g, b, a), throughSolid);

    v_RenderLine(origin + orgs[0], origin + orgs[4], Color(r, g, b, a), throughSolid);
    v_RenderLine(origin + orgs[1], origin + orgs[5], Color(r, g, b, a), throughSolid);
    v_RenderLine(origin + orgs[2], origin + orgs[6], Color(r, g, b, a), throughSolid);
    v_RenderLine(origin + orgs[3], origin + orgs[7], Color(r, g, b, a), throughSolid);
}

void RenderCapsule(const Vector3D& vStart, const Vector3D& vEnd, const float& flRadius, Color c)
{

}