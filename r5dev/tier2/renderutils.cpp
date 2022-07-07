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

Vector3D* GetBoxCorners(Vector3D org, QAngle ang, Vector3D mins, Vector3D maxs)
{
    Vector3D forward;
    Vector3D up;
    Vector3D right;

    AngleVectors(ang, &forward, &right, &up);

    Vector3D orgs[8];
    orgs[0] = org + (forward * mins.x) + (right * maxs.y) + (up * maxs.z);
    orgs[1] = org + (forward * mins.x) + (right * mins.y) + (up * maxs.z);
    orgs[2] = org + (forward * maxs.x) + (right * mins.y) + (up * maxs.z);
    orgs[3] = org + (forward * maxs.x) + (right * maxs.y) + (up * maxs.z);
    orgs[4] = org + (forward * mins.x) + (right * maxs.y) + (up * mins.z);
    orgs[5] = org + (forward * mins.x) + (right * mins.y) + (up * mins.z);
    orgs[6] = org + (forward * maxs.x) + (right * mins.y) + (up * mins.z);
    orgs[7] = org + (forward * maxs.x) + (right * maxs.y) + (up * mins.z);

    return orgs;
}

void DrawAngledBox(Vector3D org, QAngle ang, Vector3D mins, Vector3D maxs, int r, int g, int b, int a, bool throughSolid)
{
    Vector3D* orgs = GetBoxCorners(org, ang, mins, maxs);

    v_RenderLine(orgs[0], orgs[1], Color(r, g, b, a), throughSolid);
    v_RenderLine(orgs[1], orgs[2], Color(r, g, b, a), throughSolid);
    v_RenderLine(orgs[2], orgs[3], Color(r, g, b, a), throughSolid);
    v_RenderLine(orgs[3], orgs[0], Color(r, g, b, a), throughSolid);

    v_RenderLine(orgs[4], orgs[5], Color(r, g, b, a), throughSolid);
    v_RenderLine(orgs[5], orgs[6], Color(r, g, b, a), throughSolid);
    v_RenderLine(orgs[6], orgs[7], Color(r, g, b, a), throughSolid);
    v_RenderLine(orgs[7], orgs[4], Color(r, g, b, a), throughSolid);

    v_RenderLine(orgs[0], orgs[4], Color(r, g, b, a), throughSolid);
    v_RenderLine(orgs[1], orgs[5], Color(r, g, b, a), throughSolid);
    v_RenderLine(orgs[2], orgs[6], Color(r, g, b, a), throughSolid);
    v_RenderLine(orgs[3], orgs[7], Color(r, g, b, a), throughSolid);
}

void RenderCapsule(const Vector3D& vStart, const Vector3D& vEnd, const float& flRadius, Color c)
{

}