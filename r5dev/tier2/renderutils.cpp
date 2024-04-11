//===== Copyright � 2005-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: A set of utilities to render standard shapes
//
//===========================================================================//
//
///////////////////////////////////////////////////////////////////////////////

#include "mathlib/color.h"
#include "mathlib/vector.h"
#include "mathlib/vector2d.h"
#include "mathlib/vector4d.h"
#include "mathlib/mathlib.h"
#include "tier2/renderutils.h"
#include "engine/debugoverlay.h" // TODO[ AMOS ]: must be a public interface!

//-----------------------------------------------------------------------------
// Purpose: render angled box:
// +y              _+z
// ^               /|
// |              /
// |  +----------+
// | /|         /|
//  / |        / |
// +----------+  |
// |  +-------|--+
// | /        | /
// |/         |/
// +----------+ --> +x
//-----------------------------------------------------------------------------
void DebugDrawBox(const Vector3D& vOrigin, const QAngle& vAngles, const Vector3D& vMins, const Vector3D& vMaxs, Color color, bool bZBuffer)
{
    Vector3D vPoints[8];
    PointsFromAngledBox(vAngles, vMins, vMaxs, &*vPoints);

    v_RenderLine(vOrigin + vPoints[0], vOrigin + vPoints[1], color, bZBuffer);
    v_RenderLine(vOrigin + vPoints[1], vOrigin + vPoints[2], color, bZBuffer);
    v_RenderLine(vOrigin + vPoints[2], vOrigin + vPoints[3], color, bZBuffer);
    v_RenderLine(vOrigin + vPoints[3], vOrigin + vPoints[0], color, bZBuffer);

    v_RenderLine(vOrigin + vPoints[4], vOrigin + vPoints[5], color, bZBuffer);
    v_RenderLine(vOrigin + vPoints[5], vOrigin + vPoints[6], color, bZBuffer);
    v_RenderLine(vOrigin + vPoints[6], vOrigin + vPoints[7], color, bZBuffer);
    v_RenderLine(vOrigin + vPoints[7], vOrigin + vPoints[4], color, bZBuffer);

    v_RenderLine(vOrigin + vPoints[0], vOrigin + vPoints[4], color, bZBuffer);
    v_RenderLine(vOrigin + vPoints[1], vOrigin + vPoints[5], color, bZBuffer);
    v_RenderLine(vOrigin + vPoints[2], vOrigin + vPoints[6], color, bZBuffer);
    v_RenderLine(vOrigin + vPoints[3], vOrigin + vPoints[7], color, bZBuffer);
}

//-----------------------------------------------------------------------------
// Purpose: render cylinder:
// +y           _+z
// ^            /|
// |           /
// |.-'"|"'-. /
// (----|----)
// |'-._|_.-'|
// |    |    |
// |    |    |
// | <--+--> |--> +r
// |    |    |
// |    |    |
//  "-._|_.-" --> +x
//-----------------------------------------------------------------------------
void DebugDrawCylinder(const Vector3D& vOrigin, const QAngle& vAngles, float flRadius, float flHeight, Color color, int nSides, bool bZBuffer)
{
    float flDegrees = 360.f / float(nSides);
    QAngle vComposed;
    Vector3D vForward;
    CUtlVector<Vector3D> vecPoints(0, nSides);

    AngleVectors(vAngles, &vForward);

    for (int i = 0; i < nSides; i++)
    {
        Vector3D right;

        AngleCompose(vAngles, { 0.f, 0.f, flDegrees * i }, vComposed);
        AngleVectors(vComposed, nullptr, &right, nullptr);
        vecPoints.AddToTail(vOrigin + (right * flRadius));
    }

    for (int i = 0; i < nSides; i++)
    {
        Vector3D vStart = vecPoints[i];
        Vector3D vEnd = i == 0 ? vecPoints[nSides - 1] : vecPoints[i - 1];

        v_RenderLine(vStart, vEnd, color, bZBuffer);
        v_RenderLine(vStart + (vForward * flHeight), vEnd + (vForward * flHeight), color, bZBuffer);
        v_RenderLine(vStart, vStart + (vForward * flHeight), color, bZBuffer);
    }
}

//-----------------------------------------------------------------------------
// Purpose: render capsule:
// +y           _+z
// ^            /|
// |           /
// |.-'"|"'-. /
// |----|----|
// |    |    |
// |    |    |
// | <--+--> |--> +r
// |    |    |
// |    |    |
// |----|----|
//  "-..|..-" --> +x
//-----------------------------------------------------------------------------
void DebugDrawCapsule(const Vector3D& vStart, const QAngle& vAngles, const Vector3D& vRadius, float flHeight, Color color, bool bZBuffer)
{
    Vector3D vForward, vUp;
    QAngle vHemi, vComposed;

    AngleVectors(vAngles, nullptr, nullptr, &vUp);

    for (int i = 0; i < 4; i++)
    {
        AngleCompose(vAngles, { 0.f, 90.f * i, 0.f }, vComposed);
        AngleVectors(vComposed, &vForward);
        v_RenderLine(vStart + (vForward * vRadius), vStart + (vForward * vRadius) + (vUp * flHeight), color, bZBuffer);
    }

    AngleCompose(vAngles, { 180.f, 180.f, 0.f }, vHemi);

    DebugDrawHemiSphere(vStart + (vUp * flHeight), vAngles, vRadius, color, 8, bZBuffer);
    DebugDrawHemiSphere(vStart, vHemi, vRadius, color, 8, bZBuffer);
}

//-----------------------------------------------------------------------------
// Purpose: render sphere:
// +y                _+z
// ^                 /|
// |                /
// |   .--"|"--.   /
//  .'     |     '.
// /       |       \
// | <----( )---->-|--> +r
// \       |       /
//  '.     |     .'
//    "-.._|_..-"   --> +x
//-----------------------------------------------------------------------------
void DebugDrawSphere(const Vector3D& vOrigin, float flRadius, Color color, int nSegments, bool bZBuffer)
{
    DebugDrawCircle(vOrigin, { 90.f, 0.f, 0.f }, flRadius, color, nSegments, bZBuffer);
    DebugDrawCircle(vOrigin, { 0.f, 90.f, 0.f }, flRadius, color, nSegments, bZBuffer);
    DebugDrawCircle(vOrigin, { 0.f, 0.f, 90.f }, flRadius, color, nSegments, bZBuffer);
}

//-----------------------------------------------------------------------------
// Purpose: render hemisphere:
// +y                _+z
// ^                 /|
// |                /
// |   .--"|"--.   /
//  .'     |     '.
// /       |       \ /--> +r
// | <----( )---->-|/ --> +x
//-----------------------------------------------------------------------------
void DebugDrawHemiSphere(const Vector3D& vOrigin, const QAngle& vAngles, const Vector3D& vRadius, Color color, int nSegments, bool bZBuffer)
{
    bool bFirstLoop = true;
    float flDegrees = 360.0f / float(nSegments * 2);

    Vector3D vStart[4], vEnd[4], vForward[4];
    QAngle vComposed[4];

    for (int i = 0; i < (nSegments + 1); i++)
    {
        AngleCompose(vAngles, { flDegrees * i - 180, 0, 0 }, vComposed[0]);
        AngleCompose(vAngles, { 0, flDegrees * i - 90, 0 }, vComposed[1]);
        AngleCompose(vAngles, { flDegrees * i + 180, 90, 0 }, vComposed[2]);
        AngleCompose(vAngles, { 0, flDegrees * i + 90, 0 }, vComposed[3]);

        AngleVectors(vComposed[0], &vForward[0]);
        AngleVectors(vComposed[1], &vForward[1]);
        AngleVectors(vComposed[2], &vForward[2]);
        AngleVectors(vComposed[3], &vForward[3]);

        vEnd[0] = vOrigin + (vForward[0] * vRadius);
        vEnd[1] = vOrigin + (vForward[1] * vRadius);
        vEnd[2] = vOrigin + (vForward[2] * vRadius);
        vEnd[3] = vOrigin + (vForward[3] * vRadius);

        if (!bFirstLoop)
        {
            v_RenderLine(vStart[0], vEnd[0], color, bZBuffer);
            v_RenderLine(vStart[1], vEnd[1], color, bZBuffer);
            v_RenderLine(vStart[2], vEnd[2], color, bZBuffer);
            v_RenderLine(vStart[3], vEnd[3], color, bZBuffer);
        }

        vStart[0] = vEnd[0];
        vStart[1] = vEnd[1];
        vStart[2] = vEnd[2];
        vStart[3] = vEnd[3];

        bFirstLoop = false;
    }
}

//-----------------------------------------------------------------------------
// Purpose: render circle:
// +y                _+z
// ^                 /|
// |                /
// |   .--"""--.   /
//  .'           '.
// /               \
// | <----( )---->-|--> +r
// \               /
//  '.           .'
//    "-..___..-"   --> +x
//-----------------------------------------------------------------------------
void DebugDrawCircle(const Vector3D& vOrigin, const QAngle& vAngles, float flRadius, Color color, int nSegments, bool bZBuffer)
{
    bool bFirstLoop = true;
    float flDegrees = 360.f / float(nSegments);

    Vector3D vStart, vEnd, vFirstend, vForward;
    QAngle vComposed;

    for (int i = 0; i < nSegments; i++)
    {
        AngleCompose(vAngles, { 0.f, flDegrees * i, 0.f }, vComposed);
        AngleVectors(vComposed, &vForward);
        vEnd = vOrigin + (vForward * flRadius);

        if (bFirstLoop)
            vFirstend = vEnd;

        if (!bFirstLoop)
            v_RenderLine(vStart, vEnd, color, bZBuffer);

        vStart = vEnd;

        bFirstLoop = false;
    }

    v_RenderLine(vEnd, vFirstend, color, bZBuffer);
}

//-----------------------------------------------------------------------------
// Purpose: render square:
// +y              _+z
// |               /|
// .--------------.
// |              |
// |              |
// |              |
// |              |
// |              |
// |              |
// '--------------' --> +x
//-----------------------------------------------------------------------------
void DebugDrawSquare(const Vector3D& vOrigin, const QAngle& vAngles, float flSquareSize, Color color, bool bZBuffer)
{
    DebugDrawCircle(vOrigin, vAngles, flSquareSize, color, 4, bZBuffer);
}

//-----------------------------------------------------------------------------
// Purpose: render triangle:
// +y              _+z
// |               /|
// |      /\      /
// |     /  \    /
// |    /    \  /
// |   /      \
// |  /        \
// | /          \
//  /            \
// '--------------' --> +x
//-----------------------------------------------------------------------------
void DebugDrawTriangle(const Vector3D& vOrigin, const QAngle& vAngles, float flTriangleSize, Color color, bool bZBuffer)
{
    DebugDrawCircle(vOrigin, vAngles, flTriangleSize, color, 3, bZBuffer);
}

//-----------------------------------------------------------------------------
// Purpose: render mark:
// +y     _+z
// |      /|
// |     /
//   \  /--> +r
// ___\/___
//    /\
//   /  \
//  /    --> +x
//-----------------------------------------------------------------------------
void DebugDrawMark(const Vector3D& vOrigin, float flRadius, const vector<int>& vColor, bool bZBuffer)
{
    v_RenderLine((vOrigin - Vector3D{ flRadius, 0.f, 0.f }), (vOrigin + Vector3D{ flRadius, 0.f, 0.f }), Color(vColor[0], vColor[1], vColor[2], vColor[3]), bZBuffer);
    v_RenderLine((vOrigin - Vector3D{ 0.f, flRadius, 0.f }), (vOrigin + Vector3D{ 0.f, flRadius, 0.f }), Color(vColor[0], vColor[1], vColor[2], vColor[3]), bZBuffer);
    v_RenderLine((vOrigin - Vector3D{ 0.f, 0.f, flRadius }), (vOrigin + Vector3D{ 0.f, 0.f, flRadius }), Color(vColor[0], vColor[1], vColor[2], vColor[3]), bZBuffer);
}

//-----------------------------------------------------------------------------
// Purpose: render star:
// +y     _+z
// |      /|
// |     /
//   \  /--> +r
// ___\/___
//    /\
//   /  \
//       --> +x
//-----------------------------------------------------------------------------
void DrawStar(const Vector3D& vOrigin, float flRadius, bool bZBuffer)
{
    Vector3D vForward;
    for (int i = 0; i < 50; i++)
    {
        AngleVectors({ RandomFloat(0.f, 360.f), RandomFloat(0.f, 360.f), RandomFloat(0.f, 360.f) }, &vForward);
        v_RenderLine(vOrigin, vOrigin + vForward * flRadius, Color(RandomInt(0, 255), RandomInt(0, 255), RandomInt(0, 255), 255), bZBuffer);
    }
}

//-----------------------------------------------------------------------------
// Purpose: render arrow:
// +y     _+z
// |      /|
// |  .  /
// | / \
//  /   \
// /_____\ --> r
//    |
//    |
//    |   --> +x
//-----------------------------------------------------------------------------
void DebugDrawArrow(const Vector3D& vOrigin, const Vector3D& vEnd, float flArraySize, Color color, bool bZBuffer)
{
    Vector3D vAngles;

    v_RenderLine(vOrigin, vEnd, color, bZBuffer);
    AngleVectors(Vector3D(vEnd - vOrigin).Normalized().AsQAngle(), &vAngles);
    DebugDrawCircle(vEnd, vAngles.AsQAngle(), flArraySize, color, 3, bZBuffer);
}

//-----------------------------------------------------------------------------
// Purpose: render 3d axis:
// +y
// ^
// |   _+z
// |   /|
// |  /
// | /
// |/
// +----------> +x
//-----------------------------------------------------------------------------
void DebugDrawAxis(const Vector3D& vOrigin, const QAngle& vAngles, float flScale, bool bZBuffer)
{
    Vector3D vForward, vRight, vUp;
    AngleVectors(vAngles, &vForward, &vRight, &vUp);

    v_RenderLine(vOrigin, vOrigin + vForward * flScale, Color(0, 255, 0, 255), bZBuffer);
    v_RenderLine(vOrigin, vOrigin + vUp * flScale, Color(255, 0, 0, 255), bZBuffer);
    v_RenderLine(vOrigin, vOrigin + vRight * flScale, Color(0, 0, 255, 255), bZBuffer);
}
